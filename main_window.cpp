#include "main_window.h"
#include "ui_main_window.h"

#include "client_pi.h"

#include <QDebug>
#include <QTimer>
#include <QFileSystemModel>
#include <QAbstractItemDelegate>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , state(Client::IDLE)
    , client(new Client)
    , controlThread(new QThread)
    , remoteFileMenu(new QMenu(this))
    , remoteDirMenu(new QMenu(this))
    , remoteRootMenu(new QMenu(this))
    , timer(new QTimer)
    , port(21)
    , currentSize(-1)
    , lastStartPoint(0)
    , transferPercent(0.0) {
    ui->setupUi(this);
    setWindowTitle(tr("FTP Client"));

    initMenu();

    localFileModel = new FileModel("local", ui->localFileTree, "/Users/myosotis/Documents/GitHub/FTPServer-in-C/FTPFile");
    ui->localFileTree->setModel(localFileModel);
    ui->localFileTree->setDragDropMode(QAbstractItemView::DragDrop);
    ui->localFileTree->header()->setMinimumSectionSize(200);
    ui->localFileTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);

    remoteFileModel = new FileModel("remote", ui->remoteFileTree, "(root)");
    ui->remoteFileTree->setModel(remoteFileModel);
    ui->remoteFileTree->setDragDropMode(QAbstractItemView::DragDrop);
    ui->remoteFileTree->header()->setMinimumSectionSize(200);
    ui->remoteFileTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->remoteFileTree->setEnabled(false);

    transferListModel = new FileListModel(ui->transferListView);
    progressDelegate = new ProgressDelegate(ui->transferListView);
    ui->transferListView->setModel(transferListModel);
    ui->transferListView->setItemDelegateForColumn(2, progressDelegate);
    ui->transferListView->header()->setMinimumSectionSize(200);

    connect(client, &Client::setState, this, &MainWindow::setState, Qt::QueuedConnection);
    connect(client, &Client::reqUserInfo, this, &MainWindow::sendUserInfo, Qt::QueuedConnection);
    connect(client, &Client::setRemoteRoot, this, &MainWindow::initRemoteRoot, Qt::QueuedConnection);
    connect(client, &Client::showMsg, this, &MainWindow::displayMsg, Qt::QueuedConnection);
    connect(client, &Client::showLocal, this, &MainWindow::displayLocal, Qt::QueuedConnection);
    connect(client, &Client::showRemote, this, &MainWindow::displayRemote, Qt::QueuedConnection);
    connect(client, &Client::showProgress, this, &MainWindow::setPercent, Qt::QueuedConnection);
    connect(client, &Client::transferFinished, this, &MainWindow::transferFinished, Qt::QueuedConnection);
    connect(client, &Client::transferFail, this, &MainWindow::transferFail, Qt::QueuedConnection);

    connect(this, &MainWindow::setupControlConn, client, &Client::setupControlConn, Qt::QueuedConnection);
    connect(this, &MainWindow::login, client, &Client::login, Qt::QueuedConnection);
    connect(this, &MainWindow::logout, client, &Client::logout, Qt::QueuedConnection);
    connect(this, &MainWindow::refreshRemote, client, &Client::refreshRemote, Qt::QueuedConnection);
    connect(this, &MainWindow::putFile, client, &Client::putFile, Qt::QueuedConnection);
    connect(this, &MainWindow::getFile, client, &Client::getFile, Qt::QueuedConnection);
    connect(this, &MainWindow::switchMode, client, &Client::switchMode, Qt::QueuedConnection);
    connect(this, &MainWindow::removeRemote, client, &Client::removeRemote, Qt::QueuedConnection);
    connect(this, &MainWindow::renameRemote, client, &Client::renameRemote, Qt::QueuedConnection);
    connect(this, &MainWindow::makeDirRemote, client, &Client::makeDirRemote, Qt::QueuedConnection);
    connect(this, &MainWindow::changeRemoteWorkDir, client, &Client::changeRemoteWorkDir, Qt::QueuedConnection);

    connect(ui->connBtn, &QPushButton::clicked, this, &MainWindow::connectNLogin);
    connect(ui->disconnBtn, &QPushButton::clicked, this, &MainWindow::disconnNLogout);
    connect(ui->localFileTree, &QTreeView::expanded, this, &MainWindow::refreshLocalDir);
    connect(ui->remoteFileTree, &QTreeView::expanded, this, &MainWindow::refreshRemoteDir);
    connect(ui->PASVBtn, &QRadioButton::toggled, this, &MainWindow::switchPASV);
    connect(ui->PORTBtn, &QRadioButton::toggled, this, &MainWindow::switchPORT);
    connect(ui->remoteFileTree, &QWidget::customContextMenuRequested, this, &MainWindow::showMenu);

    connect(remoteFileModel, &FileModel::transfer, this, &MainWindow::uploadFile);
    connect(localFileModel, &FileModel::transfer, this, &MainWindow::downloadFile);
    connect(remoteFileModel, &FileModel::textChanged, this, &MainWindow::changeNameRemote);
    connect(remoteFileModel, &FileModel::rootChanged, this, &MainWindow::changeRemoteRoot);

    QThread* controlThread = new QThread;
    client->moveToThread(controlThread);
    controlThread->start();

    timer->start(500);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::initMenu() {
    // remoteFileMenu
    QAction* acRename = new QAction("Rename", this);
    connect(acRename, &QAction::triggered, this, &MainWindow::setEditState);
    remoteFileMenu->addAction(acRename);

    QAction* acCreate = new QAction("Create Directory", this);
    connect(acCreate, &QAction::triggered, this, &MainWindow::createRemote);
    remoteFileMenu->addAction(acCreate);

    // remoteDirMenu
    remoteDirMenu->addAction(acRename);
    remoteDirMenu->addAction(acCreate);

    QAction* acDelete = new QAction("Delete", this);
    connect(acDelete, &QAction::triggered, this, &MainWindow::deleteRemote);
    remoteDirMenu->addAction(acDelete);

    // remoteRootMenu
    remoteRootMenu->addAction(acCreate);

    QAction* acChangeWorkDir = new QAction("Change Work Directory", this);
    connect(acChangeWorkDir, &QAction::triggered, this, &MainWindow::setEditState);
    remoteRootMenu->addAction(acChangeWorkDir);
}

