#ifndef GAME_H
#define GAME_H

#include <sys/socket.h>

#include "common.h"

int pConnect(struct argdata *argdata, struct sockaddr_storage **client_addr, socklen_t **server_addr_len, char* response);
int changePlayer(struct argdata *argdata);
int addToDeck(char* deck, char card);

#endif /*GAME_H*/