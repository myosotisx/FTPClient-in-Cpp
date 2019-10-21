#include "client.h"

#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>

#include <QDebug>

void* receiver(void* _client) {
    Client* client = (Client*)_client;
    int controlConnfd = client->controlConnfd;
    int recvLen;
    char* recvBuf = client->nextBuf();
    while ((recvLen = readBuf(controlConnfd, recvBuf))) {
        if (recvLen == -1) {
            setClientState(client, Client::IDLE);
            return nullptr;
        }
        else if (recvLen != 0) {
            recvBuf[recvLen] = 0;
            emit client->showMsg(recvBuf);
            int resCode;
            char param[MAXPARAM];
            if (getResCodeNParam(recvBuf, &resCode, param) != -1) {
                resCodeMapper(client, resCode, param);
            }
            recvBuf = client->nextBuf();
        }
        pthread_testcancel();
    }
}

int getClientState(Client* client) {
    return client->state;
}

void setClientState(Client* client, int state) {
    client->state = Client::State(state);
    if (client->state == Client::IDLE) {
        pthread_cancel(client->controlThread);
        close(client->dataConnfd);
        close(client->controlConnfd);
        client->controlConnfd = -1;
    }
    emit client->setState(client->state);
}

int getControlConnfd(Client* client) {
    return client->controlConnfd;
}

void setDataConnAddr(Client* client, const char* ipAddr, int port) {
    memset(client->ipAddr, 0, 32);
    strcpy(client->ipAddr, ipAddr);
    client->port = port;
}

void setResCode(Client* client, int resCode) {
    client->resCode = resCode;
}

void setRootPath(Client* client, const char* rootPath) {
    memset(client->rootPath, 0, MAXPATH);
    strcpy(client->rootPath, rootPath);
}

void setLastSendTime(Client* client, long sec, long usec) {
    client->lastSendTime.tv_sec = sec;
    client->lastSendTime.tv_usec = usec;
}

double getLastSendTime(Client* client) {
    double timeStamp = (1000000*client->lastSendTime.tv_sec
                     +client->lastSendTime.tv_usec)/1000.0;
    return timeStamp;
}

Client::Client(QObject* parent):
    QObject(parent)
    , state(IDLE)
    , resCode(-1)
    , controlConnfd(-1)
    , dataConnfd(-1)
    , dataListenfd(-1)
    , mode(1)
    , bufp(0) {

    gettimeofday(&lastSendTime, nullptr);
}

char* Client::nextBuf() {
    char* recvBuf = buf[bufp];
    bufp = (bufp+1)%BUFCNT;
    return recvBuf;
}

int Client::waitResCode(int resCode, double timeout) {
    timeval starttime, endtime;
    gettimeofday(&starttime, nullptr);
    while (1) {
        gettimeofday(&endtime, nullptr);
        double timeuse = 1000000*(endtime.tv_sec-starttime.tv_sec)
                         +endtime.tv_usec-starttime.tv_usec;
        if (timeuse/1000 > 1000*timeout) return 0;
        if (this->resCode == resCode) {
            resCode = -1;
            return 1;
        }

    }
}

int Client::waitConn(int listenfd, double timeout) {
    timeval starttime, endtime, tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    gettimeofday(&starttime, nullptr);
    fd_set fds;
    while (1) {
        FD_ZERO(&fds);
        FD_SET(listenfd, &fds);
        int ret = select(listenfd+1, &fds, nullptr, nullptr, &tv);
        if (ret > 0 && FD_ISSET(listenfd, &fds)) {
            int newfd = acceptNewConn(listenfd);
            return newfd;
        }
        gettimeofday(&endtime, nullptr);
        double timeuse = 1000000*(endtime.tv_sec-starttime.tv_sec)
                         +endtime.tv_usec-starttime.tv_usec;
        if (timeuse/1000 > 1000*timeout) return -1;
    }
}

