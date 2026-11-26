#include "stockclient.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonParseError>
#include <QAbstractSocket>
#include <QDebug>

// Thoi gian cho truoc moi lan thu ket noi lai (ms).
static const int kReconnectIntervalMs = 3000;

StockClient::StockClient(QObject *parent)
    : QObject(parent),
      socket(new QTcpSocket(this)),
      reconnectTimer(new QTimer(this))
{
    reconnectTimer->setSingleShot(true);
    reconnectTimer->setInterval(kReconnectIntervalMs);

    connect(reconnectTimer, &QTimer::timeout, this, [this]() {
        if (!autoReconnect
            || socket->state() != QAbstractSocket::UnconnectedState) {
            return;
        }

        emit statusChanged("Dang ket noi lai den StockServer...");
        socket->connectToHost(serverHost, serverPort);
    });

    connect(socket, &QTcpSocket::connected, this, [this]() {
        buffer.clear();
        emit statusChanged("Da ket noi den StockServer.");
    });

    connect(socket, &QTcpSocket::disconnected, this, [this]() {
        buffer.clear();
        emit statusChanged("Da ngat ket noi.");
        scheduleReconnect();
    });

    connect(socket, &QTcpSocket::readyRead,
            this, &StockClient::readData);

    connect(socket, &QTcpSocket::errorOccurred,
            this, [this](QAbstractSocket::SocketError) {
                emit statusChanged(
                    "Loi TCP: " + socket->errorString());
                scheduleReconnect();
            });
}

void StockClient::connectToServer(
    const QString &host, quint16 port)
{
    if (socket->state() != QAbstractSocket::UnconnectedState) {
        socket->abort();
    }

    buffer.clear();

    serverHost = host;
    serverPort = port;
    autoReconnect = true;
    reconnectTimer->stop();

    emit statusChanged("Dang ket noi den StockServer...");
    socket->connectToHost(host, port);
}

void StockClient::disconnectFromServer()
{
    // Nguoi dung chu dong ngat: khong tu ket noi lai nua.
    autoReconnect = false;
    reconnectTimer->stop();

    if (socket->state() == QAbstractSocket::ConnectedState) {
        socket->disconnectFromHost();
        return;
    }

    socket->abort();
    emit statusChanged("Da ngat ket noi.");
}

// Hen gio thu ket noi lai neu nguoi dung chua bam Ngat ket noi.
void StockClient::scheduleReconnect()
{
    if (!autoReconnect) {
        return;
    }

    emit statusChanged(
        "Mat ket noi. Tu dong ket noi lai sau "
        + QString::number(kReconnectIntervalMs / 1000)
        + " giay...");

    if (!reconnectTimer->isActive()) {
        reconnectTimer->start();
    }
}

void StockClient::registerAccount(const QString &username,
                                  const QString &password)
{
    if (socket->state() != QAbstractSocket::ConnectedState) {
        emit registrationResult(
            false, "Vui long ket noi Server truoc khi dang ky.");
        return;
    }

    QJsonObject request;
    request["type"] = "REGISTER";
    request["username"] = username;
    request["password"] = password;

    QByteArray message =
        QJsonDocument(request).toJson(QJsonDocument::Compact);

    message.append('\n');

    if (socket->write(message) < 0) {
        emit registrationResult(
            false, "Khong gui duoc yeu cau dang ky: "
                + socket->errorString());
    }
}

void StockClient::login(const QString &username,
                        const QString &password)
{
    if (socket->state() != QAbstractSocket::ConnectedState) {
        emit loginResult(
            false,
            "Vui long ket noi Server truoc khi dang nhap.",
            "");
        return;
    }

    QJsonObject request;
    request["type"] = "LOGIN";
    request["username"] = username;
    request["password"] = password;

    QByteArray message =
        QJsonDocument(request).toJson(QJsonDocument::Compact);

    message.append('\n');

    if (socket->write(message) < 0) {
        emit loginResult(
            false,
            "Khong gui duoc yeu cau dang nhap: "
                + socket->errorString(),
            "");
    }
}

