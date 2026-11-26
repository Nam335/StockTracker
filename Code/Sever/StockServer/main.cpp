#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QTimer>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDateTime>
#include <QRandomGenerator>
#include <QAbstractSocket>
#include <QDebug>
#include <QList>
#include <QStringList>
#include <QHash>
#include <QRegularExpression>
#include <QCryptographicHash>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    const QString databasePath =
        QCoreApplication::applicationDirPath() + "/accounts.db";

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(databasePath);

    if (!db.open()) {
        qCritical() << "Khong mo duoc SQLite database:"
                    << db.lastError().text();
        return 1;
    }

    qInfo() << "SQLite database da mo:" << databasePath;
    QSqlQuery pragmaQuery(db);
    if (!pragmaQuery.exec("PRAGMA foreign_keys = ON")) {
        qCritical() << "Khong bat duoc SQLite Foreign Keys:"
                    << pragmaQuery.lastError().text();
        db.close();
        return 1;
    }

    QSqlQuery query(db);

    const QString createTable = R"(
        CREATE TABLE IF NOT EXISTS accounts (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT NOT NULL COLLATE NOCASE UNIQUE,
            password_hash TEXT NOT NULL,
            password_salt TEXT NOT NULL,
            role TEXT NOT NULL DEFAULT 'user'
                CHECK(role IN ('user', 'admin')),
            created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
            updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
        )
    )";

    if (!query.exec(createTable)) {
        qCritical() << "Khong tao duoc bang accounts:"
                    << query.lastError().text();
        db.close();
        return 1;
    }

    // Tao cac bang phuc vu danh muc co phieu, yeu thich va lich su gia.
    const QStringList featureSchema = {
        R"(
            CREATE TABLE IF NOT EXISTS stocks (
                symbol TEXT PRIMARY KEY COLLATE NOCASE,
                name TEXT NOT NULL,
                display_order INTEGER NOT NULL,
                created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
                updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
            )
        )",
        R"(
            CREATE TABLE IF NOT EXISTS favorites (
                account_id INTEGER NOT NULL,
                symbol TEXT NOT NULL COLLATE NOCASE,
                created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
                PRIMARY KEY (account_id, symbol),
                FOREIGN KEY (account_id)
                    REFERENCES accounts(id) ON DELETE CASCADE,
                FOREIGN KEY (symbol)
                    REFERENCES stocks(symbol) ON DELETE CASCADE
            )
        )",
        R"(
            CREATE TABLE IF NOT EXISTS price_history (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                symbol TEXT NOT NULL COLLATE NOCASE,
                price REAL NOT NULL CHECK(price > 0),
                timestamp TEXT NOT NULL,
                FOREIGN KEY (symbol)
                    REFERENCES stocks(symbol) ON DELETE CASCADE
            )
        )",
        R"(
            CREATE INDEX IF NOT EXISTS idx_price_history_symbol_time
            ON price_history(symbol, timestamp)
        )"
    };

    for (const QString &statement : featureSchema) {
        QSqlQuery schemaQuery(db);

        if (!schemaQuery.exec(statement)) {
            qCritical() << "Loi tao bang/index tinh nang:"
                        << schemaQuery.lastError().text();
            db.close();
            return 1;
        }
    }

    // Chi them du lieu mau neu danh muc chua co co phieu.
    QSqlQuery stockCountQuery(db);

    if (!stockCountQuery.exec("SELECT COUNT(*) FROM stocks")
        || !stockCountQuery.next()) {
        qCritical() << "Khong kiem tra duoc danh muc co phieu:"
                    << stockCountQuery.lastError().text();
        db.close();
        return 1;
    }

    if (stockCountQuery.value(0).toInt() == 0) {
        const QList<QPair<QString, QString>> sampleStocks = {
            {"FPT", "FPT Corporation"},
            {"VCB", "Vietcombank"},
            {"VNM", "Vinamilk"},
            {"HPG", "Hoa Phat Group"},
            {"MWG", "Mobile World"}
        };

        for (int i = 0; i < sampleStocks.size(); ++i) {
            QSqlQuery insertStockQuery(db);
            insertStockQuery.prepare(R"(
                INSERT INTO stocks(symbol, name, display_order)
                VALUES(:symbol, :name, :display_order)
            )");

            insertStockQuery.bindValue(
                ":symbol", sampleStocks[i].first);
            insertStockQuery.bindValue(
                ":name", sampleStocks[i].second);
            insertStockQuery.bindValue(
                ":display_order", i + 1);

            if (!insertStockQuery.exec()) {
                qCritical() << "Loi them co phieu mau:"
                            << insertStockQuery.lastError().text();
                db.close();
                return 1;
            }
        }

        qInfo() << "Da tao 5 co phieu mau.";
    }

    qInfo() << "Da kiem tra cac bang stocks, favorites, price_history.";

    // Xoa lich su gia cu hon 7 ngay.
    auto cleanupOldPriceHistory = [&]() {
        QSqlQuery cleanupQuery(db);

        if (!cleanupQuery.exec(R"(
            DELETE FROM price_history
            WHERE julianday(timestamp) IS NOT NULL
              AND julianday(timestamp) < julianday('now', '-7 days')
        )")) {
            qWarning() << "Khong don dep duoc lich su gia cu:"
                       << cleanupQuery.lastError().text();
            return;
        }

        qInfo() << "Da don dep" << cleanupQuery.numRowsAffected()
                << "ban ghi lich su gia cu hon 7 ngay.";
    };

    // Don dep mot lan ngay khi Server khoi dong.
    cleanupOldPriceHistory();
    QTcpServer server;
    QList<QTcpSocket*> clients;
    QHash<QTcpSocket*, QByteArray> clientBuffers;

    // Tai khoan da dang nhap duoc gan voi socket TCP.
    QHash<QTcpSocket*, QString> authenticatedUsers;

    QStringList symbols;

    QSqlQuery stockListQuery(db);

    if (!stockListQuery.exec(
            "SELECT symbol FROM stocks ORDER BY display_order ASC")) {
        qCritical() << "Khong doc duoc danh muc co phieu:"
                    << stockListQuery.lastError().text();
        db.close();
        return 1;
    }

    while (stockListQuery.next()) {
        symbols.append(stockListQuery.value(0).toString());
    }

    if (symbols.isEmpty()) {
        qCritical() << "Danh muc co phieu dang trong.";
        db.close();
        return 1;
    }
