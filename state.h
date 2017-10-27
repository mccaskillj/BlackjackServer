#ifndef STATE_H
#define STATE_H

#include "common.h" /*for argdata struct*/

int generateState(struct argdata *argData, char opcode, char *out);
int opSwitch(int *fd, struct argdata *argdata, char* response, char *out,
			struct sockaddr_storage **client_addr, socklen_t **server_addr_len);

#endif /*STATE_H*/