void MainWindow::setState(int state) {
    this->state = Client::State(state);
    qDebug() << "Debug Info: state is set to " << this->state;
    if (this->state == Client::IDLE) ui->remoteFileTree->setEnabled(false);
    else if (this->state == Client::NORM){ ui->remoteFileTree->setEnabled(true); }
}

bool MainWindow::checkClientState() {
    if (state == Client::BUSY) {
        showMsgbox("Please wait for previous task complete.");
        return false;
    }
    else if (state == Client::WAITUSER) {
        showMsgbox("Please login before further operations.");
        return false;
    }
    else if (state == Client::IDLE) {
        showMsgbox("Please connect to FTP server first.");
        return false;
    }
    else return true;
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

void MainWindow::showMenu(const QPoint& pos) {
    QModelIndex index = ui->remoteFileTree->indexAt(pos);
    if (index.isValid()) {
        QStandardItem* item = remoteFileModel->itemFromIndex(index);
        FileNode* node = dynamic_cast<FileNode*>(item);
        if (node && item->parent() && node->getType() == FileNode::FILE) {
            remoteFileMenu->exec(QCursor::pos());
        }
        else if (node && item->parent() && node->getType() == FileNode::DIR) {
            remoteDirMenu->exec(QCursor::pos());
        }
        else if (node && !item->parent()) {
            remoteRootMenu->exec(QCursor::pos());
        }
    }
}

void MainWindow::displayMsg(const char* msg, int type) {
    if (type == -1) type = checkState(msg);
    QString msgInHTML;
    if (type == 1) msgInHTML = "<pre><font color=\"#00FF00\">" + QString(msg).trimmed() + "</font></pre>";
    else if (type == 2) msgInHTML = "<pre><font color=\"#FFBF00\">" + QString(msg).trimmed() + "</font></pre>";
    else if (type == 0) msgInHTML = "<pre><font color=\"#FF0000\">" + QString(msg).trimmed() + "</font></pre>";
    else msgInHTML = "<pre><font color=\"#000000\">" + QString(msg) + "</font></pre>";
    ui->infoBrowser->append(msgInHTML);
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

void MainWindow::switchPASV() {
    emit switchMode(1);
}

void MainWindow::switchPORT() {
    emit switchMode(0);
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
    memset(remotePath[0], 0, MAXPATH);
    strcpy(remotePath[0], node->getPath().toLatin1().data());
    if (state != Client::NORM) return;
    emit refreshRemote(remotePath[0]);
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
    if (!checkClientState()) return;

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
    // if (state != Client::NORM) return;
    FileNode* node = FileNode::findNodeByPath(localFileModel->getRoot() , _srcPath);
    if (node) transferSize = node->getSize();
    else transferSize = -1;
    transferListModel->appendFileItem(srcFile, "Upload", 0.0, transferSize);
    connect(timer, &QTimer::timeout, this, &MainWindow::displayProgress);


    FileNode* dstNode = FileNode::findNodeByPath(remoteFileModel->getRoot(), _dstPath);
    if (dstNode && (currentSize = dstNode->getSize()) != -1) {
        if (transferSize != -1) {
            transferPercent = currentSize/(1.0*transferSize);
            transferListModel->item(transferListModel->rowCount()-1, 2)->setText(QString::number(transferPercent));
        }
        else {
            transferListModel->item(transferListModel->rowCount()-1, 2)->setText("Unknown");
        }
        emit putFile(src, dst, currentSize);
    }
    else {
        currentSize = -1;
        emit putFile(src, dst, 0);
    }
}

void MainWindow::downloadFile(const QString& srcPath, const QString& srcFile,
                              const QString& dstPath) {
    if (!checkClientState()) return;
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
    // if (state != Client::NORM) return;

    FileNode* srcNode = FileNode::findNodeByPath(remoteFileModel->getRoot() , _srcPath);
    if (srcNode) transferSize = srcNode->getSize();
    else transferSize = -1;

    transferListModel->appendFileItem(srcFile, "Download", 0.0, transferSize);
    connect(timer, &QTimer::timeout, this, &MainWindow::displayProgress);

    FileNode* dstNode = FileNode::findNodeByPath(localFileModel->getRoot(), _dstPath);
    if (dstNode && (currentSize = dstNode->getSize()) != -1) {
        lastStartPoint = currentSize;
        if (transferSize != -1) {
            transferPercent = currentSize/(1.0*transferSize);
            transferListModel->item(transferListModel->rowCount()-1, 2)->setText(QString::number(transferPercent));
        }
        else {
            transferListModel->item(transferListModel->rowCount()-1, 2)->setText("Unknown");
        }
        emit getFile(src, dst, currentSize);
    }
    else if (lastStartPoint) {
        lastStartPoint = 0;
        currentSize = -1;
        emit getFile(src, dst, 0);
    }
    else {
        currentSize = -1;
        emit getFile(src, dst, -1);
    }

}

void MainWindow::sendUserInfo() {
    if (state != Client::WAITUSER) return;
    emit login(username, password);
}

void MainWindow::setEditState() {
    if (!checkClientState()) return;
    QModelIndex index = ui->remoteFileTree->currentIndex();
    ui->remoteFileTree->edit(index);
}

void MainWindow::createRemote() {
    if (!checkClientState()) return;
    QModelIndex index = ui->remoteFileTree->currentIndex();
    FileNode* node = dynamic_cast<FileNode*>(remoteFileModel->itemFromIndex(index));
    FileNode* dirNode = FileNode::findNodeByPath(remoteFileModel->getRoot(), node->getPath());
    FileNode* childDirNode = dirNode->appendChildDir();
    QModelIndex childDirIndex = remoteFileModel->indexFromItem(childDirNode);
    if (!childDirIndex.isValid()) return;
    disconnect(ui->remoteFileTree, &QTreeView::expanded, this, &MainWindow::refreshRemoteDir);
    ui->remoteFileTree->setExpanded(remoteFileModel->indexFromItem(dirNode), true);
    connect(ui->remoteFileTree, &QTreeView::expanded, this, &MainWindow::refreshRemoteDir);
    ui->remoteFileTree->setCurrentIndex(childDirIndex);
    connect(ui->remoteFileTree->itemDelegate(), &QAbstractItemDelegate::closeEditor
            , this, &MainWindow::nameDirFinished);
    ui->remoteFileTree->edit(childDirIndex);
}

void MainWindow::nameDirFinished() {
    disconnect(ui->remoteFileTree->itemDelegate(), &QAbstractItemDelegate::closeEditor
            , this, &MainWindow::nameDirFinished);
    QModelIndex index = ui->remoteFileTree->currentIndex();
    FileNode* node = dynamic_cast<FileNode*>(remoteFileModel->itemFromIndex(index));
    if (!node) return;
    memset(remotePath[0], 0, MAXPATH);
    strcpy(remotePath[0], node->getPath().toLatin1().data());
    qDebug() << remotePath[0];
    memset(remotePath[1], 0, MAXPATH);
    strcpy(remotePath[1], node->getParentPath().toLatin1().data());
    emit makeDirRemote(remotePath[0], remotePath[1]);
}

void MainWindow::deleteRemote() {
    if (!checkClientState()) return;
    QModelIndex index = ui->remoteFileTree->currentIndex();
    FileNode* node = dynamic_cast<FileNode*>(remoteFileModel->itemFromIndex(index));
    if (!node) return;
    memset(remotePath[0], 0, MAXPATH);
    strcpy(remotePath[0], node->getPath().toLatin1().data());
    qDebug() << remotePath[0];
    memset(remotePath[1], 0, MAXPATH);
    strcpy(remotePath[1], node->getParentPath().toLatin1().data());
    emit removeRemote(remotePath[0], remotePath[1], node->getType()); // 2为文件夹
}

void MainWindow::changeNameRemote(const QModelIndex& index, const QString& oldPath) {
    if (!checkClientState()) return;
    FileNode* node = dynamic_cast<FileNode*>(remoteFileModel->itemFromIndex(index));
    if (!node) return;
    memset(remotePath[0], 0, MAXPATH);
    strcpy(remotePath[0], oldPath.toLatin1().data());
    memset(remotePath[1], 0, MAXPATH);
    strcpy(remotePath[1], node->getFilePath().toLatin1().data());
    memset(remotePath[2], 0, MAXPATH);
    strcpy(remotePath[2], node->getParentPath().toLatin1().data());
    emit renameRemote(remotePath[0], remotePath[1], remotePath[2]);
}

void MainWindow::changeRemoteRoot(const QString& oldRoot) {
    if (!checkClientState()) {
        remoteFileModel->getRoot()->setText(oldRoot);
        return;
    }
    // return 后将root恢复
    FileNode* root = remoteFileModel->getRoot();
    memset(remotePath[0], 0, MAXPATH);
    strcpy(remotePath[0], root->text().toLatin1().data());
    memset(remotePath[1], 0, MAXPATH);
    strcpy(remotePath[1], oldRoot.toLatin1().data());
    emit changeRemoteWorkDir(remotePath[0], remotePath[1]);
}

void MainWindow::setPercent(long long progress) {
    if (transferSize == -1) return;
    if (currentSize == -1) transferPercent = progress/(1.0*transferSize);
    else transferPercent = (progress+currentSize)/(1.0*transferSize);
}

void MainWindow::displayProgress() {
    if (transferSize != -1) {
        transferListModel->item(transferListModel->rowCount()-1, 2)
                ->setText(QString::number(transferPercent));
    }
    else {
        transferListModel->item(transferListModel->rowCount()-1, 2)
                ->setText("Unknown");
    }

}

void MainWindow::transferFinished() {
    transferListModel->item(transferListModel->rowCount()-1, 2)
            ->setText("Transfer Finished");
    disconnect(timer, &QTimer::timeout, this, &MainWindow::displayProgress);
    transferSize = -1;
    transferPercent = 0.0;
}

void MainWindow::transferFail() {
    transferListModel->item(transferListModel->rowCount()-1, 2)
            ->setText("Transfer Failed");
    disconnect(timer, &QTimer::timeout, this, &MainWindow::displayProgress);
    transferSize = -1;
    transferPercent = 0.0;
}
