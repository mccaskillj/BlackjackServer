/*-----------------------------------------------------------------------------
* playerlist.h
* Jordan McCaskill
* CMPT 361
* Assignment 2
*
* Structs and prototypes for playerlist.c
-----------------------------------------------------------------------------*/

#ifndef PLAYERLIST_H
#define PLAYERIST_H

#include <sys/socket.h>

#include "common.h" /*argdata struct*/

/*struct for the nodes of the playerlist*/
struct playernode {
	struct playernode *next;
	char *name;
	uint32_t money;
	uint32_t bet;
	char *cards;
	int active;
	struct sockaddr_storage *player_addr;
	socklen_t *server_addr_len;
};

/*struct for the header of the player list*/
struct playerhead {
	int size;
	struct playernode *first;
	struct playernode *last;
};

/*-----------------------------------------------------------------
* Function: playerinit
* Purpose: Create the player list
* Parameters: None
* Return: pointer to the header of the list
-----------------------------------------------------------------*/
struct playerhead *playerinit();

/*-----------------------------------------------------------------
* Function: findPlayer
* Purpose: find the data associated to a player's name or create a
*          new entry if the name does not exist
* Parameters: argdata - the main game state struct
*             name - character array for the name
* Return: the pointer to the node associated with the name
-----------------------------------------------------------------*/
struct playernode *findPlayer(struct argdata *argdata, char *name);

/*-----------------------------------------------------------------
* Function: playerListDestroy
* Purpose: destroy the player list
* Parameters: list - the list to destroy
* Return: None
-----------------------------------------------------------------*/
void playerListDestroy(struct playerhead *list);

#endif /*PLAYERLIST_H*/
