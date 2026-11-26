#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "stockclient.h"

class QLabel;
class QPushButton;
class QTableWidget;
class QLineEdit;
class QSpinBox;

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

    QLineEdit *usernameInput;
    QLineEdit *passwordInput;
    QPushButton *registerButton;
    QLabel *registrationLabel;
    QPushButton *loginButton;

    void updatePrice(const QString &symbol,
                     double price,
                     const QString &timestamp);

    void applySnapshot(const QList<StockQuote> &quotes);
};

#endif // MAINWINDOW_H
