#ifndef CLIENT_PI_H
#define CLIENT_PI_H

#include "client_util.h"

int checkState(const char* response);

int request(Client* client, const char* cmd, const char* param);

int resCodeMapper(Client* client, int resCode, const char* param);

#endif // CLIENT_PI_H
