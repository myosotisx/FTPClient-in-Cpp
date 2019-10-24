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
    friend void setRootPath(Client* client, const char* rootPath);
    friend void setLastSendTime(Client* client, long sec, long usec);
    friend double getLastSendTime(Client* client);
    friend void updateProgress(Client* client, long long progress);

signals:
    void setState(int);
    void reqUserInfo();
    void setRemoteRoot(const char* rootPath);
    void showMsg(const char* msg, int type = -1);
    void showLocal(const char* path, const char* localFileList);
    void showRemote(const char* path, const char* remoteFileList);
    void showProgress(long long progress);
    void uploadFinished();

public slots:
    void setupControlConn(const char* ipAddr, int port);
    void login(const char* username, const char* password);
    void logout();
    void refreshLocal(const char* path);
    void refreshRemote(const char* path);
    void putFile(const char* src, const char* dst);
    void getFile(const char* src, const char* dst);
    void switchMode(int mode);
    void removeRemote(const char* path, const char* parentPath, int type);
    void renameRemote(const char* oldPath, const char* newPath, const char* parentPath);
    void makeDirRemote(const char* path, const char* parentPath);
    void changeRemoteWorkDir(const char* path, const char* oldPath);

private:
    State state;
    int resCode;
    int controlConnfd;
    int dataConnfd;
    int dataListenfd;
    int mode; // 0 PORT, 1 PASV
    int bufp;
    char ipAddr[32];
    int port;
    char rootPath[MAXPATH];
    char buf[BUFCNT][MAXBUF];
    char localFileList[MAXBUF];
    char remoteFileList[MAXBUF];

    pthread_t controlThread;

    char* nextBuf();
    int waitResCode(int stateCode, double timeout);
    int waitConn(int listenfd, double timeout);

    timeval lastSendTime;

};

#endif // CLIENT_H
