#include "mainwindow.h"

#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QDateTime>
#include <QAbstractItemView>
#include <QStringList>
#include <QColor>
#include <QVariant>
#include <QLineEdit>
#include <QRegularExpression>
#include <QBrush>
#include <QSpinBox>
#include <QGroupBox>
#include <QMessageBox>
#include <QtMath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      statusLabel(new QLabel("Chua ket noi")),
      serverHostInput(new QLineEdit("127.0.0.1")),
      serverPortInput(new QSpinBox),
      connectButton(new QPushButton("Ket noi")),
      disconnectButton(new QPushButton("Ngat ket noi")),
      priceTable(new QTableWidget(5, 4)),
      usernameInput(new QLineEdit),
      passwordInput(new QLineEdit),
      registerButton(new QPushButton("Dang ky")),
      registrationLabel(new QLabel("")),
      loginButton(new QPushButton("Dang nhap")),
      adminGroup(new QGroupBox("Quan tri tai khoan")),
      accountTable(new QTableWidget(0, 4)),
      refreshAccountsButton(new QPushButton("Lam moi danh sach")),
      changeRoleButton(new QPushButton("Doi quyen"))
{
    setWindowTitle("Stock Tracker - Client");
    resize(850, 500);

    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    // Cau hinh dia chi va cong Server.
    serverHostInput->setPlaceholderText("Dia chi IP Server");
    serverPortInput->setRange(1, 65535);
    serverPortInput->setValue(5000);

    // Khu vuc ket noi Server.
    QHBoxLayout *connectionLayout = new QHBoxLayout();

    connectionLayout->addWidget(new QLabel("IP Server:"));
    connectionLayout->addWidget(serverHostInput);
    connectionLayout->addWidget(new QLabel("Cong:"));
    connectionLayout->addWidget(serverPortInput);
    connectionLayout->addWidget(connectButton);
    connectionLayout->addWidget(disconnectButton);

    mainLayout->addLayout(connectionLayout);
    mainLayout->addWidget(statusLabel);

    // Khu vuc dang ky va dang nhap tai khoan.
    QHBoxLayout *registerLayout = new QHBoxLayout();

    usernameInput->setPlaceholderText("Ten dang nhap");
    passwordInput->setPlaceholderText("Mat khau");
    passwordInput->setEchoMode(QLineEdit::Password);

    registerButton->setEnabled(false);
    loginButton->setEnabled(false);

    registerLayout->addWidget(usernameInput);
    registerLayout->addWidget(passwordInput);
    registerLayout->addWidget(registerButton);
    registerLayout->addWidget(loginButton);

    mainLayout->addLayout(registerLayout);
    mainLayout->addWidget(registrationLabel);

    // Bang gia co phieu.
    mainLayout->addWidget(priceTable);

    QVBoxLayout *adminLayout = new QVBoxLayout(adminGroup);

    accountTable->setHorizontalHeaderLabels(
        {"ID", "Ten dang nhap", "Vai tro", "Ngay tao"});

    accountTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers);

    accountTable->setSelectionBehavior(
        QAbstractItemView::SelectRows);

    accountTable->setSelectionMode(
        QAbstractItemView::SingleSelection);

    accountTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Stretch);

    QHBoxLayout *adminButtonLayout = new QHBoxLayout;
    adminButtonLayout->addWidget(refreshAccountsButton);
    adminButtonLayout->addWidget(changeRoleButton);

    adminLayout->addWidget(accountTable);
    adminLayout->addLayout(adminButtonLayout);

    adminGroup->hide();
    mainLayout->addWidget(adminGroup);

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

    // Xu ly nut Ket noi.
    connect(connectButton, &QPushButton::clicked,
            this, [this]() {
                const QString host =
                    serverHostInput->text().trimmed();

                if (host.isEmpty()) {
                    statusLabel->setText(
                        "Vui long nhap dia chi Server.");
                    return;
                }

                const quint16 port =
                    static_cast<quint16>(serverPortInput->value());

                client.connectToServer(host, port);
            });

    // Nhan Enter trong o IP de ket noi.
    connect(serverHostInput, &QLineEdit::returnPressed,
            connectButton, &QPushButton::click);

    // Xu ly nut Ngat ket noi.
    connect(disconnectButton, &QPushButton::clicked,
            &client, &StockClient::disconnectFromServer);

    // Cap nhat trang thai ket noi.
    connect(&client, &StockClient::statusChanged,
            this, [this](const QString &status) {
                statusLabel->setText(status);

                const bool active = client.isActive();

                connectButton->setEnabled(!active);
                disconnectButton->setEnabled(active);

                serverHostInput->setEnabled(!active);
                serverPortInput->setEnabled(!active);

                const bool connected =
                    status == "Da ket noi den StockServer.";

                registerButton->setEnabled(connected);
                loginButton->setEnabled(connected);

                if (!connected) {
                    adminGroup->hide();
                    accountTable->setRowCount(0);
                }
            });

    // Xu ly nut Dang ky.
    connect(registerButton, &QPushButton::clicked,
            this, [this]() {
                const QString username =
                    usernameInput->text().trimmed();

                const QString password =
                    passwordInput->text();

                if (username.isEmpty()) {
                    registrationLabel->setText(
                        "Vui long nhap ten dang nhap.");
                    return;
                }

                if (!QRegularExpression(
                         "^[A-Za-z0-9_]{3,}$")
                         .match(username).hasMatch()) {
                    registrationLabel->setText(
                        "Ten dang nhap phai co it nhat 3 ky tu, "
                        "chi gom chu, so va dau gach duoi.");
                    return;
                }

                if (password.isEmpty()) {
                    registrationLabel->setText(
                        "Vui long nhap mat khau.");
                    return;
                }

                registrationLabel->setText(
                    "Dang gui yeu cau dang ky...");

                client.registerAccount(username, password);
            });

    // Nhan ket qua dang ky tu Server.
    connect(&client, &StockClient::registrationResult,
            this, [this](bool success, const QString &message) {
                registrationLabel->setText(message);

                if (success) {
                    passwordInput->clear();
                }
            });

    // Xu ly nut Dang nhap.
    connect(loginButton, &QPushButton::clicked,
            this, [this]() {
                const QString username =
                    usernameInput->text().trimmed();

                const QString password =
                    passwordInput->text();

                if (username.isEmpty()) {
                    registrationLabel->setText(
                        "Vui long nhap ten dang nhap.");
                    return;
                }

                if (!QRegularExpression(
                         "^[A-Za-z0-9_]{3,}$")
                         .match(username).hasMatch()) {
                    registrationLabel->setText(
                        "Ten dang nhap khong hop le.");
                    return;
                }

                if (password.isEmpty()) {
                    registrationLabel->setText(
                        "Vui long nhap mat khau.");
                    return;
                }

                registrationLabel->setText(
                    "Dang gui yeu cau dang nhap...");

                client.login(username, password);
            });

    // Nhan ket qua dang nhap tu Server.
    connect(&client, &StockClient::loginResult,
            this, [this](bool success,
                         const QString &message,
                         const QString &role) {
                if (success) {
                    registrationLabel->setText(
                        message + " - Quyen: " + role);
                    passwordInput->clear();

                    if (role == "admin") {
                        adminGroup->show();
                        client.requestAdminAccounts();
                    } else {
                        adminGroup->hide();
                        accountTable->setRowCount(0);
                    }
                } else {
                    registrationLabel->setText(message);
                    adminGroup->hide();
                    accountTable->setRowCount(0);
                }
            });

    // Nhan ket qua danh sach tai khoan tu Server.
    connect(&client, &StockClient::adminAccountsResult,
            this, [this](bool success,
                         const QList<AccountInfo> &accounts,
                         const QString &message) {
                applyAdminAccounts(success, accounts, message);
            });

    // Nhan ket qua thay doi quyen tai khoan.
    connect(&client, &StockClient::adminRoleUpdateResult,
            this, [this](bool success, const QString &message) {
                if (success) {
                    QMessageBox::information(
                        this, "Quan tri tai khoan", message);
                    client.requestAdminAccounts();
                } else {
                    QMessageBox::warning(
                        this, "Quan tri tai khoan", message);
                }
            });

    // Lam moi danh sach tai khoan.
    connect(refreshAccountsButton, &QPushButton::clicked,
            this, [this]() {
                client.requestAdminAccounts();
            });

    // Doi quyen tai khoan duoc chon.
    connect(changeRoleButton, &QPushButton::clicked,
            this, [this]() {
                int row = accountTable->currentRow();

                if (row < 0) {
                    QMessageBox::warning(
                        this, "Quan tri tai khoan",
                        "Hay chon mot tai khoan trong danh sach.");
                    return;
                }

                QTableWidgetItem *idItem = accountTable->item(row, 0);
                QTableWidgetItem *usernameItem = accountTable->item(row, 1);
                QTableWidgetItem *roleItem = accountTable->item(row, 2);

                if (!idItem || !usernameItem || !roleItem)
                    return;

                int userId = idItem->data(Qt::UserRole).toInt();
                QString username = usernameItem->text();
                QString currentRole = roleItem->text();

                QString newRole =
                    (currentRole == "admin") ? "user" : "admin";

                QString question =
                    "Ban co chac muon doi quyen tai khoan "
                    + username + " tu " + currentRole
                    + " thanh " + newRole + "?";

                if (QMessageBox::question(
                        this, "Xac nhan doi quyen", question,
                        QMessageBox::Yes | QMessageBox::No,
                        QMessageBox::No) == QMessageBox::Yes) {
                    client.setAccountRole(userId, newRole);
                }
            });

    // Nhan bang gia ban dau tu SNAPSHOT.
    connect(&client, &StockClient::snapshotReceived,
            this, &MainWindow::applySnapshot);

    // Nhan cac cap nhat gia tiep theo.
    connect(&client, &StockClient::priceReceived,
            this, &MainWindow::updatePrice);
}

