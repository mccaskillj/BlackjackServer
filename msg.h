#ifndef MSG_H
#define MSG_H

#include <stdint.h>
#include <sys/socket.h>

#define MESSAGE_SIZE 144

struct msgnode {
	struct msgnode *next;
	struct msgnode *prev;
	int id;
	uint16_t seq;
	struct sockaddr_storage *client_addr;
	socklen_t *client_addr_len;
	char *message;
};

struct msghead {
	struct msgnode *first;
	struct msgnode *last;
	int size;
};

/*-----------------------------------------------------------------
* Function: msginit
* Purpose: Create the list of messages
* Parameters: None
* Return: pointer to new msghead
-----------------------------------------------------------------*/
struct msghead *msginit();

/*-----------------------------------------------------------------
* Function: msgAppend
* Purpose: Append an item to the message list
* Parameters: list - list of messages needing acks
*             id - player seat id number
*             seq - chat seq number
*             client_addr - connection information
*             client_addr_len - size of client addr
*             msg - the message sent
* Return: int for success or failure
-----------------------------------------------------------------*/
int msgAppend(struct msghead *list, int id, uint16_t seq,
	struct sockaddr_storage **client_addr,
	socklen_t **client_addr_len, char *msg);

/*-----------------------------------------------------------------
* Function: msgRemovePlayer
* Purpose: Remove all messages for a player from the list
* Parameters: list - list of messages needing acks
*             id - player seat id number
* Return: int for success or failure
-----------------------------------------------------------------*/
int msgRemovePlayer(struct msghead *list, int id);

/*-----------------------------------------------------------------
* Function: msgRemoveEvent
* Purpose: Remove a single message from the list
* Parameters: list - list of messages needing acks
*             id - player seat id number
*             seq - chat seq number
* Return: int for success or failure
-----------------------------------------------------------------*/
int msgRemoveEvent(struct msghead *list, int id, uint16_t seq);

/*-----------------------------------------------------------------
* Function: msgDestroy
* Purpose: Destroy the message list
* Parameters: list - list of messages needing acks
* Return: int for success or failure
-----------------------------------------------------------------*/
int msgDestroy(struct msghead *list);

/*-----------------------------------------------------------------
* Function: msgBroadcast
* Purpose: broadcast all messages
* Parameters: fd - file descriptor
*             list - list of messages needing acks
* Return: int for success or failure
-----------------------------------------------------------------*/
int msgBroadcast(int *fd, struct msghead *list);

#endif /*MSG_H*/
