#include "client_util.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <unistd.h>
#include <dirent.h>
#include <errno.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <regex.h>

#include <QDebug>

int readBuf(int sockfd, void* buf) {
    int readLen;
    int totLen = 0, bufp = 0;
    while (!bufp) {
        readLen = recv(sockfd, (char*)buf+bufp, MAXBUF-1-bufp, 0);
        if (readLen < 0) {
            printf("Error read(): %s(%d)\n", strerror(errno), errno);
            return -1;
        }
        else if (readLen == 0) {
            return 0;
        }
        else {
            totLen += readLen;
            bufp = readLen % (MAXBUF-1);
        }
    }
    return totLen;
}

int writeBuf(int sockfd, const void* buf, int len) {
    int sendLen;
    int bufp = 0;
    while (bufp < len) {
        // 不接收Broken pipe信号，防止进程意外退出
        #ifndef __OSX__
            sendLen = send(sockfd, buf+bufp, len-bufp, MSG_NOSIGNAL);
        #else
            sendLen = send(sockfd, (char*)buf+bufp, len-bufp, SO_NOSIGPIPE);
        #endif
        if (sendLen < 0) {
            printf("Error write(): %s(%d)\n", strerror(errno), errno);
            return -1;
        }
        else bufp += sendLen;
    }
    return bufp;
}

int setupConn(const char* ipAddr, int port, int opt) {
    struct sockaddr_in addrServer;
    int sockfd;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        // printf("Error socket(): %s(%d)\n", strerror(errno), errno);
        qDebug() << "Error socket(): " << strerror(errno) << "(" << errno << ")";
        return -1;
    }

    // 设置端口复用
    if (opt) {
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&opt, sizeof(opt));
    }
    // 不接收Broken pipe信号，防止进程意外退出
    int set = 1;
    #ifndef __OSX__
        setsockopt(dataConnfd, SOL_SOCKET, MSG_NOSIGNAL, (const void*)&set, sizeof(set));
    #else
        setsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, (const void*)&set, sizeof(set));
    #endif

    memset(&addrServer, 0, sizeof(addrServer));
    addrServer.sin_family = AF_INET;
    addrServer.sin_port = htons(port);
    if (inet_pton(AF_INET, ipAddr, &addrServer.sin_addr) <= 0) {
        // printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
        qDebug() << "Error inet_pton(): " << strerror(errno) << "(" << errno << ")";
        return -1;
    }

    if (connect(sockfd, (struct sockaddr*)&addrServer, sizeof(addrServer)) < 0) {
        // printf("Error connect(): %s(%d)\n", strerror(errno), errno);
        qDebug() << "Error connect(): " << strerror(errno) << "(" << errno << ")";
        return -1;
    }

    return sockfd;
}

int receive(int fd, char* recvBuf) {
    int len = readBuf(fd, recvBuf);
    if (len > 0) {
        recvBuf[len] = 0;
        return 1;
    }
    // else if (len == 0) return 0;
    else return -1;
}

char* concatCmdNParam(char* request, const char* cmd, const char* param) {
    memset(request, 0, MAXREQ);
    strcpy(request, cmd);
    if (param != nullptr) {
        strcat(request, " ");
        strcat(request, param);
    }
    strcat(request, "\r\n");
    return request;
}

void strReplace(char* str, char oldc, char newc) {
    int len = strlen(str);
    for (int i = 0;i < len;i++) {
        if (str[i] == oldc) str[i] = newc;
    }
}

void parseIpAddrNPort(char* param, char* ipAddr, int* port) {
    char p1[8];
    char p2[8];
    memset(p1, 0, sizeof(p1));
    memset(p2, 0, sizeof(p2));
    memset(ipAddr, 0, 32);
    int rfirst = -1;
    int i = strlen(param)-1;
    while (i >= 0) {
        if (param[i] == ',') {
            if (rfirst == -1) {
                rfirst = i;
                strncpy(p2, param+rfirst+1, strlen(param)-1-rfirst);
            }
            else {
                strncpy(p1, param+i+1, rfirst-i-1);
                strncpy(ipAddr, param, i);
                strReplace(ipAddr, ',', '.');
                *port = atoi(p1)*256+atoi(p2);
                return;
            }
        }
        i--;
    }
}

int searchIpAddrNPort(const char* str, char* ipAddr, int* port) {
    regmatch_t pMatch[1];
    regex_t reg;
    const char pattern[] = "([0-9]{1,3}[,]){5}[0-9]{1,3}";
    regcomp(&reg, pattern, REG_EXTENDED);

    if (regexec(&reg, str, 1, pMatch, 0) != REG_NOMATCH) {
        char formatAddr[32];
        memset(formatAddr, 0, 32);
        strncpy(formatAddr, str+pMatch[0].rm_so, pMatch[0].rm_eo-pMatch[0].rm_so);
        parseIpAddrNPort(formatAddr, ipAddr, port);
        qDebug() << ipAddr << *port;
        regfree(&reg);
        return 1;
    }
    else {
        qDebug() << "No match!";
        regfree(&reg);
        return -1;
    }
}

const char* searchFinalLine(const char* response) {
    int len = strlen(response);
    if (!(response[len-1] == '\n' && response[len-2] == '\r')) {
        return nullptr;
    }
    int p = len-3;
    while (p >= 0) {
        if (p && response[p] == '\n' && response[p-1] == '\r') break;
        else p--;
    }
    if (isdigit(response[p+1]) && isdigit(response[p+2])
    && isdigit(response[p+3]) && response[p+4] == ' ') {
        return response+p+1;
    }
    else {
        // 返回错误
        return nullptr;
    }
}


int getResCodeNParam(const char* response, int* stateCode, char* param) {
    const char* finalLine = searchFinalLine(response);
    if (!finalLine) return -1;
    char _stateCode[8];
    memset(_stateCode, 0, 8);
    memset(param, 0, MAXPARAM);
    strncpy(_stateCode, finalLine, 3);
    strcpy(param, finalLine+4);
    *stateCode = atoi(_stateCode);
    return 1;
}


