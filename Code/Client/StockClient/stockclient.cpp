#include "stockclient.h"

#include <QJsonDocument>
#include <QJsonObject>
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

        if (object.value("type").toString()
            != "PRICE_UPDATE") {
            continue;
        }

        QString symbol =
            object.value("symbol").toString();

        QJsonValue priceValue = object.value("price");

        if (symbol.isEmpty() || !priceValue.isDouble()) {
            qWarning() << "Du lieu gia co phieu khong hop le:"
                       << line;
            continue;
        }

        double price = priceValue.toDouble();

        if (price <= 0.0) {
            qWarning() << "Gia co phieu khong hop le:" << price;
            continue;
        }

        QString timestamp =
            object.value("timestamp").toString();

        emit priceReceived(symbol, price, timestamp);
    }
}