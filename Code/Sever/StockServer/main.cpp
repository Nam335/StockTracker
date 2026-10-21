#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QTimer>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>
#include <QRandomGenerator>
#include <QAbstractSocket>
#include <QDebug>
#include <QList>
#include <QStringList>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QTcpServer server;
    QList<QTcpSocket*> clients;

    // Mo cong TCP 5000
    if (!server.listen(QHostAddress::AnyIPv4, 5000)) {
        qCritical() << "Khong mo duoc port 5000:"
                    << server.errorString();
        return 1;
    }

    qInfo() << "StockServer dang lang nghe port 5000.";

    // Chap nhan cac Client moi
    QObject::connect(&server, &QTcpServer::newConnection, [&]() {
        while (server.hasPendingConnections()) {
            QTcpSocket *socket = server.nextPendingConnection();
            clients.append(socket);

            qInfo() << "Client ket noi:"
                    << socket->peerAddress().toString();

            QObject::connect(socket, &QTcpSocket::disconnected,
                             [&clients, socket]() {
                                 clients.removeAll(socket);
                                 qInfo() << "Client da ngat ket noi.";
                                 socket->deleteLater();
                             });
        }
    });

    // Danh sach ma co phieu mo phong
    const QStringList symbols = {
        "FPT", "VCB", "VNM", "HPG", "MWG"
    };

    // Phat du lieu moi giay
    QTimer timer;

    QObject::connect(&timer, &QTimer::timeout, [&]() {
        for (const QString &symbol : symbols) {
            double price =
                50.0
                + QRandomGenerator::global()->bounded(10000) / 100.0;

            QJsonObject message;
            message["type"] = "PRICE_UPDATE";
            message["symbol"] = symbol;
            message["price"] = price;
            message["timestamp"] =
                QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

            QByteArray data =
                QJsonDocument(message).toJson(QJsonDocument::Compact);

            // Moi thong diep ket thuc bang newline
            data.append('\n');

            for (QTcpSocket *client : clients) {
                if (client->state() == QAbstractSocket::ConnectedState) {
                    client->write(data);
                }
            }

            qInfo().noquote() << "Gia mo phong:"
                              << symbol << price;
        }

        qInfo() << "So Client dang ket noi:" << clients.size();
    });

    timer.start(1000);

    return app.exec();
}