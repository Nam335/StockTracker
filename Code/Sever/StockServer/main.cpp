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

    QTcpServer server;
    QList<QTcpSocket*> clients;
    QHash<QTcpSocket*, QByteArray> clientBuffers;

    // Tai khoan da dang nhap duoc gan voi socket TCP.
    QHash<QTcpSocket*, QString> authenticatedUsers;

    const QStringList symbols = {
        "FPT", "VCB", "VNM", "HPG", "MWG"
    };

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

            QObject::connect(
                socket, &QTcpSocket::readyRead,
                [socket, &clientBuffers,
                 &handleRegister, &handleLogin,
                 &handleAdminList, &handleAdminRole]() {

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
                    } else if (type == "ADMIN_LIST_REQUEST") {
                        handleAdminList(socket);
                    } else if (type == "ADMIN_ROLE_REQUEST") {
                        handleAdminRole(socket, request);
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

    timer.start(1000);
    return app.exec();
}
