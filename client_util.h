#ifndef CLIENT_UTIL_H
#define CLIENT_UTIL_H

#define __OSX__

#define MAXBUF 8192
#define MAXREQ 1024
#define MAXPARAM 4096
#define MAXPATH 1024
#define MAXLINE 4096
#define MAXCMD 1024
#define BACKLOG 10

#include <stdio.h>

///////////////////////////////////////////////////////////////////////////////////////////////////

// 用户自定义类

class Client;

int getClientState(Client* client);

void setClientState(Client* client, int state);

int getControlConnfd(Client* client);

void setDataConnAddr(Client* client, const char* ipAddr, int port);

void setResCode(Client* client, int resCode);

void setRootPath(Client* client, const char* rootPath);

///////////////////////////////////////////////////////////////////////////////////////////////////

int readBuf(int sockfd, void* buf);

int writeBuf(int sockfd, const void* buf, int len);

int setupConn(const char* ipAddr, int port, int opt);

int setupListen(char* ipAddr, int* port, int opt);

int acceptNewConn(int listenfd);

int receive(int fd, char* recvBuf);

char* concatCmdNParam(char* request, const char* cmd, const char* param);

void strReplace(char* str, char oldc, char newc);

void parseIpAddrNPort(char* param, char* ipAddr, int* port);

int searchIpAddrNPort(const char* str, char* ipAddr, int* port);

int searchRootPath(const char* str, char* rootPath);

const char* searchFinalLine(const char* response);

int getResCodeNParam(const char* response, int* stateCode, char* param);

char* generatePortParam(char* param, const char* ipAddr, int port);

char* listDir(char* fileList, const char* path, const char* param);

int recvFileList(int dataConnfd, char* fileList);

int sendFile(int dataConnfd, FILE* file);

int recvFile(int dataConnfd, FILE* file);

#endif // CLIENT_UTIL_H
