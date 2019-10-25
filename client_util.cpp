#include "client_util.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <unistd.h>
#include <dirent.h>
#include <errno.h>

#include <string.h>
#include <stdlib.h>

#include <regex.h>

#include <QDebug>

int readBuf(int sockfd, void* buf) {
    int readLen;
    int totLen = 0, bufp = 0;
    while (!bufp) {
        readLen = recv(sockfd, (char*)buf+bufp, MAXBUF-1-bufp, 0);
        if (readLen < 0) {
            // printf("Error read(): %s(%d)\n", strerror(errno), errno);
            qDebug() << "Error read():" << strerror(errno) << "(" << errno << ")";
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
            // printf("Error write(): %s(%d)\n", strerror(errno), errno);
            qDebug() << "Error write():" << strerror(errno) << "(" << errno << ")";
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

int setupListen(char* ipAddr, int* port, int opt) {
    struct sockaddr_in addr;
    int listenfd;

    if ((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        printf("Error socket(): %s(%d)\n", strerror(errno), errno);
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    // addr.sin_port = htons(port);
    addr.sin_port = htons(0);
    addr.sin_addr.s_addr = inet_addr(ipAddr);

    // 设置端口复用
    if (opt) {
        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&opt, sizeof(opt));
    }
    // 不接收Broken pipe信号，防止进程意外退出
    int set = 1;
    #ifndef __OSX__
        setsockopt(listenfd, SOL_SOCKET, MSG_NOSIGNAL, (const void*)&set, sizeof(set));
    #else
        setsockopt(listenfd, SOL_SOCKET, SO_NOSIGPIPE, (const void*)&set, sizeof(set));
    #endif

    if (bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        // printf("Error bind(): %s(%d)\n", strerror(errno), errno);
        qDebug() << "Error bind():" << strerror(errno) << "(" << errno << ")";
        return -1;
    }

    socklen_t len = sizeof(addr);
    if (getsockname(listenfd, (struct sockaddr*)&addr, &len)) {
        qDebug() << "Error getsockname():" << strerror(errno) << "(" << errno << ")";
        return -1;
    }
    *port = ntohs(addr.sin_port);

    if (listen(listenfd, BACKLOG) == -1) {
        printf("Error listen(): %s(%d)\n", strerror(errno), errno);
        return -1;
    }
    return listenfd;
}

int acceptNewConn(int listenfd) {
    int newfd;
    if ((newfd = accept(listenfd, NULL, NULL)) == -1) {
        // printf("Error accept(): %s(%d)\n", strerror(errno), errno);
        qDebug() << "Error accept():" << strerror(errno) << errno;
        return -1;
    }
    else return newfd;
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
        regfree(&reg);
        return -1;
    }
}

int searchRootPath(const char* str, char* rootPath) {
    regmatch_t pMatch[1];
    regex_t reg;
    const char pattern[] = "\".*\"";
    regcomp(&reg, pattern, REG_EXTENDED);
    if (regexec(&reg, str, 1, pMatch, 0) != REG_NOMATCH) {
        memset(rootPath, 0, MAXPATH);
        strncpy(rootPath, str+pMatch[0].rm_so+1, pMatch[0].rm_eo-pMatch[0].rm_so-2);
        regfree(&reg);
        return 1;
    }
    else {
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

char* listDir(char* fileList, const char* path, const char* param) {
    char cmd[MAXCMD];
    char tmp[MAXLINE];
    int lineLen;
    int totLen = 0;
    memset(fileList, 0, MAXBUF);

    memset(cmd, 0, MAXCMD);
    strcpy(cmd, "cd ");
    strcat(cmd, path);
    strcat(cmd, " 2>&1");
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return nullptr;
    if (fgets(tmp, MAXLINE, pipe)) {
        pclose(pipe);
        return nullptr;
    }
    pclose(pipe);

    memset(cmd, 0, MAXCMD);
    strcpy(cmd, "cd ");
    strcat(cmd, path);
    strcat(cmd, "; ls ");
    strcat(cmd, param);
    pipe = popen(cmd, "r");
    if (!pipe) return nullptr;

    while (fgets(tmp, MAXLINE, pipe)) {
        lineLen = strlen(tmp);
        if (tmp[lineLen-1] == '\n' && tmp[lineLen-2] != '\r') {
            tmp[lineLen-1] = '\r';
            tmp[lineLen] = '\n';
            tmp[lineLen+1] = 0;
            lineLen += 1;
        }
        if ((totLen += lineLen) > MAXBUF) break; // 此处限制了获取列表的最大长度
        else strcat(fileList, tmp);
    }
    pclose(pipe);
    return fileList;
}

char* generatePortParam(char* param, const char* ipAddr, int port) {
    char _ipAddr[32];
    memset(param, 0, MAXPARAM);
    memset(_ipAddr, 0, 32);
    strcpy(_ipAddr, ipAddr);
    int p1, p2;
    p1 = port/256;
    p2 = port%256;
    strReplace(_ipAddr, '.', ',');
    sprintf(param, "%s,%d,%d", _ipAddr, p1, p2);
    qDebug() << param;
    return param;
}

int recvFileList(int dataConnfd, char* fileList) {
    int readLen;
    readLen = readBuf(dataConnfd, fileList);
    close(dataConnfd);
    if (readLen == -1) return -1;
    else {
        fileList[readLen] = 0;
        return 1;
    }
}

int sendFile(Client* client, int dataConnfd, FILE* file) {
    unsigned char fileBuf[MAXBUF];
    int readLen, writeLen = 0;
    long long totLen = 0;
    while ((readLen = fread(fileBuf, sizeof(unsigned char), MAXBUF, file))) {
        if ((writeLen = writeBuf(dataConnfd, fileBuf, readLen)) == -1) {
            break;
        }
        totLen += readLen;
        updateProgress(client, totLen);
    }
    if (writeLen == -1) return -1;
    else return 1;
}

int recvFile(Client* client, int dataConnfd, FILE* file) {
    unsigned char fileBuf[MAXBUF];
    int readLen;
    long long totLen = 0;
    while ((readLen = read(dataConnfd, fileBuf, MAXBUF))) {
        if (readLen == -1) break;
        fwrite(fileBuf, sizeof(unsigned char), readLen, file);
        totLen += readLen;
        updateProgress(client, totLen);
    }
    if (readLen == -1) return -1;
    else return 1;
}