void StockClient::requestAdminAccounts()
{
    if (socket->state() != QAbstractSocket::ConnectedState) {
        emit adminAccountsResult(false, {},
                                 "Chua ket noi Server.");
        return;
    }

    QJsonObject request;
    request["type"] = "ADMIN_LIST_REQUEST";

    QByteArray message =
        QJsonDocument(request).toJson(QJsonDocument::Compact);
    message.append('\n');

    if (socket->write(message) < 0) {
        emit adminAccountsResult(
            false, {},
            "Khong gui duoc yeu cau: " + socket->errorString());
    }
}

void StockClient::setAccountRole(int userId, const QString &role)
{
    if (socket->state() != QAbstractSocket::ConnectedState) {
        emit adminRoleUpdateResult(false,
                                   "Chua ket noi Server.");
        return;
    }

    QJsonObject request;
    request["type"] = "ADMIN_ROLE_REQUEST";
    request["user_id"] = userId;
    request["role"] = role;

    QByteArray message =
        QJsonDocument(request).toJson(QJsonDocument::Compact);
    message.append('\n');

    if (socket->write(message) < 0) {
        emit adminRoleUpdateResult(
            false,
            "Khong gui duoc yeu cau: " + socket->errorString());
    }
}


// Gui yeu cau lay danh muc co phieu.
void StockClient::requestStockList()
{
    QJsonObject request;
    request["type"] = "STOCK_LIST_REQUEST";

    QByteArray message =
        QJsonDocument(request).toJson(QJsonDocument::Compact);
    message.append('\n');

    if (socket->state() != QAbstractSocket::ConnectedState) {
        emit stockListReceived(false, {}, "Chua ket noi Server.");
        return;
    }

    socket->write(message);
}

// Gui yeu cau lay lich su gia.
void StockClient::requestStockHistory(
    const QString &symbol, const QString &range)
{
    if (socket->state() != QAbstractSocket::ConnectedState) {
        emit stockHistoryReceived(
            false, symbol, {}, "Chua ket noi Server.");
        return;
    }

    QJsonObject request;
    request["type"] = "STOCK_HISTORY_REQUEST";
    request["symbol"] = symbol;
    request["range"] = range;

    QByteArray message =
        QJsonDocument(request).toJson(QJsonDocument::Compact);
    message.append('\n');
    socket->write(message);
}

// Gui yeu cau them co phieu.
void StockClient::addStock(
    const QString &symbol, const QString &name)
{
    QJsonObject request;
    request["type"] = "ADMIN_STOCK_REQUEST";
    request["action"] = "add";
    request["symbol"] = symbol;
    request["name"] = name;

    QByteArray message =
        QJsonDocument(request).toJson(QJsonDocument::Compact);
    message.append('\n');

    if (socket->state() != QAbstractSocket::ConnectedState) {
        emit adminStockResult(false, "Chua ket noi Server.");
        return;
    }

    socket->write(message);
}

// Gui yeu cau xoa co phieu.
void StockClient::removeStock(const QString &symbol)
{
    QJsonObject request;
    request["type"] = "ADMIN_STOCK_REQUEST";
    request["action"] = "remove";
    request["symbol"] = symbol;

    QByteArray message =
        QJsonDocument(request).toJson(QJsonDocument::Compact);
    message.append('\n');

    if (socket->state() != QAbstractSocket::ConnectedState) {
        emit adminStockResult(false, "Chua ket noi Server.");
        return;
    }

    socket->write(message);
}

// Gui danh sach ma theo thu tu moi.
void StockClient::reorderStocks(const QStringList &symbols)
{
    QJsonObject request;
    request["type"] = "ADMIN_STOCK_REQUEST";
    request["action"] = "reorder";

    QJsonArray array;
    for (const QString &symbol : symbols)
        array.append(symbol);

    request["symbols"] = array;

    QByteArray message =
        QJsonDocument(request).toJson(QJsonDocument::Compact);
    message.append('\n');

    if (socket->state() != QAbstractSocket::ConnectedState) {
        emit adminStockResult(false, "Chua ket noi Server.");
        return;
    }

    socket->write(message);
}
// Gui yeu cau lay danh sach co phieu yeu thich.
void StockClient::requestFavorites()
{
    if (socket->state() != QAbstractSocket::ConnectedState) {
        emit favoritesReceived(false, {}, "Chua ket noi Server.");
        return;
    }

    QJsonObject request;
    request["type"] = "FAVORITES_REQUEST";

    QByteArray message =
        QJsonDocument(request).toJson(QJsonDocument::Compact);
    message.append('\n');

    socket->write(message);
}

