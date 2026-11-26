#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "stockclient.h"

class QLabel;
class QPushButton;
class QTableWidget;
class QLineEdit;
class QSpinBox;
class QGroupBox;
class QComboBox;
class QChartView;
class QChart;
class QLineSeries;
class QDateTimeAxis;
class QValueAxis;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    StockClient client;

    QLabel *statusLabel;

    QLineEdit *serverHostInput;
    QSpinBox *serverPortInput;

    QPushButton *connectButton;
    QPushButton *disconnectButton;

    QTableWidget *priceTable;

    QGroupBox *favoritesGroup;
    QTableWidget *favoritesTable;
    QPushButton *addFavoriteButton;
    QPushButton *removeFavoriteButton;

    QLineEdit *usernameInput;
    QLineEdit *passwordInput;
    QPushButton *registerButton;
    QLabel *registrationLabel;
    QPushButton *loginButton;

    QGroupBox *adminGroup;
    QTableWidget *accountTable;
    QPushButton *refreshAccountsButton;
    QPushButton *changeRoleButton;

    QGroupBox *adminStockGroup;
    QLineEdit *stockSymbolInput;
    QLineEdit *stockNameInput;
    QTableWidget *stockTable;
    QPushButton *addStockButton;
    QPushButton *removeStockButton;
    QPushButton *moveStockUpButton;
    QPushButton *moveStockDownButton;
    QPushButton *saveStockOrderButton;

    QGroupBox *chartGroup;
    QComboBox *chartSymbolCombo;
    QComboBox *chartRangeCombo;
    QPushButton *loadHistoryButton;
    QChartView *chartView;
    QChart *historyChart;
    QLineSeries *historySeries;
    QDateTimeAxis *historyTimeAxis;
    QValueAxis *historyPriceAxis;

    void updatePrice(const QString &symbol,
                     double price,
                     const QString &timestamp);

    int findOrAddRow(const QString &symbol);

    void applySnapshot(const QList<StockQuote> &quotes);

    void applyStockList(bool success,
                        const QList<StockInfo> &stocks,
                        const QString &message);

    void applyHistory(bool success,
                      const QString &symbol,
                      const QList<HistoryPoint> &points,
                      const QString &message);

    void applyFavorites(bool success,
                        const QList<FavoriteInfo> &favorites,
                        const QString &message);

    void applyAdminAccounts(bool success,
                            const QList<AccountInfo> &accounts,
                            const QString &message);
};

#endif // MAINWINDOW_H
