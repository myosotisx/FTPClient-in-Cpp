#ifndef CLIENT_H
#define CLIENT_H

#include "client_util.h"
#include "client_pi.h"

#include <QObject>


class Client: public QObject {
    Q_OBJECT

public:
    enum State {ERRORQUIT = -1
                , NORMQUIT = 0
                , NORM = 1
                , TRANSFER = 2
                , WAITUSER = 3
                , WAITPASS = 4
                , WAITRNTO = 5};
    Client(QObject* parent = nullptr);

signals:
    void setState(int);
    void showMsg(const char* msg);

public slots:
    void setupControlConn(const char* ipAddr, int port);
    void login(const char* username, const char* password);
    void logout();

private:
    int controlConnfd;
    char recvBuf[MAXBUF];
};

#endif // CLIENT_H