void Client::setupControlConn(const char* ipAddr, int port) {
    setClientState(this, BUSY);
    State nState = IDLE;
    if ((controlConnfd = setupConn(ipAddr, port, 1)) != -1) {
        pthread_create(&controlThread, nullptr, &receiver, this);

        if (waitResCode(220, 2)) {
            nState = WAITUSER;
            if (state != IDLE) setClientState(this, nState);
            emit reqUserInfo();
            return;
        }
        else {
            emit showMsg("Client: Connection timeout!", 0);
        }
    }
    else {
        emit showMsg("Client: Fail to setup connection with FTP server!\r\nPlease check your host and port.", 0);
    }
    setClientState(this, nState);
}

void Client::login(const char* username, const char* password) {
    if (state != WAITUSER) return;
    setClientState(this, BUSY);
    State nState = WAITUSER;
    request(this, "USER", username);
    if (waitResCode(331, 5)) {
        request(this, "PASS", password);
    }
    if (waitResCode(230, 10)
        && request(this, "PWD", nullptr)
        && waitResCode(257, 5)) {
        nState = NORM;
        emit setRemoteRoot(rootPath);
    }
    if (state != IDLE) setClientState(this, nState);
    if (state == NORM) emit showMsg("Client: login success.", 1);
    else emit showMsg("Client: login fail.", 0);
}

void Client::logout() {
    setClientState(this, BUSY);
    request(this, "QUIT", nullptr);
    if (waitResCode(221, 5)) {
        emit showMsg("Client: normally disconnect.", 1);
    }
    else emit showMsg("Client: Error occurs when quit.", 2);
    setClientState(this, IDLE);
}

void Client::refreshLocal(const char* path) {
    if (listDir(localFileList, path, "-l")) {
        emit showLocal(path, localFileList);
    }
}

void Client::refreshRemote(const char* path) {
    if (state != NORM) return;

    setClientState(this, BUSY);
    State nState = NORM;
    if (mode) {
        if (request(this, "PASV", nullptr) != -1
            && waitResCode(227, 5)
            && (dataConnfd = setupConn(ipAddr, port, 1)) != -1
            && request(this, "LIST", path) != -1
            && waitResCode(150, 5)
            && recvFileList(dataConnfd, remoteFileList) != -1
            && waitResCode(226, 5)) {
            close(dataConnfd);
            dataConnfd = -1;

            emit showRemote(path, remoteFileList);
            emit showMsg("Client: Refresh remote file list success.", 1);
        }
        else emit showMsg("Client: Fail to refresh remote file list.", 0);
    }
    else {
        memset(ipAddr, 0, 32);
        strcpy(ipAddr, "192.168.0.102");
        char param[MAXPARAM];
        if ((dataListenfd = setupListen(ipAddr, &port, 1)) != -1
            && request(this, "PORT", generatePortParam(param, ipAddr, port)) != -1
            && waitResCode(200, 5)
            && request(this, "LIST", path) != -1
            && waitResCode(150, 5)
            && (dataConnfd = waitConn(dataListenfd, 5)) != -1
            && recvFileList(dataConnfd, remoteFileList) != -1
            && waitResCode(226, 5)) {
            close(dataConnfd);
            close(dataListenfd);
            dataConnfd = -1;
            dataListenfd = -1;

            emit showRemote(path, remoteFileList);
            emit showMsg("Client: Refresh remote file list success.", 1);
        }
        else emit showMsg("Client: Fail to refresh remote file list.", 0);
    }


    if (state != IDLE) setClientState(this, nState);
}

