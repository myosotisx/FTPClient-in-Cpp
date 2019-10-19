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

    localFileModel = new FileModel(ui->localFileTree, "/Users/myosotis/Desktop");
    ui->localFileTree->setModel(localFileModel);
    ui->localFileTree->setDragDropMode(QAbstractItemView::DragDrop);
    ui->localFileTree->header()->setMinimumSectionSize(200);
    ui->localFileTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);

    remoteFileModel = new FileModel(ui->remoteFileTree, "/");
    ui->remoteFileTree->setModel(remoteFileModel);
    ui->remoteFileTree->setDragDropMode(QAbstractItemView::DragDrop);
    ui->remoteFileTree->header()->setMinimumSectionSize(200);
    ui->remoteFileTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);

    connect(client, &Client::setState, this, &MainWindow::setState, Qt::QueuedConnection);
    connect(client, &Client::reqUserInfo, this, &MainWindow::sendUserInfo, Qt::QueuedConnection);
    connect(client, &Client::setRemoteRoot, this, &MainWindow::initRemoteRoot, Qt::QueuedConnection);
    connect(client, &Client::showMsg, this, &MainWindow::displayMsg, Qt::QueuedConnection);
    connect(client, &Client::showLocal, this, &MainWindow::displayLocal, Qt::QueuedConnection);
    connect(client, &Client::showRemote, this, &MainWindow::displayRemote, Qt::QueuedConnection);

    connect(this, &MainWindow::setupControlConn, client, &Client::setupControlConn, Qt::QueuedConnection);
    connect(this, &MainWindow::login, client, &Client::login, Qt::QueuedConnection);
    connect(this, &MainWindow::logout, client, &Client::logout, Qt::QueuedConnection);
    // connect(this, &MainWindow::refreshLocal, client, &Client::refreshLocal, Qt::QueuedConnection);
    connect(this, &MainWindow::refreshRemote, client, &Client::refreshRemote, Qt::QueuedConnection);
    connect(this, &MainWindow::putFile, client, &Client::putFile, Qt::QueuedConnection);
    connect(this, &MainWindow::getFile, client, &Client::getFile, Qt::QueuedConnection);

    connect(ui->connBtn, &QPushButton::clicked, this, &MainWindow::connectNLogin);
    connect(ui->disconnBtn, &QPushButton::clicked, this, &MainWindow::disconnNLogout);
    connect(ui->localFileTree, &QTreeView::expanded, this, &MainWindow::refreshLocalDir);
    connect(ui->remoteFileTree, &QTreeView::expanded, this, &MainWindow::refreshRemoteDir);

    connect(remoteFileModel, &FileModel::transfer, this, &MainWindow::uploadFile);
    connect(localFileModel, &FileModel::transfer, this, &MainWindow::downloadFile);

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

void MainWindow::initRemoteRoot(const char* rootPath) {
    QString _rootPath(rootPath);
    remoteFileModel->initRoot(rootPath);
}

void MainWindow::showMsgbox(const QString& text) {
    QMessageBox msgbox(QMessageBox::Information, "Info", "");
    msgbox.setText(text);
    msgbox.exec();
}

void MainWindow::displayMsg(const char* msg, int type) {
    if (type == -1) type = checkState(msg);
    QString msgInHTML;
    if (type == 1) msgInHTML = "<pre><font color=\"#00FF00\">" + QString(msg).trimmed() + "</font></pre>";
    else if (type == 2) msgInHTML = "<pre><font color=\"#FFBF00\">" + QString(msg).trimmed() + "</font></pre>";
    else if (type == 0) msgInHTML = "<pre><font color=\"#FF0000\">" + QString(msg).trimmed() + "</font></pre>";
    else msgInHTML = "<pre><font color=\"#000000\">" + QString(msg) + "</font></pre>";
    ui->infoBroser->append(msgInHTML);
}

void MainWindow::connectNLogin() {
    memset(ipAddr, 0, 32);
    memset(username, 0, 32);
    memset(password, 0, 32);
    if (state == Client::BUSY) {
        showMsgbox("Waitint for response...");
        return;
    }
    if (state != Client::IDLE && state != Client::WAITUSER) {
        showMsgbox("You have connected to FTP server.");
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
        showMsgbox("Please input password.");
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

void MainWindow::refreshLocalDir(const QModelIndex& index) {
    FileNode* node = dynamic_cast<FileNode*>(localFileModel->itemFromIndex(index));
    if (!node) return;
    memset(localPath, 0, MAXPATH);
    strcpy(localPath, node->getPath().toLatin1().data());
    // emit refreshLocal(localPath);

    if (listDir(localFileList, localPath, "-l")) {
        displayLocal(localPath, localFileList);
    }
}

void MainWindow::refreshRemoteDir(const QModelIndex& index) {
    FileNode* node = dynamic_cast<FileNode*>(remoteFileModel->itemFromIndex(index));
    if (!node) return;
    memset(remotePath, 0, MAXPATH);
    strcpy(remotePath, node->getPath().toLatin1().data());
    if (state != Client::NORM) return;
    emit refreshRemote(remotePath);
}

void MainWindow::displayLocal(const char* path, const char* localFileList) {
    QString fileListStr(localFileList);
    FileNode* node = FileNode::findNodeByPath(localFileModel->getRoot(), path);
    if (!node) return;
    node->appendChildren(FileNode::parseFileListStr(fileListStr));
}

void MainWindow::displayRemote(const char* path, const char* remoteFileList) {
    QString fileListStr(remoteFileList);
    FileNode* node = FileNode::findNodeByPath(remoteFileModel->getRoot(), path);
    if (!node) return;
    node->appendChildren(FileNode::parseFileListStr(fileListStr));
}

void MainWindow::uploadFile(const QString& srcPath, const QString& srcFile,
                            const QString& dstPath) {
    QString _srcPath;
    QString _dstPath;
    if (srcPath[srcPath.length()-1] == '/') {
        _srcPath = srcPath+srcFile;
    }
    else _srcPath = srcPath+"/"+srcFile;

    if (dstPath[dstPath.length()-1] == '/') {
        _dstPath = dstPath+srcFile;
    }
    else _dstPath = dstPath+"/"+srcFile;

    memset(src, 0, MAXPATH);
    memset(dst, 0, MAXPATH);
    strcpy(src, _srcPath.toLatin1().data());
    strcpy(dst, _dstPath.toLatin1().data());
    if (state != Client::NORM) return;
    emit putFile(src, dst);
}

void MainWindow::downloadFile(const QString& srcPath, const QString& srcFile,
                              const QString& dstPath) {
    QString _srcPath;
    QString _dstPath;
    if (srcPath[srcPath.length()-1] == '/') {
        _srcPath = srcPath+srcFile;
    }
    else _srcPath = srcPath+"/"+srcFile;

    if (dstPath[dstPath.length()-1] == '/') {
        _dstPath = dstPath+srcFile;
    }
    else _dstPath = dstPath+"/"+srcFile;

    memset(src, 0, MAXPATH);
    memset(dst, 0, MAXPATH);
    strcpy(src, _srcPath.toLatin1().data());
    strcpy(dst, _dstPath.toLatin1().data());
    if (state != Client::NORM) return;
    emit getFile(src, dst);
}

void MainWindow::sendUserInfo() {
    if (state != Client::WAITUSER) return;
    emit login(username, password);
}