MainWindow::~MainWindow() = default;


// Nap bang gia ban dau theo SNAPSHOT.
void MainWindow::applySnapshot(const QList<StockQuote> &quotes)
{
    for (const StockQuote &quote : quotes) {
        for (int row = 0; row < priceTable->rowCount(); ++row) {
            if (priceTable->item(row, 0)->text() != quote.symbol) {
                continue;
            }

            QTableWidgetItem *priceItem =
                priceTable->item(row, 1);
            QTableWidgetItem *changeItem =
                priceTable->item(row, 2);
            QTableWidgetItem *timeItem =
                priceTable->item(row, 3);

            priceItem->setText(
                QString::number(quote.price, 'f', 2));
            priceItem->setData(Qt::UserRole, quote.price);

            changeItem->setText("--");

            priceItem->setForeground(QBrush());
            changeItem->setForeground(QBrush());

            const QDateTime dateTime =
                QDateTime::fromString(
                    quote.timestamp, Qt::ISODate);

            if (dateTime.isValid()) {
                timeItem->setText(
                    dateTime.toLocalTime().toString("HH:mm:ss"));
            } else {
                timeItem->setText("--");
            }

            break;
        }
    }
}


// Cap nhat mot gia co phieu theo PRICE_UPDATE.
void MainWindow::updatePrice(
    const QString &symbol,
    double price,
    const QString &timestamp)
{
    for (int row = 0; row < priceTable->rowCount(); ++row) {
        if (priceTable->item(row, 0)->text() != symbol) {
            continue;
        }

        QTableWidgetItem *priceItem =
            priceTable->item(row, 1);
        QTableWidgetItem *changeItem =
            priceTable->item(row, 2);
        QTableWidgetItem *timeItem =
            priceTable->item(row, 3);

        const QVariant oldValue =
            priceItem->data(Qt::UserRole);

        priceItem->setText(
            QString::number(price, 'f', 2));
        priceItem->setData(Qt::UserRole, price);

        const QDateTime dateTime =
            QDateTime::fromString(timestamp, Qt::ISODate);

        if (dateTime.isValid()) {
            timeItem->setText(
                dateTime.toLocalTime().toString("HH:mm:ss"));
        } else {
            timeItem->setText("--");
        }

        if (!oldValue.isValid()) {
            changeItem->setText("--");
            return;
        }

        const double oldPrice = oldValue.toDouble();

        if (oldPrice <= 0.0) {
            changeItem->setText("--");
            return;
        }

        const double change = price - oldPrice;
        const double percent = change / oldPrice * 100.0;

        if (qAbs(change) < 0.005) {
            changeItem->setText("0.00 (0.00%)");
            changeItem->setForeground(QBrush());
            priceItem->setForeground(QBrush());
            return;
        }

        const QString sign = change > 0 ? "+" : "";

        changeItem->setText(
            sign + QString::number(change, 'f', 2)
            + " (" + sign
            + QString::number(percent, 'f', 2) + "%)");

        const QColor color = change > 0
                                 ? QColor(0x1B, 0x8A, 0x3A)
                                 : QColor(0xD3, 0x2F, 0x2F);

        changeItem->setForeground(color);
        priceItem->setForeground(color);

        return;
    }
}

void MainWindow::applyAdminAccounts(
    bool success,
    const QList<AccountInfo> &accounts,
    const QString &message)
{
    if (!success) {
        QMessageBox::warning(
            this, "Quan tri tai khoan",
            message.isEmpty()
                ? "Khong lay duoc danh sach tai khoan."
                : message);
        return;
    }

    accountTable->setRowCount(0);

    for (const AccountInfo &account : accounts) {
        int row = accountTable->rowCount();
        accountTable->insertRow(row);

        QTableWidgetItem *idItem =
            new QTableWidgetItem(QString::number(account.id));

        idItem->setData(Qt::UserRole, account.id);

        accountTable->setItem(row, 0, idItem);
        accountTable->setItem(
            row, 1, new QTableWidgetItem(account.username));
        accountTable->setItem(
            row, 2, new QTableWidgetItem(account.role));
        accountTable->setItem(
            row, 3, new QTableWidgetItem(account.createdAt));
    }
}