void Client::putFile(const char* src, const char* dst) {
    if (state != NORM) return;

    setClientState(this, BUSY);
    State nState = NORM;
    FILE* file = fopen(src, "rb");
    if (!file) {
        emit showMsg("Client: Fail to open local file.", 0);
        if (state != IDLE) setClientState(this, nState);
        return;
    }
    if (mode) {
        if (request(this, "TYPE", "I") != -1
            && waitResCode(200, 5)
            && request(this, "PASV", nullptr) != -1
            && waitResCode(227, 5)
            && (dataConnfd = setupConn(ipAddr, port, 1)) != -1
            && request(this, "STOR", dst) != -1
            && waitResCode(150, 5)
            && sendFile(dataConnfd, file) != -1) {
            close(dataConnfd);
            dataConnfd = -1;
            if (waitResCode(226, 5)) emit showMsg("Client: Upload file success.", 1);
            else emit showMsg("Client: Upload finished without response from server.", 2);
        }
        else {
            close(dataConnfd);
            dataConnfd = -1;
            emit showMsg("Client: Fail to upload file.", 0);
        }
    }
    else {
        memset(ipAddr, 0, 32);
        strcpy(ipAddr, "192.168.0.102");
        char param[MAXPARAM];
        if (request(this, "TYPE", "I") != -1
            && waitResCode(200, 5)
            && (dataListenfd = setupListen(ipAddr, &port, 1)) != -1
            && request(this, "PORT", generatePortParam(param, ipAddr, port)) != -1
            && waitResCode(200, 5)
            && request(this, "STOR", dst) != -1
            && waitResCode(150, 5)
            && (dataConnfd = waitConn(dataListenfd, 5)) != -1
            && sendFile(dataConnfd, file) != -1) {
            close(dataConnfd);
            close(dataListenfd);
            dataConnfd = -1;
            dataListenfd = -1;

            if (waitResCode(226, 5)) emit showMsg("Client: Upload file success.", 1);
            else emit showMsg("Client: Upload finished without response from server.", 2);
        }
        else {
            close(dataConnfd);
            close(dataListenfd);
            dataConnfd = -1;
            dataListenfd = -1;
            emit showMsg("Client: Fail to upload file.", 0);
        }
    }
    fclose(file);
    if (state != IDLE) setClientState(this, nState);
}

void Client::getFile(const char* src, const char* dst) {
    if (state != NORM) return;

    setClientState(this, BUSY);
    State nState = NORM;
    FILE* file = fopen(dst, "wb");
    if (!file) {
        emit showMsg("Client: Fail to open local file.", 0);
        if (state != IDLE) setClientState(this, nState);
        return;
    }
    if (mode) {
        if (request(this, "TYPE", "I") != -1
            && waitResCode(200, 5)
            && request(this, "PASV", nullptr) != -1
            && waitResCode(227, 5)
            && (dataConnfd = setupConn(ipAddr, port, 1)) != -1
            && request(this, "RETR", src) != -1
            && waitResCode(150, 5)
            && recvFile(dataConnfd, file) != -1) {

            if (waitResCode(226, 5)) emit showMsg("Client: Download file success.", 1);
            else emit showMsg("Client: Download finished without response from server.", 2);
        }
        else emit showMsg("Client: Fail to upload file.", 0);
    }
    else {
        memset(ipAddr, 0, 32);
        strcpy(ipAddr, "192.168.0.102");
        char param[MAXPARAM];
        if (request(this, "TYPE", "I") != -1
            && waitResCode(200, 5)
            && (dataListenfd = setupListen(ipAddr, &port, 1)) != -1
            && request(this, "PORT", generatePortParam(param, ipAddr, port)) != -1
            && waitResCode(200, 5)
            && request(this, "RETR", src) != -1
            && waitResCode(150, 5)
            && (dataConnfd = waitConn(dataListenfd, 5)) != -1
            && recvFile(dataConnfd, file) != -1) {

            if (waitResCode(226, 5)) emit showMsg("Client: Download file success.", 1);
            else emit showMsg("Client: Download finished without response from server.", 2);
        }
        else emit showMsg("Client: Fail to upload file.", 0);
    }
    close(dataConnfd);
    close(dataListenfd);
    dataConnfd = -1;
    dataListenfd = -1;
    fclose(file);
    if (state != IDLE) setClientState(this, nState);
}

void Client::switchMode(int mode) {
    this->mode = mode;
}

void Client::removeRemote(const char* path, int type) {
    if (state != NORM) return;

    setClientState(this, BUSY);
    State nState = NORM;
    if (type == 1) {
        if (request(this, "RMD", path) != -1
            && waitResCode(250, 5)) {

            emit showMsg("Client: Remove directory success.", 1);
        }
        else emit showMsg("Client: Fail to remove directory.", 0);
    }

    if (state != IDLE) setClientState(this, nState);
}
