#include "client.h"

#include <unistd.h>

#include <QDebug>



Client::Client(QObject* parent):
    QObject(parent)
    , controlConnfd(-1)
    , dataConnfd(-1)
    , mode(1)
    , bufp(0) {
}

void Client::setupControlConn(const char* ipAddr, int port) {
    char* recvBuf = nextBuf();
    if ((controlConnfd = setupConn(ipAddr, port, 1)) != -1
            && receive(controlConnfd, recvBuf) != -1) {
        qDebug() << recvBuf;
        emit showMsg(recvBuf);
        if (checkState(recvBuf) == 1) emit setState(WAITUSER);
        else emit setState(ERRORQUIT);

    }
    else emit setState(ERRORQUIT);
}

void Client::login(const char* username, const char* password) {
    char* recvBuf = nextBuf();
    if (request(controlConnfd, "USER", username) != -1
        && receive(controlConnfd, recvBuf) != -1) {
        qDebug() << recvBuf;
        emit showMsg(recvBuf);
        int state = checkState(recvBuf);

        recvBuf = nextBuf();
        if (state == 1) {
            emit setState(NORM);
        }
        else if (state == 2
            && request(controlConnfd, "PASS", password) != -1
            && receive(controlConnfd, recvBuf) != -1) {
            qDebug() << recvBuf;
            emit showMsg(recvBuf);
            // 处理密码错误
            emit setState(NORM);
        }
        else if (state != -1) emit setState(WAITUSER);
        else emit setState(ERRORQUIT);
    }
    else emit setState(ERRORQUIT);
}

void Client::logout() {
    char* recvBuf = nextBuf();
    if (request(controlConnfd, "QUIT", nullptr) != -1
        && receive(controlConnfd, recvBuf) != -1) {
        qDebug() << recvBuf;
        emit showMsg(recvBuf);
        if (checkState(recvBuf) == 1) emit setState(NORMQUIT);
        else emit setState(ERRORQUIT);
    }
    else emit setState(ERRORQUIT);
}

void Client::refresh() {
    char* recvBuf = nextBuf();
    if (mode) {
        if (request(controlConnfd, "PASV", nullptr) != -1
            && receive(controlConnfd, recvBuf) != -1) {
            emit showMsg(recvBuf);
            int state = checkState(recvBuf);
            if (state == 1) {
                // 处理参数
                char dataIpAddr[32];
                int dataPort;
                if (searchIpAddrNPort(recvBuf, dataIpAddr, &dataPort) != -1
                    && (dataConnfd = setupConn(dataIpAddr, dataPort, 1)) != -1) {

                    recvBuf = nextBuf();

                    /*if (receive(controlConnfd, recvBuf) != -1) {

                        emit showMsg(recvBuf);
                        state = checkState(recvBuf);
                        recvBuf = nextBuf();
                        if (state == 2
                            && request(controlConnfd, "LIST", nullptr) != -1
                            && receive(controlConnfd, recvBuf) != -1
                            && recvFileList(dataConnfd, fileList) != -1) emit showFileList(fileList);
                    }*/
                    if (request(controlConnfd, "LIST", nullptr) != -1
                        && receive(controlConnfd, recvBuf) != -1) {
                        emit showMsg(recvBuf);
                        recvFileList(dataConnfd, fileList);
                        emit showFileList(fileList);
                    }

                }
            }
        }
    }
}


