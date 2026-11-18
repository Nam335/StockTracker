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
                // Ket noi that bai (vd Server chua bat) cung thu lai.
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

    // Dang ket noi do hoac dang cho thu lai: khong co ket noi that
    // nen khong co tin hieu disconnected, tu bao trang thai.
    socket->abort();
    emit statusChanged("Da ngat ket noi.");
}

// Hen gio thu ket noi lai neu nguoi dung chua bam Ngat ket noi.
// Goi nhieu lan van chi co mot lan hen gio.
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
        const QString type = object.value("type").toString();

        if (type == "SNAPSHOT") {
            handleSnapshot(object);
            continue;
        }

        if (type != "PRICE_UPDATE") {
            continue;
        }

        StockQuote quote;

        if (!parseQuote(object, QString(), quote)) {
            qWarning() << "Du lieu gia co phieu khong hop le:"
                       << line;
            continue;
        }

        emit priceReceived(quote.symbol, quote.price,
                           quote.timestamp);
    }
}

// Doc mot ban ghi gia (dung chung cho PRICE_UPDATE va tung phan
// tu cua SNAPSHOT). Tra ve false neu thieu ma, gia sai kieu hoac <= 0.
bool StockClient::parseQuote(const QJsonObject &object,
                             const QString &defaultTimestamp,
                             StockQuote &quote)
{
    const QString symbol = object.value("symbol").toString();
    const QJsonValue priceValue = object.value("price");

    if (symbol.isEmpty() || !priceValue.isDouble()) {
        return false;
    }

    const double price = priceValue.toDouble();

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

void StockClient::handleSnapshot(const QJsonObject &object)
{
    const QJsonValue pricesValue = object.value("prices");

    if (!pricesValue.isArray()) {
        qWarning() << "SNAPSHOT thieu mang \"prices\".";
        return;
    }

    // Moi ban ghi co the khong co timestamp rieng: dung timestamp
    // chung cua SNAPSHOT.
    const QString snapshotTimestamp =
        object.value("timestamp").toString();

    QList<StockQuote> quotes;

    const QJsonArray array = pricesValue.toArray();

    for (const QJsonValue &value : array) {
        StockQuote quote;

        if (!value.isObject()
            || !parseQuote(value.toObject(),
                           snapshotTimestamp, quote)) {
            qWarning() << "Phan tu SNAPSHOT khong hop le, bo qua.";
            continue;
        }

        quotes.append(quote);
    }

    emit snapshotReceived(quotes);
}