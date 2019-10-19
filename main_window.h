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
    void refresh(const char* path);

public slots:
    void setState(int state);
    void connectNLogin();
    void disconnNLogout();
    // void refreshFileList(const QModelIndex& index);
    void refreshRemoteRoot();
    void refreshRemoteDir(const QModelIndex& index);
    void displayMsg(const char* msg);
    void displayFileList(const char* path, const char* fileList);
    void sendUserInfo();

private:
    Ui::MainWindow *ui;
    Client::State state;
    Client* client;
    FileModel* localFileModel;
    FileModel* remoteFileModel;
    QThread* controlThread;
    char ipAddr[32];
    int port;
    char username[32];
    char password[32];
    char path[MAXPATH];
};
#endif // MAINWINDOW_H
