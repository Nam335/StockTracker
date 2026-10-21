
#include "mainwindow.h"

#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QDateTime>
#include <QAbstractItemView>
#include <QStringList>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    statusLabel(new QLabel("Chua ket noi")),
    connectButton(new QPushButton("Ket noi")),
    disconnectButton(new QPushButton("Ngat ket noi")),
    priceTable(new QTableWidget(5, 3))
{
    setWindowTitle("Stock Tracker - Client");
    resize(700, 400);

    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    buttonLayout->addWidget(connectButton);
    buttonLayout->addWidget(disconnectButton);

    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(statusLabel);
    mainLayout->addWidget(priceTable);

    priceTable->setHorizontalHeaderLabels(
        {"Ma co phieu", "Gia", "Thoi gian"});

    priceTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers);

    priceTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Stretch);

    const QStringList symbols = {
        "FPT", "VCB", "VNM", "HPG", "MWG"
    };

    for (int row = 0; row < symbols.size(); ++row) {
        priceTable->setItem(
            row, 0, new QTableWidgetItem(symbols[row]));
        priceTable->setItem(
            row, 1, new QTableWidgetItem("--"));
        priceTable->setItem(
            row, 2, new QTableWidgetItem("--"));
    }

    disconnectButton->setEnabled(false);

    connect(connectButton, &QPushButton::clicked,
            this, [this]() {
                client.connectToServer("127.0.0.1", 5000);
            });

    connect(disconnectButton, &QPushButton::clicked,
            &client, &StockClient::disconnectFromServer);

    connect(&client, &StockClient::statusChanged,
            this, [this](const QString &status) {
                statusLabel->setText(status);

                bool connected =
                    status.startsWith("Da ket noi den StockServer.");

                connectButton->setEnabled(!connected);
                disconnectButton->setEnabled(connected);
            });

    connect(&client, &StockClient::priceReceived,
            this, &MainWindow::updatePrice);
}

MainWindow::~MainWindow() = default;

void MainWindow::updatePrice(
    const QString &symbol,
    double price,
    const QString &timestamp)
{
    for (int row = 0; row < priceTable->rowCount(); ++row) {
        if (priceTable->item(row, 0)->text() == symbol) {
            priceTable->item(row, 1)->setText(
                QString::number(price, 'f', 2));

            priceTable->item(row, 2)->setText(
                QDateTime::fromString(timestamp, Qt::ISODate)
                    .toLocalTime()
                    .toString("HH:mm:ss"));

            return;
        }
    }
}
