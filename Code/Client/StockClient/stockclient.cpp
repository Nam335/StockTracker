
#include "stockclient.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QAbstractSocket>
#include <QDebug>

StockClient::StockClient(QObject *parent)
    : QObject(parent),
    socket(new QTcpSocket(this))
{
    connect(socket, &QTcpSocket::connected, this, [this]() {
        buffer.clear();
        emit statusChanged("Da ket noi den StockServer.");
    });

    connect(socket, &QTcpSocket::disconnected, this, [this]() {
        buffer.clear();
        emit statusChanged("Da ngat ket noi.");
    });

    connect(socket, &QTcpSocket::readyRead,
            this, &StockClient::readData);

    connect(socket, &QTcpSocket::errorOccurred,
            this, [this](QAbstractSocket::SocketError) {
                emit statusChanged(
                    "Loi TCP: " + socket->errorString());
            });
}

void StockClient::connectToServer(
    const QString &host, quint16 port)
{
    if (socket->state() != QAbstractSocket::UnconnectedState) {
        socket->abort();
    }

    buffer.clear();

    emit statusChanged("Dang ket noi den StockServer...");
    socket->connectToHost(host, port);
}

void StockClient::disconnectFromServer()
{
    socket->disconnectFromHost();
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
