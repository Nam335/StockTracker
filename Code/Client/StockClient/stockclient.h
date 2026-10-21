
#ifndef STOCKCLIENT_H
#define STOCKCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QByteArray>
#include <QString>

class StockClient : public QObject
{
    Q_OBJECT

public:
    explicit StockClient(QObject *parent = nullptr);

    void connectToServer(const QString &host, quint16 port);
    void disconnectFromServer();

signals:
    void statusChanged(const QString &status);
    void priceReceived(const QString &symbol,
                       double price,
                       const QString &timestamp);

private slots:
    void readData();

private:
    QTcpSocket *socket;
    QByteArray buffer;
};

#endif // STOCKCLIENT_H
