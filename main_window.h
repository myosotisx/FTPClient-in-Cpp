#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "client.h"

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

public slots:
    void setState(int state);
    void connectNLogin();
    void disconnNLogout();
    void displayMsg(const char* msg);

private:
    Ui::MainWindow *ui;
    Client::State state;
    Client* client;
    QThread* controlThread;
    char ipAddr[32];
    int port;
    char username[32];
    char password[32];
};
#endif // MAINWINDOW_H
