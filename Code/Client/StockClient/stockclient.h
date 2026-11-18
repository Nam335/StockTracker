#ifndef STOCKCLIENT_H
#define STOCKCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QByteArray>
#include <QString>
#include <QTimer>
#include <QJsonObject>
#include <QList>

// Mot ban ghi gia cua mot ma co phieu.
struct StockQuote
{
    QString symbol;
    double price = 0.0;
    QString timestamp;
};

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

    // Bang gia ban dau Server gui ngay khi Client vua ket noi:
    // {"type":"SNAPSHOT","timestamp":"...","prices":[
    //     {"symbol":"FPT","price":100.5}, ...]}
    // Chi gom cac ban ghi hop le (da loai ma rong, gia <= 0).
    void snapshotReceived(const QList<StockQuote> &quotes);

private slots:
    void readData();

private:
    void scheduleReconnect();
    static bool parseQuote(const QJsonObject &object,
                           const QString &defaultTimestamp,
                           StockQuote &quote);
    void handleSnapshot(const QJsonObject &object);

    QTcpSocket *socket;
    QByteArray buffer;

    QTimer *reconnectTimer;
    QString serverHost;
    quint16 serverPort = 0;
    bool autoReconnect = false;
};

#endif // STOCKCLIENT_H