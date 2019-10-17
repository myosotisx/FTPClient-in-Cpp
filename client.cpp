#include "client.h"

#include <QDebug>

Client::Client(QObject* parent): QObject(parent) {

}

void Client::setupControlConn(const char* ipAddr, int port) {
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
    if (requestUSER(controlConnfd, username) != -1
        && receive(controlConnfd, recvBuf) != -1) {
        qDebug() << recvBuf;
        emit showMsg(recvBuf);
        int state = checkState(recvBuf);
        if (state == 1) {
            emit setState(NORM);
        }
        else if (checkState(recvBuf) == 2
            && requestPASS(controlConnfd, password) != -1
            && receive(controlConnfd, recvBuf) != -1) {
            qDebug() << recvBuf;
            emit showMsg(recvBuf);
            emit setState(checkState(recvBuf));
        }
        else emit setState(ERRORQUIT);
    }
    else emit setState(ERRORQUIT);
}

void Client::logout() {
    if (requestQUIT(controlConnfd) != -1 && receive(controlConnfd, recvBuf) != -1) {
        qDebug() << recvBuf;
        emit showMsg(recvBuf);
        if (checkState(recvBuf) == 1) emit setState(NORMQUIT);
        else emit setState(ERRORQUIT);
    }
    else emit setState(ERRORQUIT);
}


