#include "client_pi.h"
#include "client_util.h"

#include "string.h"

int checkState(const char* response) {
    const char* finalLine;
    if (!(finalLine = findFinalLine(response))) return -1;
    else if (finalLine[0] == '2') return 1; // 请求成功
    else if (finalLine[0] == '3') return 2; // 需要进一步操作
    else if (finalLine[0] == '4' || finalLine[0] == '5') return 0; // 请求失败
    else return -1;// 不合法响应
}

int requestUSER(int fd, const char* username) {
    char request[MAXREQ];
    concatCmdNParam(request, "USER", username);
    return writeBuf(fd, request, strlen(request));
}

int requestPASS(int fd, const char* password) {
    char request[MAXREQ];
    concatCmdNParam(request, "PASS", password);
    return writeBuf(fd, request, strlen(request));
}

int requestQUIT(int fd) {
    char request[MAXREQ];
    concatCmdNParam(request, "QUIT", nullptr);
    return writeBuf(fd, request, strlen(request));
}
