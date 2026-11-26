#ifndef STOCKCLIENT_H
#define STOCKCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QByteArray>
#include <QString>
#include <QTimer>
#include <QJsonObject>
#include <QList>

struct StockQuote
{
    QString symbol;
    double price = 0.0;
    QString timestamp;
};

struct AccountInfo
{
    int id = 0;
    QString username;
    QString role;
    QString createdAt;
};

class StockClient : public QObject
{
    Q_OBJECT

public:
    explicit StockClient(QObject *parent = nullptr);

    void connectToServer(const QString &host, quint16 port);
    void disconnectFromServer();

    void registerAccount(const QString &username,
                         const QString &password);

    void login(const QString &username,
               const QString &password);

    void requestAdminAccounts();
    void setAccountRole(int userId, const QString &role);

    bool isActive() const { return autoReconnect; }

signals:
    void statusChanged(const QString &status);

    void priceReceived(const QString &symbol,
                       double price,
                       const QString &timestamp);

    void registrationResult(bool success,
                            const QString &message);

    void loginResult(bool success,
                     const QString &message,
                     const QString &role);

    void snapshotReceived(const QList<StockQuote> &quotes);

    void adminAccountsResult(bool success,
                             const QList<AccountInfo> &accounts,
                             const QString &message);

    void adminRoleUpdateResult(bool success,
                               const QString &message);

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
