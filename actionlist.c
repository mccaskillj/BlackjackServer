#include <stdlib.h>
#include <string.h>

#include "actionlist.h"
#include "common.h"

struct actionhead* actioninit(){
	struct actionhead* list = malloc(sizeof(struct actionhead));
	list->first = NULL;
	list->last = NULL;
	list->size = 0;

	return list;
}

struct actionnode* actionNodeCreate(uint8_t id, char* message, uint16_t seq, struct sockaddr_storage **addr, socklen_t **server_addr_len){
	struct actionnode *node = malloc(sizeof(struct actionnode));
	node->message = malloc(sizeof(char)*MAXPSIZE);
	strncpy(node->message, message, MAXPSIZE);
	node->player_addr = *addr;
	node->server_addr_len = *server_addr_len;
	node->gameSeq = seq;
	node->next = NULL;
	node->prev = NULL;

	return node;
}

int actionAppend(struct actionhead *list, uint8_t id, char* message, uint8_t seq, struct sockaddr_storage **addr, socklen_t **server_addr_len){
	struct actionnode *node = actionNodeCreate(id, message, seq, addr, server_addr_len);
	if (list->size == 0){
		list->first = node;
		list->last = node;
		list->size++;
	} else {
		node->prev = list->last;
		list->last->next = node;
		list->last = node;
	}

	return 1;
}

int actionPop(struct actionhead *list, struct actionnode *node){
	if (list->size == 1){
		list->first = NULL;
		list->last = NULL;
		node->next = NULL;
		node->prev = NULL;
	} else if (node == list->first){
		list->first = node->next;
		list->first->prev = NULL;
		node->next = NULL;
	} else if (node == list->last) {
		list->last = node->prev;
		list->last->next = NULL;
		node->prev = NULL;
	} else {
		node->prev->next = node->next;
		node->next->prev = node->prev;
		node->next = NULL;
		node->prev = NULL;
	}

	free(node->message);
	node->message = NULL;
	free(node);
	node = NULL;

	list->size--;

	return 1;
}

int actionRemove(struct actionhead *list, uint8_t id, uint16_t gameSeq){
	struct actionnode *pos = list->first;
	for (;pos != NULL && pos->gameSeq != gameSeq || pos->id != id; pos= pos->next);

	if (pos == NULL)
		return 1;

	actionPop(list, pos);

	return 1;
}