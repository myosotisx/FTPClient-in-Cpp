#ifndef CLIENT_PI_H
#define CLIENT_PI_H

int checkState(const char* response);

int requestUSER(int fd, const char* param);

int requestPASS(int fd, const char* param);

int requestQUIT(int fd);

#endif // CLIENT_PI_H
