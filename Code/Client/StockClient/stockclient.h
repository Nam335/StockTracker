#ifndef STOCKCLIENT_H
#define STOCKCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QByteArray>
#include <QString>
#include <QTimer>
#include <QJsonObject>
#include <QList>

struct StockInfo
{
    QString symbol;
    QString name;
    int displayOrder = 0;
};

struct HistoryPoint
{
    QString timestamp;
    double price = 0.0;
};

struct FavoriteInfo
{
    QString symbol;
    QString name;
    int displayOrder = 0;
    QString createdAt;
};

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

    void requestStockList();
    void requestStockHistory(const QString &symbol, const QString &range);
    void addStock(const QString &symbol, const QString &name);
    void removeStock(const QString &symbol);
    void reorderStocks(const QStringList &symbols);

    void requestFavorites();
    void addFavorite(const QString &symbol);
    void removeFavorite(const QString &symbol);
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

    void stockListReceived(bool success,
                           const QList<StockInfo> &stocks,
                           const QString &message);

    void stockHistoryReceived(bool success,
                              const QString &symbol,
                              const QList<HistoryPoint> &points,
                              const QString &message);

    void adminStockResult(bool success, const QString &message);

    void favoritesReceived(bool success,
                           const QList<FavoriteInfo> &favorites,
                           const QString &message);

    void favoriteActionResult(bool success,
                              const QString &type,
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
