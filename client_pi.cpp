#include "client_pi.h"

#include "string.h"

#include <unistd.h>

#include <ctype.h>

#include <QDebug>


int recvFileList(int dataConnfd, char* fileList) {
    int readLen;
    readLen = readBuf(dataConnfd, fileList);
    close(dataConnfd);
    if (readLen == -1) return -1;
    else {
        fileList[readLen] = 0;
        return 1;
    }
    return -1;
}

int checkState(const char* response) {
    const char* finalLine;
    if (!(finalLine = findFinalLine(response))) return -1;
    else if (finalLine[0] == '1') return 2;
    else if (finalLine[0] == '2') return 1; // 请求成功
    else if (finalLine[0] == '3') return 2; // 需要进一步操作
    else if (finalLine[0] == '4' || finalLine[0] == '5') return 0; // 请求失败
    else return -1;// 不合法响应
}

int request(int fd, const char* cmd, const char* param) {
    char request[MAXREQ];
    concatCmdNParam(request, cmd, param);
    return writeBuf(fd, request, strlen(request));
}

const char* findFinalLine(const char* response) {
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