QHash<QString, double> latestPrices;

    for (const QString &symbol : symbols) {
        latestPrices.insert(
            symbol,
            50.0 + QRandomGenerator::global()->bounded(10000) / 100.0);
    }

    if (!server.listen(QHostAddress::AnyIPv4, 5000)) {
        qCritical() << "Khong mo duoc port 5000:"
                    << server.errorString();
        db.close();
        return 1;
    }

    qInfo() << "StockServer dang lang nghe port 5000.";

    auto sendJson = [](QTcpSocket *socket, const QJsonObject &object) {
        if (!socket
            || socket->state() != QAbstractSocket::ConnectedState) {
            return;
        }

        QByteArray data =
            QJsonDocument(object).toJson(QJsonDocument::Compact);
        data.append('\n');
        socket->write(data);
    };

    auto sendSimpleResponse = [&sendJson](
        QTcpSocket *socket,
        const QString &type,
        bool success,
        const QString &message) {

        QJsonObject response;
        response["type"] = type;
        response["success"] = success;
        response["message"] = message;
        sendJson(socket, response);
    };

    auto currentRole = [&db](const QString &username) -> QString {
        QSqlQuery roleQuery(db);
        roleQuery.prepare(
            "SELECT role FROM accounts WHERE username = :username");
        roleQuery.bindValue(":username", username);

        if (!roleQuery.exec() || !roleQuery.next()) {
            return QString();
        }

        return roleQuery.value(0).toString();
    };

    auto handleStockList = [&db, &sendJson](
        QTcpSocket *socket) {

        QJsonObject response;
        response["type"] = "STOCK_LIST_RESPONSE";

        QJsonArray stocksArray;
        QSqlQuery query(db);

        if (!query.exec(
                "SELECT symbol, name, display_order "
                "FROM stocks ORDER BY display_order ASC")) {
            response["success"] = false;
            response["message"] = query.lastError().text();
            response["stocks"] = stocksArray;
            sendJson(socket, response);
            return;
        }

        while (query.next()) {
            QJsonObject stock;
            stock["symbol"] = query.value(0).toString();
            stock["name"] = query.value(1).toString();
            stock["display_order"] = query.value(2).toInt();
            stocksArray.append(stock);
        }

        response["success"] = true;
        response["message"] = "Lay danh muc co phieu thanh cong.";
        response["stocks"] = stocksArray;

        sendJson(socket, response);
    };

    auto handleStockSearch = [&db, &sendJson, &authenticatedUsers, &currentRole](
        QTcpSocket *socket,
        const QJsonObject &request) {

        QJsonObject response;
        response["type"] = "STOCK_SEARCH_RESPONSE";
        QJsonArray stocksArray;

        const QString username = authenticatedUsers.value(socket);
        const QString role = username.isEmpty()
            ? QString()
            : currentRole(username);

        if (role != "user" && role != "admin") {
            response["success"] = false;
            response["message"] = "Ban can dang nhap de tim kiem co phieu.";
            response["stocks"] = stocksArray;
            sendJson(socket, response);
            return;
        }

        const QString keyword =
            request.value("keyword").toString().trimmed();

        if (keyword.isEmpty()) {
            response["success"] = false;
            response["message"] = "Tu khoa tim kiem khong duoc de trong.";
            response["stocks"] = stocksArray;
            sendJson(socket, response);
            return;
        }

        QSqlQuery query(db);
        query.prepare(
            "SELECT symbol, name, display_order "
            "FROM stocks "
            "WHERE symbol LIKE :keyword OR name LIKE :keyword "
            "ORDER BY display_order ASC "
            "LIMIT 50");
        query.bindValue(":keyword", "%" + keyword + "%");

        if (!query.exec()) {
            response["success"] = false;
            response["message"] = query.lastError().text();
            response["stocks"] = stocksArray;
            sendJson(socket, response);
            return;
        }

        while (query.next()) {
            QJsonObject stock;
            stock["symbol"] = query.value(0).toString();
            stock["name"] = query.value(1).toString();
            stock["display_order"] = query.value(2).toInt();
            stocksArray.append(stock);
        }

        response["success"] = true;
        response["message"] = "Tim kiem co phieu thanh cong.";
        response["stocks"] = stocksArray;
        sendJson(socket, response);
    };
    auto handleStockHistory = [&db, &sendJson, &authenticatedUsers, &currentRole](
        QTcpSocket *socket,
        const QJsonObject &request) {

        QJsonObject response;
        response["type"] = "STOCK_HISTORY_RESPONSE";

        QJsonArray pointsArray;
        response["points"] = pointsArray;

        const QString username = authenticatedUsers.value(socket);
        const QString role = username.isEmpty()
            ? QString()
            : currentRole(username);

        if (role != "user" && role != "admin") {
            response["success"] = false;
            response["message"] = "Ban can dang nhap de xem lich su gia.";
            sendJson(socket, response);
            return;
        }

        const QString symbol =
            request.value("symbol").toString().trimmed().toUpper();
        const QString range =
            request.value("range").toString().trimmed();

        response["symbol"] = symbol;
        response["range"] = range;

        if (symbol.isEmpty()) {
            response["success"] = false;
            response["message"] = "Ma co phieu khong duoc de trong.";
            sendJson(socket, response);
            return;
        }

        QString rangeModifier;
        if (range == "1h") {
            rangeModifier = "-1 hours";
        } else if (range == "24h") {
            rangeModifier = "-24 hours";
        } else if (range == "7d") {
            rangeModifier = "-7 days";
        } else {
            response["success"] = false;
            response["message"] = "Khoang thoi gian khong hop le.";
            sendJson(socket, response);
            return;
        }

        QSqlQuery stockQuery(db);
        stockQuery.prepare(
            "SELECT 1 FROM stocks WHERE symbol = :symbol");
        stockQuery.bindValue(":symbol", symbol);

        if (!stockQuery.exec()) {
            response["success"] = false;
            response["message"] = stockQuery.lastError().text();
            sendJson(socket, response);
            return;
        }

        if (!stockQuery.next()) {
            response["success"] = false;
            response["message"] = "Ma co phieu khong ton tai.";
            sendJson(socket, response);
            return;
        }

        QSqlQuery historyQuery(db);
        historyQuery.prepare(
            "SELECT timestamp, price "
            "FROM price_history "
            "WHERE symbol = :symbol "
            "AND julianday(timestamp) >= julianday('now', :rangeModifier) "
            "ORDER BY julianday(timestamp) ASC, id ASC");

        historyQuery.bindValue(":symbol", symbol);
        historyQuery.bindValue(":rangeModifier", rangeModifier);

        if (!historyQuery.exec()) {
            response["success"] = false;
            response["message"] = historyQuery.lastError().text();
            sendJson(socket, response);
            return;
        }

        QList<QJsonObject> allPoints;

        while (historyQuery.next()) {
            QJsonObject point;
            point["timestamp"] = historyQuery.value(0).toString();
            point["price"] = historyQuery.value(1).toDouble();
            allPoints.append(point);
        }

        if (allPoints.size() <= 200) {
            for (const QJsonObject &point : allPoints) {
                pointsArray.append(point);
            }
        } else {
            for (int i = 0; i < 200; ++i) {
                const int index =
                    i * (allPoints.size() - 1) / 199;
                pointsArray.append(allPoints.at(index));
            }
        }

        response["success"] = true;
        response["message"] = "Lay lich su gia thanh cong.";
        response["points"] = pointsArray;
        sendJson(socket, response);
    };
    auto handleFavorites = [&db, &sendJson, &authenticatedUsers, &currentRole](
        QTcpSocket *socket) {

        QJsonObject response;
        response["type"] = "FAVORITES_RESPONSE";
        QJsonArray favoritesArray;

        const QString username = authenticatedUsers.value(socket);
        const QString role = username.isEmpty() ? QString() : currentRole(username);

        if (role != "user" && role != "admin") {
            response["success"] = false;
            response["message"] = "Ban can dang nhap de xem danh sach yeu thich.";
            response["favorites"] = favoritesArray;
            sendJson(socket, response);
            return;
        }

        QSqlQuery query(db);
        query.prepare(R"(
            SELECT f.symbol, s.name, s.display_order, f.created_at
            FROM favorites f
            JOIN stocks s ON s.symbol = f.symbol
            JOIN accounts a ON a.id = f.account_id
            WHERE a.username = :username
            ORDER BY s.display_order ASC
        )");
        query.bindValue(":username", username);

        if (!query.exec()) {
            response["success"] = false;
            response["message"] = query.lastError().text();
            response["favorites"] = favoritesArray;
            sendJson(socket, response);
            return;
        }

        while (query.next()) {
            QJsonObject item;
            item["symbol"] = query.value(0).toString();
            item["name"] = query.value(1).toString();
            item["display_order"] = query.value(2).toInt();
            item["created_at"] = query.value(3).toString();
            favoritesArray.append(item);
        }

        response["success"] = true;
        response["message"] = "Lay danh sach yeu thich thanh cong.";
        response["favorites"] = favoritesArray;
        sendJson(socket, response);
    };

    auto handleFavoriteAdd = [&db, &sendSimpleResponse, &authenticatedUsers, &currentRole](
        QTcpSocket *socket,
        const QJsonObject &request) {

        const QString username = authenticatedUsers.value(socket);
        const QString role = username.isEmpty() ? QString() : currentRole(username);

        if (role != "user" && role != "admin") {
            sendSimpleResponse(socket, "FAVORITE_ADD_RESPONSE", false,
                               "Ban can dang nhap de them co phieu yeu thich.");
            return;
        }

        const QString symbol = request.value("symbol").toString().trimmed().toUpper();
        if (symbol.isEmpty()) {
            sendSimpleResponse(socket, "FAVORITE_ADD_RESPONSE", false,
                               "Ma co phieu khong duoc de trong.");
            return;
        }

        QSqlQuery accountQuery(db);
        accountQuery.prepare(
            "SELECT id FROM accounts WHERE username = :username");
        accountQuery.bindValue(":username", username);

        if (!accountQuery.exec() || !accountQuery.next()) {
            sendSimpleResponse(socket, "FAVORITE_ADD_RESPONSE", false,
                               "Khong tim thay tai khoan.");
            return;
        }

        const int accountId = accountQuery.value(0).toInt();

        QSqlQuery stockQuery(db);
        stockQuery.prepare(
            "SELECT 1 FROM stocks WHERE symbol = :symbol");
        stockQuery.bindValue(":symbol", symbol);

        if (!stockQuery.exec() || !stockQuery.next()) {
            sendSimpleResponse(socket, "FAVORITE_ADD_RESPONSE", false,
                               "Ma co phieu khong ton tai.");
            return;
        }

        QSqlQuery insertQuery(db);
        insertQuery.prepare(R"(
            INSERT OR IGNORE INTO favorites(account_id, symbol)
            VALUES(:account_id, :symbol)
        )");
        insertQuery.bindValue(":account_id", accountId);
        insertQuery.bindValue(":symbol", symbol);

        if (!insertQuery.exec()) {
            sendSimpleResponse(socket, "FAVORITE_ADD_RESPONSE", false,
                               insertQuery.lastError().text());
            return;
        }

        if (insertQuery.numRowsAffected() == 0) {
            sendSimpleResponse(socket, "FAVORITE_ADD_RESPONSE", true,
                               "Co phieu da co trong danh sach yeu thich.");
        } else {
            sendSimpleResponse(socket, "FAVORITE_ADD_RESPONSE", true,
                               "Them co phieu yeu thich thanh cong.");
        }
    };

    auto handleFavoriteRemove = [&db, &sendSimpleResponse, &authenticatedUsers, &currentRole](
        QTcpSocket *socket,
        const QJsonObject &request) {

        const QString username = authenticatedUsers.value(socket);
        const QString role = username.isEmpty() ? QString() : currentRole(username);

        if (role != "user" && role != "admin") {
            sendSimpleResponse(socket, "FAVORITE_REMOVE_RESPONSE", false,
                               "Ban can dang nhap de xoa co phieu yeu thich.");
            return;
        }

        const QString symbol = request.value("symbol").toString().trimmed().toUpper();
        if (symbol.isEmpty()) {
            sendSimpleResponse(socket, "FAVORITE_REMOVE_RESPONSE", false,
                               "Ma co phieu khong duoc de trong.");
            return;
        }

        QSqlQuery accountQuery(db);
        accountQuery.prepare(
            "SELECT id FROM accounts WHERE username = :username");
        accountQuery.bindValue(":username", username);

        if (!accountQuery.exec() || !accountQuery.next()) {
            sendSimpleResponse(socket, "FAVORITE_REMOVE_RESPONSE", false,
                               "Khong tim thay tai khoan.");
            return;
        }

        const int accountId = accountQuery.value(0).toInt();

        QSqlQuery deleteQuery(db);
        deleteQuery.prepare(R"(
            DELETE FROM favorites
            WHERE account_id = :account_id AND symbol = :symbol
        )");
        deleteQuery.bindValue(":account_id", accountId);
        deleteQuery.bindValue(":symbol", symbol);

        if (!deleteQuery.exec()) {
            sendSimpleResponse(socket, "FAVORITE_REMOVE_RESPONSE", false,
                               deleteQuery.lastError().text());
            return;
        }

        if (deleteQuery.numRowsAffected() == 0) {
            sendSimpleResponse(socket, "FAVORITE_REMOVE_RESPONSE", true,
                               "Co phieu khong co trong danh sach yeu thich.");
        } else {
            sendSimpleResponse(socket, "FAVORITE_REMOVE_RESPONSE", true,
                               "Xoa co phieu yeu thich thanh cong.");
        }
    };
    auto handleRegister = [&db, &sendJson](
        QTcpSocket *socket,
        const QJsonObject &request) {

        QJsonObject response;
        response["type"] = "REGISTER_RESPONSE";

        const QString username =
            request.value("username").toString().trimmed();
        const QString password =
            request.value("password").toString();

        static const QRegularExpression usernamePattern(
            "^[A-Za-z0-9_]{3,}$");

        if (!usernamePattern.match(username).hasMatch()) {
            response["success"] = false;
            response["message"] =
                "Username phai co it nhat 3 ky tu, chi gom chu, so va dau _";
            sendJson(socket, response);
            return;
        }

        if (password.isEmpty()) {
            response["success"] = false;
            response["message"] = "Mat khau khong duoc de trong";
            sendJson(socket, response);
            return;
        }

        const QByteArray salt =
            QByteArray::number(
                QRandomGenerator::system()->generate64(), 16)
            + QByteArray::number(
                QRandomGenerator::system()->generate64(), 16);

        const QByteArray passwordHash =
            QCryptographicHash::hash(
                salt + password.toUtf8(),
                QCryptographicHash::Sha256).toHex();

        QSqlQuery insertQuery(db);
        insertQuery.prepare(R"(
            INSERT INTO accounts
                (username, password_hash, password_salt, role)
            VALUES
                (:username, :password_hash, :password_salt, 'user')
        )");

        insertQuery.bindValue(":username", username);
        insertQuery.bindValue(
            ":password_hash", QString::fromLatin1(passwordHash));
        insertQuery.bindValue(
            ":password_salt", QString::fromLatin1(salt));

        if (!insertQuery.exec()) {
            const bool duplicate =
                insertQuery.lastError().text().contains(
                    "UNIQUE constraint failed",
                    Qt::CaseInsensitive);

            response["success"] = false;
            response["message"] = duplicate
                ? "Ten dang nhap da ton tai"
                : "Khong the tao tai khoan";

            qWarning() << "Loi dang ky:"
                       << insertQuery.lastError().text();
            sendJson(socket, response);
            return;
        }

        qInfo() << "Dang ky tai khoan thanh cong:" << username;
        response["success"] = true;
        response["message"] = "Dang ky tai khoan thanh cong";
        sendJson(socket, response);
    };

    auto handleLogin = [&db, &authenticatedUsers, &sendJson](
        QTcpSocket *socket,
        const QJsonObject &request) {

        // Moi lan dang nhap phai xac thuc lai.
        authenticatedUsers.remove(socket);

        QJsonObject response;
        response["type"] = "LOGIN_RESPONSE";

        const QString username =
            request.value("username").toString().trimmed();
        const QString password =
            request.value("password").toString();

        if (username.isEmpty() || password.isEmpty()) {
            response["success"] = false;
            response["message"] =
                "Vui long nhap ten dang nhap va mat khau";
            sendJson(socket, response);
            return;
        }

        QSqlQuery loginQuery(db);
        loginQuery.prepare(R"(
            SELECT password_hash, password_salt, role
            FROM accounts
            WHERE username = :username
        )");
        loginQuery.bindValue(":username", username);

        if (!loginQuery.exec()) {
            qWarning() << "Loi truy van dang nhap:"
                       << loginQuery.lastError().text();
            response["success"] = false;
            response["message"] =
                "Khong the xu ly yeu cau dang nhap";
            sendJson(socket, response);
            return;
        }

        if (!loginQuery.next()) {
            response["success"] = false;
            response["message"] =
                "Ten dang nhap hoac mat khau khong dung";
            sendJson(socket, response);
            return;
        }

        const QByteArray storedHash =
            loginQuery.value(0).toString().toLatin1();
        const QByteArray salt =
            loginQuery.value(1).toString().toLatin1();
        const QString role =
            loginQuery.value(2).toString();

        const QByteArray inputHash =
            QCryptographicHash::hash(
                salt + password.toUtf8(),
                QCryptographicHash::Sha256).toHex();

        if (inputHash != storedHash) {
            response["success"] = false;
            response["message"] =
                "Ten dang nhap hoac mat khau khong dung";
            sendJson(socket, response);
            return;
        }

        authenticatedUsers.insert(socket, username);

        qInfo() << "Dang nhap thanh cong:"
                << username << "role:" << role;

        response["success"] = true;
        response["message"] = "Dang nhap thanh cong";
        response["role"] = role;
        sendJson(socket, response);
    };

    auto handleAdminList = [&db, &authenticatedUsers,
                            &currentRole, &sendJson, &sendSimpleResponse](
        QTcpSocket *socket) {

        const QString username = authenticatedUsers.value(socket);

        if (username.isEmpty() || currentRole(username) != "admin") {
            sendSimpleResponse(
                socket, "ADMIN_LIST_RESPONSE",
                false, "Tu choi truy cap: yeu cau quyen Admin");
            return;
        }

        QSqlQuery listQuery(db);

        if (!listQuery.exec(
                "SELECT id, username, role, created_at "
                "FROM accounts ORDER BY id")) {
            qWarning() << "Loi lay danh sach tai khoan:"
                       << listQuery.lastError().text();

            sendSimpleResponse(
                socket, "ADMIN_LIST_RESPONSE",
                false, "Khong the lay danh sach tai khoan");
            return;
        }

        QJsonArray accounts;

        while (listQuery.next()) {
            QJsonObject account;
            account["id"] = listQuery.value(0).toInt();
            account["username"] = listQuery.value(1).toString();
            account["role"] = listQuery.value(2).toString();
            account["created_at"] = listQuery.value(3).toString();
            accounts.append(account);
        }

        QJsonObject response;
        response["type"] = "ADMIN_LIST_RESPONSE";
        response["success"] = true;
        response["accounts"] = accounts;
        sendJson(socket, response);
    };

    auto handleAdminRole = [&db, &authenticatedUsers,
                            &currentRole, &sendJson,
                            &sendSimpleResponse](
        QTcpSocket *socket,
        const QJsonObject &request) {

        const QString adminUsername =
            authenticatedUsers.value(socket);

        if (adminUsername.isEmpty()
            || currentRole(adminUsername) != "admin") {
            sendSimpleResponse(
                socket, "ADMIN_ROLE_RESPONSE",
                false, "Tu choi truy cap: yeu cau quyen Admin");
            return;
        }

        const int targetId =
            request.value("user_id").toInt(-1);
        const QString newRole =
            request.value("role").toString();

        if (targetId <= 0
            || (newRole != "user" && newRole != "admin")) {
            sendSimpleResponse(
                socket, "ADMIN_ROLE_RESPONSE",
                false, "Du lieu thay doi quyen khong hop le");
            return;
        }

        QSqlQuery targetQuery(db);
        targetQuery.prepare(
            "SELECT username, role FROM accounts WHERE id = :id");
        targetQuery.bindValue(":id", targetId);

        if (!targetQuery.exec() || !targetQuery.next()) {
            sendSimpleResponse(
                socket, "ADMIN_ROLE_RESPONSE",
                false, "Khong tim thay tai khoan");
            return;
        }

        const QString targetUsername =
            targetQuery.value(0).toString();
        const QString oldRole =
            targetQuery.value(1).toString();

        if (targetUsername.compare(
                adminUsername, Qt::CaseInsensitive) == 0) {
            sendSimpleResponse(
                socket, "ADMIN_ROLE_RESPONSE",
                false, "Khong the tu thay doi quyen cua chinh minh");
            return;
        }

        if (oldRole == newRole) {
            sendSimpleResponse(
                socket, "ADMIN_ROLE_RESPONSE",
                true, "Tai khoan da co vai tro nay");
            return;
        }

        if (oldRole == "admin" && newRole == "user") {
            QSqlQuery countQuery(db);

            if (!countQuery.exec(
                    "SELECT COUNT(*) FROM accounts WHERE role = 'admin'")) {
                sendSimpleResponse(
                    socket, "ADMIN_ROLE_RESPONSE",
                    false, "Khong the kiem tra so luong Admin");
                return;
            }

            countQuery.next();

            if (countQuery.value(0).toInt() <= 1) {
                sendSimpleResponse(
                    socket, "ADMIN_ROLE_RESPONSE",
                    false, "Khong the ha quyen Admin cuoi cung");
                return;
            }
        }

        QSqlQuery updateQuery(db);
        updateQuery.prepare(R"(
            UPDATE accounts
            SET role = :role, updated_at = CURRENT_TIMESTAMP
            WHERE id = :id
        )");
        updateQuery.bindValue(":role", newRole);
        updateQuery.bindValue(":id", targetId);

        if (!updateQuery.exec() || updateQuery.numRowsAffected() != 1) {
            qWarning() << "Loi cap nhat quyen:"
                       << updateQuery.lastError().text();

            sendSimpleResponse(
                socket, "ADMIN_ROLE_RESPONSE",
                false, "Khong the cap nhat quyen tai khoan");
            return;
        }

        qInfo() << "Admin" << adminUsername
                << "da doi quyen" << targetUsername
                << "tu" << oldRole << "sang" << newRole;

        sendSimpleResponse(
            socket, "ADMIN_ROLE_RESPONSE",
            true, "Cap nhat quyen tai khoan thanh cong");
    };

        auto handleAdminStock = [&db, &authenticatedUsers, &currentRole,
                             &sendSimpleResponse, &clients, &symbols,
                             &latestPrices, &handleStockList](
        QTcpSocket *socket, const QJsonObject &request) {

        const QString username = authenticatedUsers.value(socket);
        if (username.isEmpty() || currentRole(username) != "admin") {
            sendSimpleResponse(socket, "ADMIN_STOCK_RESPONSE", false,
                               "Tu choi truy cap: yeu cau quyen Admin");
            return;
        }

        const QString action = request.value("action").toString();

        if (action == "add") {
            const QString symbol =
                request.value("symbol").toString().trimmed().toUpper();
            const QString name =
                request.value("name").toString().trimmed();

            if (!QRegularExpression("^[A-Z0-9.]{1,10}$").match(symbol).hasMatch()
                || name.isEmpty() || name.size() > 100) {
                sendSimpleResponse(socket, "ADMIN_STOCK_RESPONSE", false,
                                   "Ma hoac ten co phieu khong hop le");
                return;
            }

            if (!db.transaction()) {
                sendSimpleResponse(socket, "ADMIN_STOCK_RESPONSE", false,
                                   "Khong bat dau duoc giao dich database");
                return;
            }

            QSqlQuery query(db);
            query.prepare(
                "INSERT INTO stocks(symbol, name, display_order) "
                "VALUES(:symbol, :name, "
                "(SELECT COALESCE(MAX(display_order), 0) + 1 FROM stocks))");
            query.bindValue(":symbol", symbol);
            query.bindValue(":name", name);

            if (!query.exec()) {
                db.rollback();
                sendSimpleResponse(socket, "ADMIN_STOCK_RESPONSE", false,
                                   "Khong them duoc co phieu: ma co the da ton tai");
                return;
            }

            if (!db.commit()) {
                db.rollback();
                sendSimpleResponse(socket, "ADMIN_STOCK_RESPONSE", false,
                                   "Khong luu duoc co phieu");
                return;
            }

            symbols.append(symbol);
            latestPrices.insert(
                symbol,
                50.0 + QRandomGenerator::global()->bounded(10000) / 100.0);

        } else if (action == "remove") {
            const QString symbol =
                request.value("symbol").toString().trimmed().toUpper();

            if (!symbols.contains(symbol)) {
                sendSimpleResponse(socket, "ADMIN_STOCK_RESPONSE", false,
                                   "Ma co phieu khong ton tai");
                return;
            }

            if (symbols.size() <= 1) {
                sendSimpleResponse(socket, "ADMIN_STOCK_RESPONSE", false,
                                   "Khong the xoa co phieu cuoi cung");
                return;
            }

            if (!db.transaction()) {
                sendSimpleResponse(socket, "ADMIN_STOCK_RESPONSE", false,
                                   "Khong bat dau duoc giao dich database");
                return;
            }

            QSqlQuery query(db);
            query.prepare("DELETE FROM stocks WHERE symbol = :symbol");
            query.bindValue(":symbol", symbol);

            if (!query.exec() || query.numRowsAffected() != 1) {
                db.rollback();
                sendSimpleResponse(socket, "ADMIN_STOCK_RESPONSE", false,
                                   "Khong xoa duoc co phieu");
                return;
            }

            if (!db.commit()) {
                db.rollback();
                sendSimpleResponse(socket, "ADMIN_STOCK_RESPONSE", false,
                                   "Khong luu duoc thao tac xoa");
                return;
            }

            symbols.removeAll(symbol);
            latestPrices.remove(symbol);

        } else if (action == "reorder") {
            const QJsonArray requested = request.value("symbols").toArray();

            if (requested.size() != symbols.size()) {
                sendSimpleResponse(socket, "ADMIN_STOCK_RESPONSE", false,
                                   "Danh sach sap xep phai gom day du co phieu");
                return;
            }

            QStringList reordered;
            for (const QJsonValue &value : requested) {
                const QString symbol = value.toString().trimmed().toUpper();

                if (symbol.isEmpty() || !symbols.contains(symbol)
                    || reordered.contains(symbol)) {
                    sendSimpleResponse(socket, "ADMIN_STOCK_RESPONSE", false,
                                       "Danh sach sap xep khong hop le");
                    return;
                }

                reordered.append(symbol);
            }

            if (!db.transaction()) {
                sendSimpleResponse(socket, "ADMIN_STOCK_RESPONSE", false,
                                   "Khong bat dau duoc giao dich database");
                return;
            }

            QSqlQuery query(db);
            query.prepare(
                "UPDATE stocks SET display_order = :order, "
                "updated_at = CURRENT_TIMESTAMP WHERE symbol = :symbol");

            bool ok = true;
            for (int i = 0; i < reordered.size(); ++i) {
                query.bindValue(":order", i + 1);
                query.bindValue(":symbol", reordered[i]);

                if (!query.exec() || query.numRowsAffected() != 1) {
                    ok = false;
                    break;
                }
            }

            if (!ok) {
                db.rollback();
                sendSimpleResponse(socket, "ADMIN_STOCK_RESPONSE", false,
                                   "Khong cap nhat duoc thu tu co phieu");
                return;
            }

            if (!db.commit()) {
                db.rollback();
                sendSimpleResponse(socket, "ADMIN_STOCK_RESPONSE", false,
                                   "Khong luu duoc thu tu co phieu");
                return;
            }

            symbols = reordered;

        } else {
            sendSimpleResponse(socket, "ADMIN_STOCK_RESPONSE", false,
                               "Thao tac khong hop le");
            return;
        }

        sendSimpleResponse(socket, "ADMIN_STOCK_RESPONSE", true,
                           "Cap nhat danh muc co phieu thanh cong");

        for (QTcpSocket *client : clients) {
            handleStockList(client);
        }
    };
