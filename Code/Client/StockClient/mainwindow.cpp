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
#include <QtCharts/QLegend>
#include <QtCharts/QValueAxis>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QLineSeries>
#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <limits>
#include <QInputDialog>
#include <QComboBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      statusLabel(new QLabel("Chua ket noi")),
      serverHostInput(new QLineEdit("127.0.0.1")),
      serverPortInput(new QSpinBox),
      connectButton(new QPushButton("Ket noi")),
      disconnectButton(new QPushButton("Ngat ket noi")),
      priceTable(new QTableWidget(5, 4)),
      favoritesGroup(new QGroupBox("Danh sach yeu thich")),
      favoritesTable(new QTableWidget(0, 3)),
      addFavoriteButton(new QPushButton("Ghim ma dang chon")),
      removeFavoriteButton(new QPushButton("Bo ghim ma dang chon")),
      usernameInput(new QLineEdit),
      passwordInput(new QLineEdit),
      registerButton(new QPushButton("Dang ky")),
      registrationLabel(new QLabel("")),
      loginButton(new QPushButton("Dang nhap")),
      adminGroup(new QGroupBox("Quan tri tai khoan")),
      accountTable(new QTableWidget(0, 4)),
      refreshAccountsButton(new QPushButton("Lam moi danh sach")),
      changeRoleButton(new QPushButton("Doi quyen")),
      adminStockGroup(new QGroupBox("Quan tri danh muc co phieu")),
      stockSymbolInput(new QLineEdit),
      stockNameInput(new QLineEdit),
      stockTable(new QTableWidget(0, 3)),
      addStockButton(new QPushButton("Them ma")),
      removeStockButton(new QPushButton("Xoa ma")),
      moveStockUpButton(new QPushButton("Len")),
      moveStockDownButton(new QPushButton("Xuong")),
      saveStockOrderButton(new QPushButton("Luu thu tu")),
      chartGroup(new QGroupBox("Bieu do lich su gia")),
      chartSymbolCombo(new QComboBox),
      chartRangeCombo(new QComboBox),
      loadHistoryButton(new QPushButton("Tai lich su")),
      chartView(new QChartView),
      historyChart(new QChart),
      historySeries(new QLineSeries),
      historyTimeAxis(new QDateTimeAxis),
      historyPriceAxis(new QValueAxis)
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



    // Giao dien danh sach ma co phieu yeu thich.
    QVBoxLayout *favoritesLayout = new QVBoxLayout(favoritesGroup);

    priceTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    priceTable->setSelectionMode(QAbstractItemView::SingleSelection);

    favoritesTable->setHorizontalHeaderLabels(
        {"Ma co phieu", "Ten cong ty", "Thoi diem ghim"});
    favoritesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    favoritesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    favoritesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    favoritesTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Stretch);

    QHBoxLayout *favoritesButtonLayout = new QHBoxLayout;
    favoritesButtonLayout->addWidget(addFavoriteButton);
    favoritesButtonLayout->addWidget(removeFavoriteButton);

    favoritesLayout->addWidget(favoritesTable);
    favoritesLayout->addLayout(favoritesButtonLayout);

    favoritesGroup->hide();
    mainLayout->addWidget(favoritesGroup);

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

    // Khu vuc quan tri danh muc co phieu.
    QVBoxLayout *stockAdminLayout = new QVBoxLayout(adminStockGroup);

    QHBoxLayout *stockInputLayout = new QHBoxLayout;
    stockSymbolInput->setPlaceholderText("Ma co phieu");
    stockNameInput->setPlaceholderText("Ten cong ty");
    stockInputLayout->addWidget(stockSymbolInput);
    stockInputLayout->addWidget(stockNameInput);
    stockInputLayout->addWidget(addStockButton);

    stockTable->setHorizontalHeaderLabels(
        {"Ma co phieu", "Ten cong ty", "Thu tu"});
    stockTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    stockTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    stockTable->setSelectionMode(QAbstractItemView::SingleSelection);
    stockTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Stretch);

    QHBoxLayout *stockButtonLayout = new QHBoxLayout;
    stockButtonLayout->addWidget(removeStockButton);
    stockButtonLayout->addWidget(moveStockUpButton);
    stockButtonLayout->addWidget(moveStockDownButton);
    stockButtonLayout->addWidget(saveStockOrderButton);

    stockAdminLayout->addLayout(stockInputLayout);
    stockAdminLayout->addWidget(stockTable);
    stockAdminLayout->addLayout(stockButtonLayout);

    adminStockGroup->hide();
    mainLayout->addWidget(adminStockGroup);

    // Khu vuc bieu do lich su gia.
    QVBoxLayout *chartLayout = new QVBoxLayout(chartGroup);
    QHBoxLayout *chartControlLayout = new QHBoxLayout;

    chartSymbolCombo->setMinimumWidth(100);
    chartRangeCombo->addItem("1 gio", "1h");
    chartRangeCombo->addItem("24 gio", "24h");
    chartRangeCombo->addItem("7 ngay", "7d");

    chartControlLayout->addWidget(new QLabel("Ma co phieu:"));
    chartControlLayout->addWidget(chartSymbolCombo);
    chartControlLayout->addWidget(new QLabel("Khoang thoi gian:"));
    chartControlLayout->addWidget(chartRangeCombo);
    chartControlLayout->addWidget(loadHistoryButton);

    historyChart->addSeries(historySeries);
    historyTimeAxis->setFormat("HH:mm dd/MM");
    historyTimeAxis->setTitleText("Thoi gian");
    historyPriceAxis->setTitleText("Gia");
    historyPriceAxis->setLabelFormat("%.2f");

    historyChart->addAxis(historyTimeAxis, Qt::AlignBottom);
    historyChart->addAxis(historyPriceAxis, Qt::AlignLeft);
    historySeries->attachAxis(historyTimeAxis);
    historySeries->attachAxis(historyPriceAxis);
    historyChart->legend()->hide();

    chartView->setChart(historyChart);
    chartView->setMinimumHeight(260);

    chartLayout->addLayout(chartControlLayout);
    chartLayout->addWidget(chartView);

    chartGroup->hide();
    mainLayout->addWidget(chartGroup);

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
                    adminStockGroup->hide();
                    chartGroup->hide();
                    favoritesGroup->hide();
                    favoritesTable->setRowCount(0);
                    accountTable->setRowCount(0);
                    stockTable->setRowCount(0);
                    chartSymbolCombo->clear();
                    historySeries->clear();
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

                    chartGroup->show();
                    client.requestStockList();

                    if (role == "user" || role == "admin") {
                        favoritesGroup->show();
                        client.requestFavorites();
                    } else {
                        favoritesGroup->hide();
                        favoritesTable->setRowCount(0);
                    }

                    if (role == "admin") {
                        adminGroup->show();
                        adminStockGroup->show();
                        client.requestAdminAccounts();
                    } else {
                        adminGroup->hide();
                        adminStockGroup->hide();
                        accountTable->setRowCount(0);
                    }
                } else {
                    registrationLabel->setText(message);
                    adminGroup->hide();
                    adminStockGroup->hide();
                    chartGroup->hide();
                    favoritesGroup->hide();
                    favoritesTable->setRowCount(0);
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
    // Nhan danh sach ma co phieu tu Server.
    connect(&client, &StockClient::stockListReceived,
            this, &MainWindow::applyStockList);

    // Nhan du lieu lich su gia tu Server.
    connect(&client, &StockClient::stockHistoryReceived,
            this, &MainWindow::applyHistory);


    // Nhan danh sach ma co phieu yeu thich.
    connect(&client, &StockClient::favoritesReceived,
            this, &MainWindow::applyFavorites);

    // Nhan ket qua ghim hoac bo ghim ma co phieu.
    connect(&client, &StockClient::favoriteActionResult,
            this, [this](bool success, const QString &type,
                         const QString &message) {
                if (success) {
                    QMessageBox::information(
                        this, "Favorites", message);
                    client.requestFavorites();
                } else {
                    QMessageBox::warning(
                        this, "Favorites", message);
                }
            });
    // Nhan ket qua thao tac quan tri danh muc.
    connect(&client, &StockClient::adminStockResult,
            this, [this](bool success, const QString &message) {
                if (success) {
                    QMessageBox::information(
                        this, "Quan tri danh muc", message);
                    client.requestStockList();
                } else {
                    QMessageBox::warning(
                        this, "Quan tri danh muc", message);
                }
            });
// Tai lich su gia theo ma va khoang thoi gian.
    connect(loadHistoryButton, &QPushButton::clicked,
            this, [this]() {
                const QString symbol =
                    chartSymbolCombo->currentData().toString();

                if (symbol.isEmpty()) {
                    QMessageBox::warning(
                        this, "Lich su gia",
                        "Hay chon ma co phieu.");
                    return;
                }

                client.requestStockHistory(
                    symbol, chartRangeCombo->currentData().toString());
            });

    // Them ma co phieu.
    connect(addStockButton, &QPushButton::clicked,
            this, [this]() {
                const QString symbol =
                    stockSymbolInput->text().trimmed().toUpper();
                const QString name =
                    stockNameInput->text().trimmed();

                if (!QRegularExpression("^[A-Z0-9]{1,10}$")
                         .match(symbol).hasMatch()) {
                    QMessageBox::warning(
                        this, "Them ma co phieu",
                        "Ma co phieu chi gom 1-10 chu cai hoac chu so.");
                    return;
                }

                if (name.isEmpty()) {
                    QMessageBox::warning(
                        this, "Them ma co phieu",
                        "Vui long nhap ten cong ty.");
                    return;
                }

                client.addStock(symbol, name);
                stockSymbolInput->clear();
                stockNameInput->clear();
            });

    // Xoa ma co phieu dang duoc chon.
    connect(removeStockButton, &QPushButton::clicked,
            this, [this]() {
                const int row = stockTable->currentRow();

                if (row < 0 || !stockTable->item(row, 0)) {
                    QMessageBox::warning(
                        this, "Xoa ma co phieu",
                        "Hay chon mot ma trong danh sach.");
                    return;
                }

                const QString symbol =
                    stockTable->item(row, 0)->text();

                if (QMessageBox::question(
                        this, "Xac nhan xoa",
                        "Ban co chac muon xoa ma " + symbol + "?",
                        QMessageBox::Yes | QMessageBox::No,
                        QMessageBox::No) == QMessageBox::Yes) {
                    client.removeStock(symbol);
                }
            });

    // Hoan doi hai hang trong bang theo thu tu moi.
    auto moveStockRow = [this](int direction) {
        const int row = stockTable->currentRow();
        const int target = row + direction;

        if (row < 0 || target < 0 ||
            target >= stockTable->rowCount()) {
            return;
        }

        for (int column = 0; column < stockTable->columnCount(); ++column) {
            QTableWidgetItem *current = stockTable->takeItem(row, column);
            QTableWidgetItem *other = stockTable->takeItem(target, column);

            stockTable->setItem(row, column, other);
            stockTable->setItem(target, column, current);
        }

        for (int i = 0; i < stockTable->rowCount(); ++i) {
            if (stockTable->item(i, 2))
                stockTable->item(i, 2)->setText(QString::number(i));
        }

        stockTable->setCurrentCell(target, 0);
    };

    connect(moveStockUpButton, &QPushButton::clicked,
            this, [moveStockRow]() { moveStockRow(-1); });

    connect(moveStockDownButton, &QPushButton::clicked,
            this, [moveStockRow]() { moveStockRow(1); });

    // Gui thu tu danh muc moi len Server.
    connect(saveStockOrderButton, &QPushButton::clicked,
            this, [this]() {
                QStringList symbols;

                for (int row = 0; row < stockTable->rowCount(); ++row) {
                    if (stockTable->item(row, 0))
                        symbols.append(stockTable->item(row, 0)->text());
                }

                if (symbols.isEmpty()) {
                    QMessageBox::warning(
                        this, "Luu thu tu",
                        "Danh sach ma co phieu dang trong.");
                    return;
                }

                client.reorderStocks(symbols);
            });

// Ghim ma co phieu dang duoc chon tren bang gia.
    connect(addFavoriteButton, &QPushButton::clicked,
            this, [this]() {
                const int row = priceTable->currentRow();

                if (row < 0 || !priceTable->item(row, 0)) {
                    QMessageBox::warning(
                        this, "Ghim ma co phieu",
                        "Hay chon mot ma co phieu trong bang gia.");
                    return;
                }

                const QString symbol =
                    priceTable->item(row, 0)->text();

                client.addFavorite(symbol);
            });

    // Bo ghim ma co phieu dang duoc chon trong Favorites.
    connect(removeFavoriteButton, &QPushButton::clicked,
            this, [this]() {
                const int row = favoritesTable->currentRow();

                if (row < 0 || !favoritesTable->item(row, 0)) {
                    QMessageBox::warning(
                        this, "Bo ghim ma co phieu",
                        "Hay chon mot ma trong danh sach Favorites.");
                    return;
                }

                const QString symbol =
                    favoritesTable->item(row, 0)->text();

                if (QMessageBox::question(
                        this, "Xac nhan bo ghim",
                        "Ban co chac muon bo ghim ma " + symbol + "?",
                        QMessageBox::Yes | QMessageBox::No,
                        QMessageBox::No) == QMessageBox::Yes) {
                    client.removeFavorite(symbol);
                }
            });
}

