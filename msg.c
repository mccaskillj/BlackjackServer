#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#include "msg.h"
#include "common.h"

/*Append an item to the message list*/
struct msghead *msginit()
{
	struct msghead *list = malloc(sizeof(struct msghead));

	if (list == NULL)
		exit(EXIT_FAILURE);

	list->first = NULL;
	list->last = NULL;
	list->size = 0;

	return list;
}

/*broadcast all messages*/
int msgBroadcast(int *fd, struct msghead *list)
{
	struct msgnode *pos;
	int retVal;
	/*loop through and send all messages*/
	for (pos = list->first; pos != NULL; pos = pos->next) {
		retVal = sendto(*fd, pos->message, MESPSIZE, 0,
			(struct sockaddr *)pos->client_addr, *pos->client_addr_len);
		while (retVal == -1 && errno == EINTR) {
			retVal = sendto(*fd, pos->message, MESPSIZE, 0,
				(struct sockaddr *)pos->client_addr, *pos->client_addr_len);
		}

		if (retVal == -1)
			perror("sendto:");
	}

	return 1;
}

/*-----------------------------------------------------------------
* Function: msgAppend
* Purpose: Append an item to the message list
* Parameters: id - player seat id number
*             seq - chat seq number
*             client_addr - connection information
*             client_addr_len - size of client addr
*             msg - the message sent
* Return: int for success or failure
-----------------------------------------------------------------*/
static struct msgnode *msgCreateNode(int id, uint16_t seq,
	struct sockaddr_storage **client_addr,
	socklen_t **client_addr_len, char *msg)
{
	struct msgnode *node = malloc(sizeof(struct msgnode));

	if (node == NULL)
		exit(EXIT_FAILURE);

	node->message = calloc(MESSAGE_SIZE, sizeof(char));

	if (node->message == NULL)
		exit(EXIT_FAILURE);

	node->next = NULL;
	node->prev = NULL;
	strncpy(node->message, msg, MESSAGE_SIZE - 1);
	node->id = id;
	node->seq = seq;
	node->client_addr = *client_addr;
	node->client_addr_len = *client_addr_len;

	return node;
}

/*Append an item to the message list*/
int msgAppend(struct msghead *list, int id, uint16_t seq,
	struct sockaddr_storage **client_addr,
	socklen_t **client_addr_len, char *msg)
{
	struct msgnode *node = msgCreateNode(id, seq,
		client_addr, client_addr_len, msg);

	/*add to empty list*/
	if (list->size == 0) {
		list->first = node;
		list->last = node;
		list->size++;
	/*add to list with at least one item*/
	} else {
		list->last->next = node;
		node->prev = list->last;
		list->last = node;
		list->size++;
	}

	return 1;
}

/*-----------------------------------------------------------------
* Function: msgAppend
* Purpose: Append an item to the message list
* Parameters: list - list of messages needing acks
*             node - the item in the list to remove
* Return: int for success or failure
-----------------------------------------------------------------*/
static int msgPop(struct msghead *list, struct msgnode *node)
{
	/*remove from list with only one item*/
	if (list->size == 1) {
		list->first = NULL;
		list->last = NULL;
		node->next = NULL;
		node->prev = NULL;
	/*remove if item is at the front*/
	} else if (node == list->first) {
		list->first = node->next;
		list->first->prev = NULL;
		node->next = NULL;
	/*remove if item is at the end*/
	} else if (node == list->last) {
		list->last = node->prev;
		list->last->next = NULL;
		node->prev = NULL;
	/*remove if item is in the middle*/
	} else {
		node->prev->next = node->next;
		node->next->prev = node->prev;
		node->next = NULL;
		node->prev = NULL;
	}

	node->next = NULL;
	node->prev = NULL;
	node->client_addr = NULL;
	node->client_addr_len = NULL;
	free(node->message);
	node->message = NULL;
	free(node);
	node = NULL;

	list->size--;

	return 1;
}

/*Remove all messages for a player from the list*/
int msgRemovePlayer(struct msghead *list, int id)
{
	struct msgnode *pos, *next;
	for (pos = list->first; pos != NULL; pos = next) {
		next = pos->next;

		if (pos->id == id)
			msgPop(list, pos);
	}

	return 1;
}

/*Remove a single message from the list*/
int msgRemoveEvent(struct msghead *list, int id, uint16_t seq)
{
	struct msgnode *pos, *next;

	for (pos = list->first; pos != NULL; pos = next) {
		next = pos->next;
		if (pos->id == id && pos->seq == seq)
			msgPop(list, pos);
	}

	return 1;
}

/*Destroy the message list*/
int msgDestroy(struct msghead *list)
{
	while (list->size != 0)
		msgPop(list, list->first);

	free(list);
	list = NULL;

	return 1;
}
