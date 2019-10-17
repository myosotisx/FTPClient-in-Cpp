#include "main_window.h"
#include "ui_main_window.h"

#include <QDebug>
#include <QFileSystemModel>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , state(Client::NORMQUIT)
    , client(new Client)
    , controlThread(new QThread)
    , port(21) {
    ui->setupUi(this);

    QFileSystemModel* localFileModel = new QFileSystemModel(this);
    localFileModel->setRootPath(QDir::currentPath());
    ui->localFileTree->setModel(localFileModel);
    ui->localFileTree->setRootIndex(localFileModel->setRootPath("/Users/myosotis"));

    connect(client, &Client::setState, this, &MainWindow::setState, Qt::QueuedConnection);
    connect(client, &Client::showMsg, this, &MainWindow::displayMsg, Qt::QueuedConnection);

    connect(this, &MainWindow::setupControlConn, client, &Client::setupControlConn, Qt::QueuedConnection);
    connect(this, &MainWindow::login, client, &Client::login, Qt::QueuedConnection);
    connect(this, &MainWindow::logout, client, &Client::logout, Qt::QueuedConnection);

    connect(ui->connBtn, &QPushButton::clicked, this, &MainWindow::connectNLogin);
    connect(ui->disconnBtn, &QPushButton::clicked, this, &MainWindow::disconnNLogout);
    client->moveToThread(controlThread);
    controlThread->start();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::setState(int state) {
    if (state == Client::WAITUSER) {
        emit login(username ,password);
    }

    this->state = Client::State(state);

    if (this->state == Client::NORMQUIT || this->state == Client::ERRORQUIT) {
        ui->connBtn->setEnabled(true);
    }
    else ui->connBtn->setEnabled(false);
    qDebug() << "Debug Info: state is set to " << this->state;
}

void MainWindow::displayMsg(const char* msg) {

}

void MainWindow::connectNLogin() {
    memset(ipAddr, 0, 32);
    memset(username, 0, 32);
    memset(password, 0, 32);
    QMessageBox msgbox(QMessageBox::Information, tr("提示"), tr(""));
    msgbox.setButtonText(QMessageBox::Ok, tr("确 定"));
    if (!ui->hostLineEdit->text().isEmpty()) {
        strcpy(ipAddr, ui->hostLineEdit->text().toLatin1().data());
    }
    else {
        strcpy(ipAddr, "127.0.0.1");
        ui->hostLineEdit->setText("127.0.0.1");
    }

    if (!ui->portLineEdit->text().isEmpty()) {
        port = ui->portLineEdit->text().toInt();
    }
    else {
        ui->portLineEdit->setText("21");
    }
    if (!ui->userLineEdit->text().isEmpty()) {
        strcpy(username, ui->userLineEdit->text().toLatin1().data());
    }
    else {
        strcpy(username, "anonymous");
        ui->userLineEdit->setText("anonymous");
    }
    if (!ui->pswLineEdit->text().isEmpty()) {
        strcpy(password, ui->pswLineEdit->text().toLatin1().data());
    }
    else if (!strcmp(username, "anonymous")) {
        strcpy(password, "");
        ui->pswLineEdit->setText("");
    }
    else {
        msgbox.setText(tr("请输入密码！"));
        msgbox.exec();
        return;
    }

    emit setupControlConn(ipAddr, port);

}

void MainWindow::disconnNLogout() {
    if (this->state == Client::ERRORQUIT || this->state == Client::NORMQUIT) return;
    else emit logout();
}