MainWindow::~MainWindow() = default;


// Tim hang cua mot ma co phieu. Neu bang chua co ma nay thi them
// mot hang moi o cuoi bang (cac o gia, thay doi, thoi gian la "--")
// va tra ve chi so hang moi. Nho vay Server doi so luong ma luc demo
// thi Client van hien du.
int MainWindow::findOrAddRow(const QString &symbol)
{
    for (int row = 0; row < priceTable->rowCount(); ++row) {
        if (priceTable->item(row, 0)->text() == symbol) {
            return row;
        }
    }

    const int newRow = priceTable->rowCount();
    priceTable->insertRow(newRow);

    priceTable->setItem(
        newRow, 0, new QTableWidgetItem(symbol));

    for (int col = 1; col < priceTable->columnCount(); ++col) {
        priceTable->setItem(
            newRow, col, new QTableWidgetItem("--"));
    }

    return newRow;
}


// Nap bang gia ban dau theo SNAPSHOT.
void MainWindow::applySnapshot(const QList<StockQuote> &quotes)
{
    for (const StockQuote &quote : quotes) {
        const int row = findOrAddRow(quote.symbol);

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
    }
}


// Cap nhat mot gia co phieu theo PRICE_UPDATE.
void MainWindow::updatePrice(
    const QString &symbol,
    double price,
    const QString &timestamp)
{
    const int row = findOrAddRow(symbol);

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

void MainWindow::applyStockList(
    bool success,
    const QList<StockInfo> &stocks,
    const QString &message)
{
    if (!success) {
        QMessageBox::warning(
            this, "Danh sach co phieu",
            message.isEmpty()
                ? "Khong lay duoc danh sach co phieu."
                : message);
        return;
    }

    stockTable->setRowCount(0);
    chartSymbolCombo->clear();

    for (const StockInfo &stock : stocks) {
        const int row = stockTable->rowCount();
        stockTable->insertRow(row);

        stockTable->setItem(
            row, 0, new QTableWidgetItem(stock.symbol));
        stockTable->setItem(
            row, 1, new QTableWidgetItem(stock.name));
        stockTable->setItem(
            row, 2, new QTableWidgetItem(
                        QString::number(stock.displayOrder)));

        chartSymbolCombo->addItem(
            stock.symbol + " - " + stock.name,
            stock.symbol);

        findOrAddRow(stock.symbol);
    }
}

void MainWindow::applyHistory(
    bool success,
    const QString &symbol,
    const QList<HistoryPoint> &points,
    const QString &message)
{
    historySeries->clear();

    if (!success) {
        QMessageBox::warning(
            this, "Lich su gia",
            message.isEmpty()
                ? "Khong tai duoc lich su gia."
                : message);
        return;
    }

    if (points.isEmpty()) {
        QMessageBox::information(
            this, "Lich su gia",
            "Chua co du lieu lich su cho ma " + symbol + ".");
        return;
    }

    double minPrice = std::numeric_limits<double>::max();
    double maxPrice = std::numeric_limits<double>::lowest();

    qint64 minTime = std::numeric_limits<qint64>::max();
    qint64 maxTime = std::numeric_limits<qint64>::lowest();

    int validPoints = 0;

    for (const HistoryPoint &point : points) {
        QDateTime dateTime =
            QDateTime::fromString(
                point.timestamp, Qt::ISODateWithMs);

        if (!dateTime.isValid()) {
            dateTime = QDateTime::fromString(
                point.timestamp, Qt::ISODate);
        }

        if (!dateTime.isValid() || !qIsFinite(point.price))
            continue;

        const qint64 timestamp = dateTime.toMSecsSinceEpoch();

        historySeries->append(timestamp, point.price);

        minTime = qMin(minTime, timestamp);
        maxTime = qMax(maxTime, timestamp);
        minPrice = qMin(minPrice, point.price);
        maxPrice = qMax(maxPrice, point.price);

        ++validPoints;
    }

    if (validPoints == 0) {
        QMessageBox::warning(
            this, "Lich su gia",
            "Du lieu thoi gian hoac gia khong hop le.");
        return;
    }

    if (minTime == maxTime) {
        minTime -= 60000;
        maxTime += 60000;
    }

    double padding = (maxPrice - minPrice) * 0.1;

    if (padding < 0.01)
        padding = qMax(0.01, qAbs(maxPrice) * 0.01);

    historyTimeAxis->setRange(
        QDateTime::fromMSecsSinceEpoch(minTime),
        QDateTime::fromMSecsSinceEpoch(maxTime));

    historyPriceAxis->setRange(
        qMax(0.0, minPrice - padding),
        maxPrice + padding);

    historyChart->setTitle(
        "Lich su gia " + symbol);
}



// Hien thi danh sach ma co phieu yeu thich nhan tu Server.
void MainWindow::applyFavorites(
    bool success,
    const QList<FavoriteInfo> &favorites,
    const QString &message)
{
    if (!success) {
        QMessageBox::warning(
            this, "Favorites",
            message.isEmpty()
                ? "Khong tai duoc danh sach yeu thich."
                : message);
        return;
    }

    favoritesTable->setRowCount(0);

    for (const FavoriteInfo &favorite : favorites) {
        const int row = favoritesTable->rowCount();
        favoritesTable->insertRow(row);

        favoritesTable->setItem(
            row, 0, new QTableWidgetItem(favorite.symbol));

        favoritesTable->setItem(
            row, 1, new QTableWidgetItem(favorite.name));

        QDateTime dateTime = QDateTime::fromString(
            favorite.createdAt, Qt::ISODateWithMs);

        if (!dateTime.isValid()) {
            dateTime = QDateTime::fromString(
                favorite.createdAt, Qt::ISODate);
        }

        if (!dateTime.isValid()) {
            dateTime = QDateTime::fromString(
                favorite.createdAt, "yyyy-MM-dd HH:mm:ss");
        }

        const QString displayTime = dateTime.isValid()
            ? dateTime.toLocalTime().toString("dd/MM/yyyy HH:mm:ss")
            : favorite.createdAt;

        favoritesTable->setItem(
            row, 2, new QTableWidgetItem(displayTime));
    }
}
