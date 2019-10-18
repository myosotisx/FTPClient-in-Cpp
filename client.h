#ifndef CLIENT_H
#define CLIENT_H

#include "client_util.h"
#include "client_pi.h"

#include <QObject>

#define BUFCNT 10

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
    void showFileList(const char* fileList);

public slots:
    void setupControlConn(const char* ipAddr, int port);
    void login(const char* username, const char* password);
    void logout();
    void refresh();

private:
    int controlConnfd;
    int dataConnfd;
    int mode; // 0 PORT, 1 PASV
    int bufp;
    char buf[BUFCNT][MAXBUF];
    char fileList[MAXBUF];

    char* nextBuf() {
        char* recvBuf = buf[bufp];
        bufp = (bufp+1)%BUFCNT;
        return recvBuf;
    }

};

#endif // CLIENT_H
