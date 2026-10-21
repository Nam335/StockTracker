
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "stockclient.h"

class QLabel;
class QPushButton;
class QTableWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    StockClient client;

    QLabel *statusLabel;
    QPushButton *connectButton;
    QPushButton *disconnectButton;
    QTableWidget *priceTable;

    void updatePrice(const QString &symbol,
                     double price,
                     const QString &timestamp);
};

#endif // MAINWINDOW_H
