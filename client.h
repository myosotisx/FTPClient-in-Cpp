#ifndef CLIENT_H
#define CLIENT_H

#include "client_util.h"
#include "client_pi.h"

#include <pthread.h>

#include <QObject>

#define BUFCNT 10

class Client: public QObject {
    Q_OBJECT

public:
    enum State { IDLE = 0
                 , NORM = 1
                 , BUSY = 2
                 , WAITUSER = 3
                 , WAITRNTO = 4 };
    Client(QObject* parent = nullptr);

    friend void* receiver(void* _client);
    friend int getClientState(Client* client);
    friend void setClientState(Client* client, int state);
    friend int getControlConnfd(Client* client);
    friend void setDataConnAddr(Client* client, const char* ipAddr, int port);
    friend void setResCode(Client* client, int resCode);

signals:
    void setState(int);
    void reqUserInfo();
    void showMsg(const char* msg);
    void showFileList(const char* fileList);

public slots:
    void setupControlConn(const char* ipAddr, int port);
    void login(const char* username, const char* password);
    void logout();
    void refresh();

private:
    State state;
    int resCode;
    int controlConnfd;
    int dataConnfd;
    int mode; // 0 PORT, 1 PASV
    int bufp;
    char ipAddr[32];
    int port;
    char buf[BUFCNT][MAXBUF];
    char fileList[MAXBUF];

    pthread_t controlThread;

    char* nextBuf();
    int waitResCode(int stateCode, double timeout);

};

#endif // CLIENT_H