// Gui yeu cau ghim co phieu.
void StockClient::addFavorite(const QString &symbol)
{
    const QString type = "FAVORITE_ADD";

    if (socket->state() != QAbstractSocket::ConnectedState) {
        emit favoriteActionResult(false, type, "Chua ket noi Server.");
        return;
    }

    QJsonObject request;
    request["type"] = type;
    request["symbol"] = symbol;

    QByteArray message =
        QJsonDocument(request).toJson(QJsonDocument::Compact);
    message.append('\n');

    socket->write(message);
}

// Gui yeu cau bo ghim co phieu.
void StockClient::removeFavorite(const QString &symbol)
{
    const QString type = "FAVORITE_REMOVE";

    if (socket->state() != QAbstractSocket::ConnectedState) {
        emit favoriteActionResult(false, type, "Chua ket noi Server.");
        return;
    }

    QJsonObject request;
    request["type"] = type;
    request["symbol"] = symbol;

    QByteArray message =
        QJsonDocument(request).toJson(QJsonDocument::Compact);
    message.append('\n');

    socket->write(message);
}

// Chuyen mot doi tuong JSON thanh StockQuote.
// timestamp mac dinh duoc lay tu SNAPSHOT neu quote khong co timestamp rieng.
bool StockClient::parseQuote(const QJsonObject &object,
                             const QString &defaultTimestamp,
                             StockQuote &quote)
{
    QString symbol = object.value("symbol").toString().trimmed();
    QJsonValue priceValue = object.value("price");

    if (symbol.isEmpty() || !priceValue.isDouble()) {
        return false;
    }

    double price = priceValue.toDouble();

    if (price <= 0.0) {
        return false;
    }

    QString timestamp = object.value("timestamp").toString();

    if (timestamp.isEmpty()) {
        timestamp = defaultTimestamp;
    }

    quote.symbol = symbol;
    quote.price = price;
    quote.timestamp = timestamp;

    return true;
}

// Xu ly SNAPSHOT duoc Server gui ngay sau khi Client ket noi.
void StockClient::handleSnapshot(const QJsonObject &object)
{
    QJsonValue pricesValue = object.value("prices");

    if (!pricesValue.isArray()) {
        qWarning() << "SNAPSHOT khong co mang prices hop le.";
        return;
    }

    // Server gui timestamp chung o cap SNAPSHOT.
    QString snapshotTimestamp =
        object.value("timestamp").toString();

    QJsonArray pricesArray = pricesValue.toArray();
    QList<StockQuote> quotes;

    for (const QJsonValue &value : pricesArray) {
        if (!value.isObject()) {
            continue;
        }

        StockQuote quote;

        if (parseQuote(value.toObject(),
                       snapshotTimestamp,
                       quote)) {
            quotes.append(quote);
        } else {
            qWarning() << "Bo qua du lieu co phieu khong hop le trong SNAPSHOT:"
                       << value;
        }
    }

    emit snapshotReceived(quotes);
}

