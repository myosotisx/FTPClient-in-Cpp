#include "main_window.h"
#include "ui_main_window.h"

#include "client_pi.h"

#include <QDebug>
#include <QFileSystemModel>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , state(Client::IDLE)
    , client(new Client)
    , controlThread(new QThread)
    , port(21) {
    ui->setupUi(this);

    // QFileSystemModel* localFileModel = new QFileSystemModel(this);
    localFileModel->setRootPath(QDir::currentPath());
    ui->localFileTree->setModel(localFileModel);
    ui->localFileTree->setRootIndex(localFileModel->setRootPath("/Users/myosotis"));

    remoteFileModel = new FileModel(ui->remoteFileTree);
    ui->remoteFileTree->setModel(remoteFileModel);
    ui->remoteFileTree->setDragDropMode(QAbstractItemView::DragDrop);


    connect(client, &Client::setState, this, &MainWindow::setState, Qt::QueuedConnection);
    connect(client, &Client::reqUserInfo, this, &MainWindow::sendUserInfo, Qt::QueuedConnection);
    connect(client, &Client::showMsg, this, &MainWindow::displayMsg, Qt::QueuedConnection);
    connect(client, &Client::showFileList, this, &MainWindow::displayFileList, Qt::QueuedConnection);

    connect(this, &MainWindow::setupControlConn, client, &Client::setupControlConn, Qt::QueuedConnection);
    connect(this, &MainWindow::login, client, &Client::login, Qt::QueuedConnection);
    connect(this, &MainWindow::logout, client, &Client::logout, Qt::QueuedConnection);
    connect(this, &MainWindow::refresh, client, &Client::refresh, Qt::QueuedConnection);
    connect(ui->connBtn, &QPushButton::clicked, this, &MainWindow::connectNLogin);
    connect(ui->disconnBtn, &QPushButton::clicked, this, &MainWindow::disconnNLogout);
    connect(ui->refreshBtn, &QPushButton::clicked, this, &MainWindow::refreshRemoteRoot);
    connect(ui->remoteFileTree, &QTreeView::expanded, this, &MainWindow::refreshRemoteDir);
    QThread* controlThread = new QThread;
    client->moveToThread(controlThread);
    controlThread->start();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::setState(int state) {
    this->state = Client::State(state);
    qDebug() << "Debug Info: state is set to " << this->state;
}

void MainWindow::displayMsg(const char* msg) {
    int type = checkState(msg);
    QString msgInHTML;
    if (type == 1) msgInHTML = "<pre><font color=\"#00FF00\">" + QString(msg) + "</font></pre>";
    else if (type == 2) msgInHTML = "<pre><font color=\"#FFBF00\">" + QString(msg) + "</font></pre>";
    else if (type == 0) msgInHTML = "<pre><font color=\"#FF0000\">" + QString(msg) + "</font></pre>";
    else msgInHTML = "<pre><font color=\"#000000\">" + QString(msg) + "</font></pre>";
    ui->infoBroser->append(msgInHTML);
}

void MainWindow::connectNLogin() {
    memset(ipAddr, 0, 32);
    memset(username, 0, 32);
    memset(password, 0, 32);
    QMessageBox msgbox(QMessageBox::Information, tr("提示"), tr(""));

    msgbox.setButtonText(QMessageBox::Ok, tr("确 定"));
    if (state == Client::BUSY) {
        msgbox.setText(tr("等待响应..."));
        msgbox.exec();
        return;
    }
    if (state != Client::IDLE && state != Client::WAITUSER) {
        msgbox.setText(tr("您已连接！"));
        msgbox.exec();
        return;
    }

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
        strcpy(username, "myosotisx");
        ui->userLineEdit->setText("myosotisx");
    }
    if (!ui->pswLineEdit->text().isEmpty()) {
        strcpy(password, ui->pswLineEdit->text().toLatin1().data());
    }
    else if (!strcmp(username, "myosotisx")) {
        strcpy(password, "9");
        ui->pswLineEdit->setText("9");
    }
    else {
        msgbox.setText(tr("请输入密码！"));
        msgbox.exec();
        return;
    }

    if (state == Client::WAITUSER) {
        emit login(username, password);
    }
    else {
        emit setupControlConn(ipAddr, port);
    }

}

void MainWindow::disconnNLogout() {
    if (this->state == Client::IDLE) return;
    else {
        emit logout();
    }
}

void MainWindow::refreshRemoteRoot() {
    emit refresh("/");
}

void MainWindow::refreshRemoteDir(const QModelIndex& index) {
    FileNode* node = dynamic_cast<FileNode*>(remoteFileModel->itemFromIndex(index));
    if (!node) return;
    memset(path, 0, MAXPATH);
    strcpy(path, node->getPath().toLatin1().data());
    emit refresh(path);
}

void MainWindow::displayFileList(const char* path, const char* fileList) {
    QString fileListStr(fileList);
    FileNode* node = FileNode::findNodeByPath(remoteFileModel->getRoot(), path);
    //remoteFileModel->getRoot()->appendChildren(FileNode::parseFileListStr(fileListStr));
    if (!node) return;
    node->appendChildren(FileNode::parseFileListStr(fileListStr));
}

void MainWindow::sendUserInfo() {
    emit login(username, password);
}
