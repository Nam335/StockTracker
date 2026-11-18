#include "mainwindow.h"
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
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
    priceTable(new QTableWidget(5, 4)),
    hostEdit(new QLineEdit("127.0.0.1")),
    portSpin(new QSpinBox())
{
    setWindowTitle("Stock Tracker - Client");
    resize(700, 400);

    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    QHBoxLayout *inputLayout = new QHBoxLayout();
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    // O nhap IP va cong Server (mac dinh 127.0.0.1:5000)
    hostEdit->setPlaceholderText("vd: 192.168.1.10");
    portSpin->setRange(1, 65535);
    portSpin->setValue(5000);

    inputLayout->addWidget(new QLabel("IP Server:"));
    inputLayout->addWidget(hostEdit, 1);
    inputLayout->addWidget(new QLabel("Cong:"));
    inputLayout->addWidget(portSpin);

    buttonLayout->addWidget(connectButton);
    buttonLayout->addWidget(disconnectButton);

    mainLayout->addLayout(inputLayout);
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
                const QString host = hostEdit->text().trimmed();

                if (host.isEmpty()) {
                    statusLabel->setText("Vui long nhap IP Server.");
                    hostEdit->setFocus();
                    return;
                }

                // Xoa du lieu cu cua lan ket noi truoc de bang gia
                // chi hien du lieu moi tu snapshot.
                resetTable();

                client.connectToServer(
                    host, static_cast<quint16>(portSpin->value()));
            });

    // Bam Enter trong o IP cung la bam Ket noi.
    connect(hostEdit, &QLineEdit::returnPressed,
            connectButton, &QPushButton::click);

    connect(disconnectButton, &QPushButton::clicked,
            &client, &StockClient::disconnectFromServer);

    connect(&client, &StockClient::statusChanged,
            this, [this](const QString &status) {
                statusLabel->setText(status);

                // Khi dang ket noi / cho thu lai van cho bam Ngat ket noi
                // de huy viec tu ket noi lai.
                const bool active = client.isActive();

                connectButton->setEnabled(!active);
                disconnectButton->setEnabled(active);

                // Khong cho doi IP/cong khi dang ket noi.
                hostEdit->setEnabled(!active);
                portSpin->setEnabled(!active);
            });

    connect(&client, &StockClient::priceReceived,
            this, &MainWindow::updatePrice);

    connect(&client, &StockClient::snapshotReceived,
            this, &MainWindow::applySnapshot);
}

MainWindow::~MainWindow() = default;

// Doi timestamp ISO (UTC) thanh gio may "HH:mm:ss"; "--" neu rong/sai.
static QString formatTime(const QString &timestamp)
{
    const QDateTime time =
        QDateTime::fromString(timestamp, Qt::ISODate);

    if (!time.isValid()) {
        return "--";
    }

    return time.toLocalTime().toString("HH:mm:ss");
}

// Dua tat ca o ve trang thai ban dau ("--"), bo gia cu va mau.
void MainWindow::resetTable()
{
    for (int row = 0; row < priceTable->rowCount(); ++row) {
        for (int col = 1; col < priceTable->columnCount(); ++col) {
            QTableWidgetItem *item = priceTable->item(row, col);
            item->setText("--");
            item->setForeground(QBrush());
        }

        priceTable->item(row, 1)->setData(Qt::UserRole, QVariant());
    }
}

// Snapshot la bang gia ban dau: hien ngay gia + thoi gian cua cac ma.
// Day chi la moc ban dau, chua co gia truoc do de so sanh nen cot
// "Thay doi" de "--" va khong to mau; gia nay tro thanh gia goc
// cho cac PRICE_UPDATE ke tiep.
void MainWindow::applySnapshot(const QList<StockQuote> &quotes)
{
    for (const StockQuote &quote : quotes) {
        for (int row = 0; row < priceTable->rowCount(); ++row) {
            if (priceTable->item(row, 0)->text() != quote.symbol) {
                continue;
            }

            QTableWidgetItem *priceItem = priceTable->item(row, 1);
            QTableWidgetItem *changeItem = priceTable->item(row, 2);
            QTableWidgetItem *timeItem = priceTable->item(row, 3);

            priceItem->setText(QString::number(quote.price, 'f', 2));
            priceItem->setData(Qt::UserRole, quote.price);
            priceItem->setForeground(QBrush());

            changeItem->setText("--");
            changeItem->setForeground(QBrush());

            timeItem->setText(formatTime(quote.timestamp));
            break;
        }
    }
}

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