QObject::connect(
        &server, &QTcpServer::newConnection, [&]() {

        while (server.hasPendingConnections()) {
            QTcpSocket *socket = server.nextPendingConnection();

            clients.append(socket);
            clientBuffers.insert(socket, QByteArray());

            qInfo() << "Client ket noi:"
                    << socket->peerAddress().toString();

            QJsonObject snapshot;
            snapshot["type"] = "SNAPSHOT";
            snapshot["timestamp"] =
                QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

            QJsonArray pricesArray;

            for (const QString &symbol : symbols) {
                QJsonObject quote;
                quote["symbol"] = symbol;
                quote["price"] = latestPrices.value(symbol);
                pricesArray.append(quote);
            }

            snapshot["prices"] = pricesArray;
            sendJson(socket, snapshot);
            handleStockList(socket);

            QObject::connect(
                socket, &QTcpSocket::readyRead,
                [socket, &clientBuffers,
                 &handleStockList, &handleStockSearch, &handleStockHistory, &handleFavorites, &handleFavoriteAdd, &handleFavoriteRemove, &handleRegister, &handleLogin,
                 &handleAdminList, &handleAdminRole, &handleAdminStock]() {

                clientBuffers[socket].append(socket->readAll());

                while (true) {
                    const int newlineIndex =
                        clientBuffers[socket].indexOf('\n');

                    if (newlineIndex < 0) {
                        break;
                    }

                    const QByteArray line =
                        clientBuffers[socket].left(newlineIndex).trimmed();

                    clientBuffers[socket].remove(0, newlineIndex + 1);

                    if (line.isEmpty()) {
                        continue;
                    }

                    QJsonParseError parseError;
                    const QJsonDocument document =
                        QJsonDocument::fromJson(line, &parseError);

                    if (parseError.error != QJsonParseError::NoError
                        || !document.isObject()) {
                        qWarning() << "JSON khong hop le:" << line;
                        continue;
                    }

                    const QJsonObject request = document.object();
                    const QString type =
                        request.value("type").toString();

                    if (type == "REGISTER") {
                        handleRegister(socket, request);
                    } else if (type == "LOGIN") {
                        handleLogin(socket, request);
                    } else if (type == "STOCK_LIST_REQUEST") {
                       handleStockList(socket);
                   } else if (type == "STOCK_SEARCH_REQUEST") {
                       handleStockSearch(socket, request);
                   } else if (type == "STOCK_HISTORY_REQUEST") {
                       handleStockHistory(socket, request);
                   } else if (type == "FAVORITES_REQUEST") {
    handleFavorites(socket);
} else if (type == "FAVORITE_ADD") {
    handleFavoriteAdd(socket, request);
} else if (type == "FAVORITE_REMOVE") {
    handleFavoriteRemove(socket, request);} else if (type == "ADMIN_LIST_REQUEST") {
                        handleAdminList(socket);
                    } else if (type == "ADMIN_ROLE_REQUEST") {
                        handleAdminRole(socket, request);
                    } else if (type == "ADMIN_STOCK_REQUEST") {
                        handleAdminStock(socket, request);
                    } else {
                        qWarning() << "Loai yeu cau chua ho tro:" << type;
                    }
                }
            });

            QObject::connect(
                socket, &QTcpSocket::disconnected,
                [&clients, &clientBuffers,
                 &authenticatedUsers, socket]() {

                clients.removeAll(socket);
                clientBuffers.remove(socket);
                authenticatedUsers.remove(socket);

                qInfo() << "Client da ngat ket noi.";
                socket->deleteLater();
            });
        }
    });

    QTimer timer;

    QObject::connect(&timer, &QTimer::timeout, [&]() {
        for (const QString &symbol : symbols) {
            const double price =
                50.0 + QRandomGenerator::global()->bounded(10000) / 100.0;

            latestPrices.insert(symbol, price);

            QJsonObject message;
            message["type"] = "PRICE_UPDATE";
            message["symbol"] = symbol;
            message["price"] = price;
            message["timestamp"] =
                QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

            const QByteArray data =
                QJsonDocument(message).toJson(QJsonDocument::Compact)
                + '\n';

            for (QTcpSocket *client : clients) {
                if (client->state() == QAbstractSocket::ConnectedState) {
                    client->write(data);
                }
            }
        }

        qInfo() << "So Client dang ket noi:" << clients.size();
    });

    // Luu gia hien tai cua tat ca co phieu moi 5 giay.
    QTimer historyTimer;

    QObject::connect(&historyTimer, &QTimer::timeout, [&]() {
        if (!db.transaction()) {
            qCritical() << "Khong bat dau duoc transaction lich su gia:"
                        << db.lastError().text();
            return;
        }

        QSqlQuery historyQuery(db);

        if (!historyQuery.prepare(R"(
            INSERT INTO price_history(symbol, price, timestamp)
            VALUES(:symbol, :price, :timestamp)
        )")) {
            qCritical() << "Khong chuan bi duoc lenh luu lich su gia:"
                        << historyQuery.lastError().text();
            db.rollback();
            return;
        }

        const QString timestamp =
            QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);

        bool saveSuccess = true;

        for (const QString &symbol : symbols) {
            historyQuery.bindValue(":symbol", symbol);
            historyQuery.bindValue(":price", latestPrices.value(symbol));
            historyQuery.bindValue(":timestamp", timestamp);

            if (!historyQuery.exec()) {
                qCritical() << "Loi luu lich su gia cho" << symbol
                            << ":" << historyQuery.lastError().text();
                saveSuccess = false;
                break;
            }
        }

        if (!saveSuccess) {
            db.rollback();
            qWarning() << "Da rollback luot luu lich su gia.";
            return;
        }

        if (!db.commit()) {
            qCritical() << "Khong commit duoc lich su gia:"
                        << db.lastError().text();
            db.rollback();
            return;
        }

        qInfo() << "Da luu lich su gia cho" << symbols.size()
                << "co phieu luc" << timestamp;
    });

    // Dinh ky don dep lich su gia cu moi 1 gio.
    QTimer cleanupTimer;

    QObject::connect(&cleanupTimer, &QTimer::timeout, [&]() {
        cleanupOldPriceHistory();
    });

    cleanupTimer.start(3600000);
    historyTimer.start(5000);
    timer.start(1000);
    return app.exec();
}
