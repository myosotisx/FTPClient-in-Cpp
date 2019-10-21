#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "client.h"
#include "file_model.h"

#include <QMainWindow>
#include <QThread>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void setupControlConn(const char* ipAddr, int port);
    void login(const char* username, const char* password);
    void logout();
    // void refreshLocal(const char* path);
    void refreshRemote(const char* path);
    void putFile(const char* src, const char* dst);
    void getFile(const char* src, const char* dst);
    void switchMode(int mode);
    void removeRemote(const char* path, int type);

public slots:
    void setState(int state);
    void initRemoteRoot(const char* rootPath);
    void showMsgbox(const QString& text);
    void showMenu(const QPoint& pos);
    void connectNLogin();
    void disconnNLogout();
    void switchPASV();
    void switchPORT();
    void refreshLocalDir(const QModelIndex& index);
    void refreshRemoteDir(const QModelIndex& index);
    void displayMsg(const char* msg, int type = -1);
    void displayLocal(const char* path, const char* localFileList);
    void displayRemote(const char* path, const char* remoteFileList);
    void uploadFile(const QString& srcPath, const QString& srcFile,
                    const QString& dstPath);
    void downloadFile(const QString& srcPath, const QString& srcFile,
                      const QString& dstPath);
    void sendUserInfo();
    void renameRemote();
    void deleteRemote();
    void createRemote();

    void test(QStandardItem *item);

private:
    Ui::MainWindow *ui;
    Client::State state;
    Client* client;
    FileModel* localFileModel;
    FileModel* remoteFileModel;
    QThread* controlThread;
    QMenu* remoteMenu;
    char ipAddr[32];
    int port;
    char username[32];
    char password[32];
    char localPath[MAXPATH];
    char remotePath[MAXPATH];
    char src[MAXPATH];
    char dst[MAXPATH];
    char localFileList[MAXBUF];

    void initMenu();
};
#endif // MAINWINDOW_H
