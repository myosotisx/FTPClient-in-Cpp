#ifndef CLIENT_UTIL_H
#define CLIENT_UTIL_H

#define __OSX__

#define MAXBUF 8192
#define MAXREQ 1024

int readBuf(int sockfd, void* buf);

int writeBuf(int sockfd, const void* buf, int len);

int setupConn(const char* ipAddr, int port, int opt);

int receive(int fd, char* recvBuf);

const char* findFinalLine(const char* response);

char* concatCmdNParam(char* request, const char* cmd, const char* param);

#endif // CLIENT_UTIL_H
