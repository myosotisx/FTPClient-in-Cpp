#include "client_pi.h"

#include "string.h"

#include <sys/time.h>
#include <unistd.h>

#include <ctype.h>

#include <QDebug>

int checkState(const char* response) {
    const char* finalLine;
    if (!(finalLine = searchFinalLine(response))) return -1;
    else if (finalLine[0] == '1') return 2;
    else if (finalLine[0] == '2') return 1; // 请求成功
    else if (finalLine[0] == '3') return 2; // 需要进一步操作
    else if (finalLine[0] == '4' || finalLine[0] == '5') return 0; // 请求失败
    else return -1;// 不合法响应
}

int request(Client* client, const char* cmd, const char* param) {
    int state = getClientState(client);
    if (!state) return 0;

    char request[MAXREQ];
    concatCmdNParam(request, cmd, param);

    timeval time;
    while (1) {
        gettimeofday(&time, nullptr);
        double timeuse = (1000000*time.tv_sec+time.tv_usec)/1000.0-getLastSendTime(client);
        if (timeuse > 500) break;
    }
    int res = writeBuf(getControlConnfd(client), request, strlen(request));
    setLastSendTime(client, time.tv_sec, time.tv_usec);
    if (res == -1) {
        setClientState(client, 0);
        return -1;
    }
    else return 1;
}

void resCodeMapper(Client* client, int resCode, const char* param) {
    if (resCode == 227) {
        char ipAddr[32];
        int port;
        if (searchIpAddrNPort(param, ipAddr, &port)) {
            setDataConnAddr(client, ipAddr, port);
        }
    }
    else if (resCode == 257) {
        char rootPath[MAXPATH];
        if (searchRootPath(param, rootPath)) {
            setRootPath(client, rootPath);
        }
    }
    setResCode(client, resCode);
}

