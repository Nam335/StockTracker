#ifndef STOCKCLIENT_H
#define STOCKCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QByteArray>
#include <QString>
#include <QTimer>

class StockClient : public QObject
{
    Q_OBJECT

public:
    explicit StockClient(QObject *parent = nullptr);

    void connectToServer(const QString &host, quint16 port);
    void disconnectFromServer();

    // true tu luc nguoi dung bam Ket noi den luc bam Ngat ket noi,
    // ke ca khi dang cho tu dong ket noi lai.
    bool isActive() const { return autoReconnect; }

signals:
    void statusChanged(const QString &status);
    void priceReceived(const QString &symbol,
                       double price,
                       const QString &timestamp);

private slots:
    void readData();

private:
    void scheduleReconnect();

    QTcpSocket *socket;
    QByteArray buffer;

    QTimer *reconnectTimer;
    QString serverHost;
    quint16 serverPort = 0;
    bool autoReconnect = false;
};

#endif // STOCKCLIENT_H