void StockClient::readData()
{
    buffer.append(socket->readAll());

    // TCP khong bao dam moi lan doc duoc mot thong diep day du.
    // Chi xu ly khi da nhan du ky tu xuong dong.
    while (true) {
        int newlineIndex = buffer.indexOf('\n');

        if (newlineIndex < 0) {
            break;
        }

        QByteArray line = buffer.left(newlineIndex).trimmed();
        buffer.remove(0, newlineIndex + 1);

        if (line.isEmpty()) {
            continue;
        }

        QJsonParseError parseError;
        QJsonDocument document =
            QJsonDocument::fromJson(line, &parseError);

        if (parseError.error != QJsonParseError::NoError
            || !document.isObject()) {
            qWarning() << "Thong diep JSON khong hop le:"
                       << line;
            continue;
        }

        QJsonObject object = document.object();
        QString type = object.value("type").toString();

        if (type == "REGISTER_RESPONSE") {
            bool success = object.value("success").toBool();
            QString message = object.value("message").toString();

            emit registrationResult(success, message);
            continue;
        }

        if (type == "LOGIN_RESPONSE") {
            bool success = object.value("success").toBool();
            QString message = object.value("message").toString();
            QString role = object.value("role").toString();

            emit loginResult(success, message, role);
            continue;
        }

        if (type == "ADMIN_LIST_RESPONSE") {
            bool success = object.value("success").toBool();
            QString message = object.value("message").toString();
            QList<AccountInfo> accounts;

            if (success && object.value("accounts").isArray()) {
                QJsonArray array = object.value("accounts").toArray();

                for (const QJsonValue &value : array) {
                    if (!value.isObject())
                        continue;

                    QJsonObject accountObject = value.toObject();

                    AccountInfo account;
                    account.id = accountObject.value("id").toInt();
                    account.username =
                        accountObject.value("username").toString();
                    account.role =
                        accountObject.value("role").toString();
                    account.createdAt =
                        accountObject.value("created_at").toString();

                    accounts.append(account);
                }
            }

            emit adminAccountsResult(success, accounts, message);
            continue;
        }

        if (type == "ADMIN_ROLE_RESPONSE") {
            emit adminRoleUpdateResult(
                object.value("success").toBool(),
                object.value("message").toString());
            continue;
        }

        if (type == "STOCK_LIST_RESPONSE") {
            const bool success = object.value("success").toBool();
            const QString message = object.value("message").toString();
            QList<StockInfo> stocks;

            if (success && object.value("stocks").isArray()) {
                const QJsonArray array = object.value("stocks").toArray();

                for (const QJsonValue &value : array) {
                    if (!value.isObject())
                        continue;

                    const QJsonObject item = value.toObject();
                    StockInfo stock;
                    stock.symbol = item.value("symbol").toString();
                    stock.name = item.value("name").toString();
                    stock.displayOrder =
                        item.value("display_order").toInt();
                    stocks.append(stock);
                }
            }

            emit stockListReceived(success, stocks, message);
            continue;
        }

        if (type == "STOCK_HISTORY_RESPONSE") {
            const bool success = object.value("success").toBool();
            const QString symbol = object.value("symbol").toString();
            const QString message = object.value("message").toString();
            QList<HistoryPoint> points;

            if (success && object.value("points").isArray()) {
                const QJsonArray array = object.value("points").toArray();

                for (const QJsonValue &value : array) {
                    if (!value.isObject())
                        continue;

                    const QJsonObject item = value.toObject();
                    HistoryPoint point;
                    point.timestamp = item.value("timestamp").toString();
                    point.price = item.value("price").toDouble();

                    if (!point.timestamp.isEmpty() && point.price > 0)
                        points.append(point);
                }
            }

            emit stockHistoryReceived(success, symbol, points, message);
            continue;
        }

        if (type == "ADMIN_STOCK_RESPONSE") {
            emit adminStockResult(
                object.value("success").toBool(),
                object.value("message").toString());
            continue;
        }
        if (type == "FAVORITES_RESPONSE") {
            const bool success = object.value("success").toBool();
            const QString message = object.value("message").toString();
            QList<FavoriteInfo> favorites;

            if (success && object.value("favorites").isArray()) {
                const QJsonArray array =
                    object.value("favorites").toArray();

                for (const QJsonValue &value : array) {
                    if (!value.isObject())
                        continue;

                    const QJsonObject item = value.toObject();

                    FavoriteInfo favorite;
                    favorite.symbol = item.value("symbol").toString();
                    favorite.name = item.value("name").toString();
                    favorite.displayOrder =
                        item.value("display_order").toInt();
                    favorite.createdAt =
                        item.value("created_at").toString();

                    favorites.append(favorite);
                }
            }

            emit favoritesReceived(success, favorites, message);
            continue;
        }

        if (type == "FAVORITE_ADD_RESPONSE"
            || type == "FAVORITE_REMOVE_RESPONSE") {
            emit favoriteActionResult(
                object.value("success").toBool(),
                type,
                object.value("message").toString());
            continue;
        }

        if (type == "SNAPSHOT") {
            handleSnapshot(object);
            continue;
        }

        if (type == "PRICE_UPDATE") {
            StockQuote quote;

            if (!parseQuote(object, QString(), quote)) {
                qWarning() << "Du lieu PRICE_UPDATE khong hop le:"
                           << line;
                continue;
            }

            emit priceReceived(
                quote.symbol,
                quote.price,
                quote.timestamp);
            continue;
        }

        qWarning() << "Loai thong diep khong duoc ho tro:" << type;
    }
}
