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
