/*-----------------------------------------------------------------------------
* playerlist.c
* Jordan McCaskill
* CMPT 361
* Assignment 2
*
* All functions dealing with the creation and manipulation of the player list
-----------------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "playerlist.h"
#include "common.h"

/*initialize the player list*/
struct playerhead *playerinit()
{
	/*set size for the player list*/
	struct playerhead *list = malloc(sizeof(struct playerhead));

	if (list == NULL)
		exit(EXIT_FAILURE);

	list->first = NULL;
	list->last = NULL;
	list->size = 0;

	return list;
}

/*-----------------------------------------------------------------
* Function: playerNodeCreate
* Purpose: create a player node
* Parameters: name - name of the player
*             money - how much money they have
* Return: pointer to the node created
-----------------------------------------------------------------*/
static struct playernode *playerNodeCreate(char *name, uint32_t money)
{
	/*set space for the data*/
	struct playernode *node = malloc(sizeof(struct playernode));

	if (node == NULL)
		exit(EXIT_FAILURE);

	node->name = calloc(MAXNAMESIZE + 1, sizeof(char));

	if (node->name == NULL)
		exit(EXIT_FAILURE);

	strncpy(node->name, name, MAXNAMESIZE);

	node->cards = calloc(P_DECK_SIZE, sizeof(char));

	if (node->cards == NULL)
		exit(EXIT_FAILURE);

	/*assign values to the variables*/
	node->money = money;
	node->next = NULL;
	node->bet = 0;
	node->active = 0;
	node->player_addr = NULL;
	node->server_addr_len = NULL;

	return node;
}

/*-----------------------------------------------------------------
* Function: playerAppend
* Purpose: Append a player node to the list
* Parameters: list - the list of players
*             name - name of the player to be added
*             money - the players starting money
* Return: pointer to the node added
-----------------------------------------------------------------*/
static struct playernode *playerAppend(struct playerhead *list,
	char *name, uint32_t money)
{
	struct playernode *newNode = playerNodeCreate(name, money);
	/*add to empty list*/
	if (list->first == NULL) {
		list->first = newNode;
		list->last = newNode;
		list->size++;
	/*add to list with items*/
	} else {
		list->last->next = newNode;
		list->last = newNode;
		list->size++;
	}

	return newNode;
}

/*find a player*/
struct playernode *findPlayer(struct argdata *argdata, char *name)
{
	struct playernode *next;

	/*loop until name found or end of list*/
	for (next = argdata->players->first; next != NULL &&
		strncmp(next->name, name, MAXNAMESIZE) != 0; next = next->next);

	/*check if end of list*/
	if (next == NULL)
		next = playerAppend(argdata->players, name, argdata->money);

	return next;
}

/*-----------------------------------------------------------------
* Function: playerNodeDestroy
* Purpose: destroy the node of the player
* Parameters: node - the node to destroy
* Return: None
-----------------------------------------------------------------*/
static void playerNodeDestroy(struct playernode *node)
{
	/*free off all the values if necessary*/
	free(node->name);
	free(node->cards);
	if (node->player_addr != NULL)
		free(node->player_addr);
	if (node->server_addr_len != NULL)
		free(node->server_addr_len);

	/*free the node itself*/
	free(node);
}

/*destroy the player list*/
void playerListDestroy(struct playerhead *list)
{
	struct playernode *cur;
	/*loop through all the items in the list and destroy them*/
	for (cur = list->first; list->first != NULL; cur = list->first) {
		list->first = cur->next;
		playerNodeDestroy(cur);
	}

	/*free the list header*/
	free(list);
}
