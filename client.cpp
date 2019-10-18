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
            pthread_testcancel();
            recvBuf[recvLen] = 0;
            emit client->showMsg(recvBuf);
            int resCode;
            char param[MAXPARAM];
            if (getResCodeNParam(recvBuf, &resCode, param) != -1) {
                resCodeMapper(client, resCode, param);
            }
            recvBuf = client->nextBuf();
        }
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

Client::Client(QObject* parent):
    QObject(parent)
    , state(IDLE)
    , resCode(-1)
    , controlConnfd(-1)
    , dataConnfd(-1)
    , mode(1)
    , bufp(0) {
}

char* Client::nextBuf() {
    char* recvBuf = buf[bufp];
    bufp = (bufp+1)%BUFCNT;
    return recvBuf;
}

int Client::waitResCode(int resCode, double timeout) {
    timeval starttime,endtime;
    gettimeofday(&starttime, nullptr);
    while (1) {
        if (this->resCode == resCode) return 1;
        gettimeofday(&endtime, nullptr);
        double timeuse = 1000000*(endtime.tv_sec-starttime.tv_sec)
                         +endtime.tv_usec-starttime.tv_usec;
        if (timeuse/1000 > 1000*timeout) return 0;
    }
}

void Client::setupControlConn(const char* ipAddr, int port) {
    setClientState(this, BUSY);
    State nState = IDLE;
    if ((controlConnfd = setupConn(ipAddr, port, 1)) != -1) {
        pthread_create(&controlThread, nullptr, &receiver, this);

        if (waitResCode(220, 2)) {
            nState = WAITUSER;
            emit reqUserInfo();
        }
        else {
            qDebug() << "服务器响应超时！";
        }

    }
    else {
        qDebug() << "连接服务器失败!";
    }
    setClientState(this, nState);
}

void Client::login(const char* username, const char* password) {
    if (state != WAITUSER) return;
    setClientState(this, BUSY);
    State nState = WAITUSER;
    request(this, "USER", username);
    if (waitResCode(331, 2)) {
        request(this, "PASS", password);
    }
    if (waitResCode(230, 10)) nState = NORM;
    if (state != IDLE) setClientState(this, nState);
}

void Client::logout() {
    setClientState(this, BUSY);
    request(this, "QUIT", nullptr);
    waitResCode(221, 2);
    setClientState(this, IDLE);
}

void Client::refresh() {
    if (state != NORM) return;

    setClientState(this, BUSY);
    State nState = NORM;
    if (request(this, "PASV", nullptr) != -1
        && waitResCode(227, 5)
        && (dataConnfd = setupConn(ipAddr, port, 1)) != -1
        && request(this, "LIST", nullptr) != -1
        && waitResCode(150, 5)
        && recvFileList(dataConnfd, fileList) != -1
        && waitResCode(226, 5)) {
        close(dataConnfd);
        dataConnfd = -1;

        emit showFileList(fileList);
    }
    else qDebug() << "接收文件列表失败！";
    if (state != IDLE) setClientState(this, nState);
}



