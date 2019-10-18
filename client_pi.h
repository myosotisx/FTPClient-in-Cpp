#ifndef CLIENT_PI_H
#define CLIENT_PI_H

#include "client_util.h"

int recvFileList(int dataConnfd, char* fileList);

int checkState(const char* response);

int request(int fd, const char* cmd, const char* param);

const char* findFinalLine(const char* response);

#endif // CLIENT_PI_H
