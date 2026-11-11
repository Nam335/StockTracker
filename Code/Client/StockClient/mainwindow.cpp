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
#include <QColor>
#include <QVariant>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    statusLabel(new QLabel("Chua ket noi")),
    connectButton(new QPushButton("Ket noi")),
    disconnectButton(new QPushButton("Ngat ket noi")),
    priceTable(new QTableWidget(5, 4))
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
        {"Ma co phieu", "Gia", "Thay doi", "Thoi gian"});

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
        priceTable->setItem(
            row, 3, new QTableWidgetItem("--"));
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
        if (priceTable->item(row, 0)->text() != symbol) {
            continue;
        }

        QTableWidgetItem *priceItem = priceTable->item(row, 1);
        QTableWidgetItem *changeItem = priceTable->item(row, 2);
        QTableWidgetItem *timeItem = priceTable->item(row, 3);

        // Gia nhan duoc o lan truoc, duoc cat kem trong o gia.
        // Lan dau chua co gia cu nen QVariant khong hop le.
        const QVariant oldValue = priceItem->data(Qt::UserRole);

        priceItem->setText(QString::number(price, 'f', 2));
        priceItem->setData(Qt::UserRole, price);

        timeItem->setText(
            QDateTime::fromString(timestamp, Qt::ISODate)
                .toLocalTime()
                .toString("HH:mm:ss"));

        if (!oldValue.isValid()) {
            // Lan dau nhan gia cua ma nay: chua co gi de so sanh.
            changeItem->setText("--");
            return;
        }

        const double oldPrice = oldValue.toDouble();
        const double change = price - oldPrice;
        const double percent = change / oldPrice * 100.0;

        // Chenh lech nho hon 0.005 se hien thi 0.00, coi nhu khong doi.
        if (qAbs(change) < 0.005) {
            changeItem->setText("0.00 (0.00%)");
            changeItem->setForeground(QBrush());
            priceItem->setForeground(QBrush());
            return;
        }

        const QString sign = change > 0 ? "+" : "";
        changeItem->setText(
            sign + QString::number(change, 'f', 2)
            + " (" + sign + QString::number(percent, 'f', 2) + "%)");

        const QColor color = change > 0
                                 ? QColor(0x1B, 0x8A, 0x3A)   // xanh: gia tang
                                 : QColor(0xD3, 0x2F, 0x2F);  // do: gia giam
        changeItem->setForeground(color);
        priceItem->setForeground(color);
        return;
    }
}