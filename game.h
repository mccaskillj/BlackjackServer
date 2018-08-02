/*-----------------------------------------------------------------------------
* game.h
* Jordan McCaskill
* CMPT 361
* Assignment 2
*
* Defines and prototypes for game.c
-----------------------------------------------------------------------------*/

#ifndef GAME_H
#define GAME_H

#include <sys/socket.h>

#include "common.h"

/*Player states*/
#define ACTIVE 1
#define WAIT 0
#define KICK -1

/*play rules*/
#define DEALERHIT 16
#define BLACKJACK 21

/*game states*/
#define SETUP 0
#define BETTING 1
#define PLAY 2
#define WRAPUP 3
#define BROADCAST 4

/*-----------------------------------------------------------------
* Function: pConnect
* Purpose: connect a player to the game and put them in a seat
* Parameters: argdata - the main game state struct
*             client_addr - the clients address information
*             server_addr_len - the length of client_addr
*             response - the data read from the client
* Return: int for success or failure
-----------------------------------------------------------------*/
int pConnect(struct argdata *argdata, struct sockaddr_storage **client_addr,
	socklen_t **server_addr_len, char *response);

/*-----------------------------------------------------------------
* Function: changePlayer
* Purpose: Change from the current active player to the next
* Parameters: argdata - the main game state struct
* Return: int for success or failure
-----------------------------------------------------------------*/
int changePlayer(struct argdata *argdata);

/*-----------------------------------------------------------------
* Function: addToDeck
* Purpose: add a card to the specified deck
* Parameters: deck - the deck to add the card to
*             card - the card to add to the deck
* Return: int for success or failure
-----------------------------------------------------------------*/
int addToDeck(char *deck, char card);

/*-----------------------------------------------------------------
* Function: proceedGame
* Purpose: move the game on to the next players turn or handle the
*          setup, betting, broadcasting or tear down of the game
* Parameters: fd - file descriptor for the socket
*             argdata - the main game state struct
*             out - the output character array
* Return: int for success or failure
-----------------------------------------------------------------*/
int proceedGame(int *fd, struct argdata *argdata, char *out);

/*-----------------------------------------------------------------
* Function: deckTotal
* Purpose: Get the total of all the cards in a deck
* Parameters: deck - the deck to total up
* Return: int for success or failure
-----------------------------------------------------------------*/
int deckTotal(char *deck);

/*-----------------------------------------------------------------
* Function: broadcastBetting
* Purpose: broadcast the betting state to all players
* Parameters: fd - file descriptor for the socket
*             argdata - the main game state struct
*             out - the output character array
* Return: int for success or failure
-----------------------------------------------------------------*/
int broadcastBetting(int *fd, struct argdata *argdata, char *out);

#endif /*GAME_H*/
