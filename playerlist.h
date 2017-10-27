#ifndef PLAYERLIST_H
#define PLAYERIST_H

#include <sys/socket.h>

#include "common.h" /*argdata struct*/


struct playernode
{
	struct playernode *next;
	char *name;
	uint32_t money;
	uint32_t bet;
	char *cards;
	int active;
	struct sockaddr_storage *player_addr;
	socklen_t *server_addr_len;
};

struct playerhead
{
	int size;
	struct playernode *first;
	struct playernode *last;
};

struct playerhead *playerinit();
struct playernode *playerNodeCreate(char *name, uint32_t money);
struct playernode *playerAppend(struct playerhead *list, char *name, uint32_t money);
struct playernode * findPlayer(struct argdata *argdata, char *name);
void playerNodeDestroy(struct playernode *node);
void playerListDestroy(struct playerhead *list);

/*PLAYERLIST_H*/
#endif