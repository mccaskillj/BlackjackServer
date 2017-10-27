#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "playerlist.h"
#include "common.h"

struct playerhead *playerinit(){
	struct playerhead *list = malloc(sizeof(struct playerhead));
	list->first = NULL;
	list->last = NULL;
	list->size = 0;
	
	return list;
}

struct playernode *playerNodeCreate(char *name, uint32_t money){
	struct playernode *node = malloc(sizeof(struct playernode));
	node->name = calloc(12, sizeof(char));
	strncpy(node->name,name,12);
	node->cards = calloc(21, sizeof(char));
	node->money = money;
	node->next = NULL;
	node->bet = 0;
	node->active = 0;
	node->player_addr = NULL;
	node->server_addr_len = NULL;

	return node;
}

struct playernode *playerAppend(struct playerhead *list, char *name, uint32_t money){
	struct playernode *newNode = playerNodeCreate(name, money);
	if (list->first == NULL){
		list->first = newNode;
		list->last = newNode;
		list->size++;
	}
	else
	{
		list->last->next = newNode;
		list->last = newNode;
		list->size++;
	}

	return newNode;
}

struct playernode * findPlayer(struct argdata *argdata, char *name){
	struct playernode *next;
	for (next = argdata->players->first; next != NULL && strncmp(next->name, name,12) != 0; next=next->next);

	if (next == NULL){
		next = playerAppend(argdata->players, name, argdata->money);
	}

	return next;
}

void playerNodeDestroy(struct playernode *node){
	free(node->name);
	free(node->cards);
	if(node->player_addr != NULL)
		free(node->player_addr);
	if(node->server_addr_len != NULL)
		free(node->server_addr_len);
	free(node);
}

void playerListDestroy(struct playerhead *list){
	struct playernode *cur;
	for (cur = list->first; list->first!= NULL; cur = list->first){
		list->first = cur->next;
		playerNodeDestroy(cur);
	}

	free(list);
}