#ifndef ACTIONLIST_H
#define ACTIONLIST_H

#include <stdint.h> /*uintxx_t*/
#include <sys/socket.h>

struct actionhead
{
	struct actionnode *first;
	struct actionnode *last;
	int size;
};

struct actionnode
{
	uint8_t id;
	struct sockaddr_storage *player_addr;
	socklen_t *server_addr_len;
	char *message;
	uint16_t gameSeq;
	struct actionnode *prev;
	struct actionnode *next;
};

#endif /*ACTIONLIST_H*/