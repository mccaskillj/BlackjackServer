/*-----------------------------------------------------------------------------
* common.c
* Jordan McCaskill
* CMPT 361
* Assignment 2
*
* The build and destroy functions for the main game state struct
-----------------------------------------------------------------------------*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "common.h"
#include "decklist.h"
#include "playerlist.h"
#include "msg.h"
#include "game.h"

/*build argdata struct*/
struct argdata *argdataBuild()
{
	/*malloc space for data*/
	struct argdata *argdata = malloc(sizeof(struct argdata));

	if (argdata == NULL)
		exit(EXIT_FAILURE);

	argdata->port = calloc(8, sizeof(char));

	if (argdata->port == NULL)
		exit(EXIT_FAILURE);

	argdata->dCards = calloc(P_DECK_SIZE, sizeof(char));

	if (argdata->dCards == NULL)
		exit(EXIT_FAILURE);

	argdata->seats = malloc(sizeof(struct playernode *) * MAX_PLAYERS);

	if (argdata->seats == NULL)
		exit(EXIT_FAILURE);

	/*set all values to their defaults*/
	argdata->gameSeq = 0;
	argdata->chatSeq = 0;
	argdata->decks = DEF_DECKS;
	argdata->money = DEF_MONEY;
	argdata->timer = DEF_TIMER;
	argdata->mBet = DEF_MBET;
	argdata->aPlayer = 0;
	argdata->gamestate = 0;
	argdata->startTime = 0;
	argdata->rebroadcast = 0;
	strcpy(argdata->port, "4420");
	/*initialize lists*/
	argdata->discard = deckinit(0);
	argdata->players = playerinit();
	argdata->msglist = msginit();
	/*set all the seats to null*/
	for (int i = 0; i < MAX_PLAYERS; i++)
		argdata->seats[i] = NULL;

	return argdata;
}

/*destroy argdata struct*/
int argdataDestroy(struct argdata *argdata)
{
	/*free all the malloc'd variables*/
	free(argdata->port);
	free(argdata->seats);
	free(argdata->dCards);
	/*destroy the lists*/
	deckDestroy(argdata->mainDeck);
	deckDestroy(argdata->discard);
	playerListDestroy(argdata->players);
	msgDestroy(argdata->msglist);
	/*set free'd pointers to null*/
	argdata->port = NULL;
	argdata->discard = NULL;
	argdata->players = NULL;
	argdata->seats = NULL;
	argdata->dCards = NULL;
	/*free the struct and set to null*/
	free(argdata);
	argdata = NULL;

	return 1;
}

/*-----------------------------------------------------------------
* Function: printCard
* Purpose: print a human readable value for the card
* Parameters: card - card to print out
* Return: int for success or failure
-----------------------------------------------------------------*/
static int printCard(char card)
{
	char value;
	/*get the cards value*/
	int val = ((card - 1) % 13) + 1;
	/*get the cards suit*/
	int suit = ((card - 1) % 4);

	/*convert the value to a readable format*/
	switch (val) {
	case ACE:
		value = 'A';
		break;
	case TWO:
		value = '2';
		break;
	case THREE:
		value = '3';
		break;
	case FOUR:
		value = '4';
		break;
	case FIVE:
		value = '5';
		break;
	case SIX:
		value = '6';
		break;
	case SEVEN:
		value = '7';
		break;
	case EIGHT:
		value = '8';
		break;
	case NINE:
		value = '9';
		break;
	case TEN:
		value = 'T';
		break;
	case JACK:
		value = 'J';
		break;
	case QUEEN:
		value = 'Q';
		break;
	case KING:
		value = 'K';
		break;
	}

	/*convert the suit to a readable format and print*/
	switch (suit) {
	case HEARTS: /*Hearts*/
		printf("%c%c ", value, 'H');
		break;
	case DIAMONDS: /*Diamonds*/
		printf("%c%c ", value, 'D');
		break;
	case SPADES: /*Spades*/
		printf("%c%c ", value, 'S');
		break;
	case CLUBS: /*Clubs*/
		printf("%c%c ", value, 'C');
		break;
	}

	return 1;
}

/*-----------------------------------------------------------------
* Function: printDeck
* Purpose: Print a deck in human readable format
* Parameters: deck - the deck to print out
*             separator - a separator to use when moving to
*                         another line
* Return: int for success or failure
-----------------------------------------------------------------*/
static int printDeck(char *deck, char *separator)
{
	/*loop through all cards*/
	for (int i = 0; i < P_DECK_SIZE; i++) {
		/*break if a null is found*/
		if (deck[i] == '\0')
			break;

		/*print the card and add the seperator if required*/
		printCard(deck[i]);
		if (i != 0 && i % 11 == 0)
			printf("%s", separator);
	}
	printf("\n");

	return 1;
}

/*-----------------------------------------------------------------
* Function: printPlayers
* Purpose: print the information for each player
* Parameters: argdata - the main game state struct
* Return: int for success or failure
-----------------------------------------------------------------*/
static int printPlayers(struct argdata *argdata)
{
	/*loop through the players*/
	for (int i = 0; i < MAX_PLAYERS; i++) {
		/*check if the seat is empty or not*/
		if (argdata->seats[i] == NULL) {
			printf("Seat %d: Empty\n\n", i + 1);
		} else {
			printf("Seat %d: %s\n", i + 1, argdata->seats[i]->name);
			printf("\tMoney: $%u\n", argdata->seats[i]->money);
			printf("\tBet: $%u\n", argdata->seats[i]->bet);
			printf("\tActive: %d\n", argdata->seats[i]->active);
			printf("\tCards: ");
			printDeck(argdata->seats[i]->cards, "\n\t\t");
			printf("\tDeck Total: %d\n\n", deckTotal(argdata->seats[i]->cards));
		}
	}

	return 1;
}

/*-----------------------------------------------------------------
* Function: printState
* Purpose: print out the state of the game
* Parameters: argdata - the main game state struct
* Return: int for success or failure
-----------------------------------------------------------------*/
int printState(struct argdata *argdata)
{
	printf("------------\n");
	printf("GAME SUMMARY\n");
	printf("------------\n");
	printf("Decks: %d\n", argdata->decks);
	printf("Starting Money: $%u\n", argdata->money);
	printf("Minimum Bet: $%u\n", argdata->mBet);
	printf("Timeout: %d\n", argdata->timer);
	printf("Port: %s\n", argdata->port);
	printf("Active Player: %u\n", argdata->aPlayer);
	printf("Game Sequence #: %u\n", argdata->gameSeq);
	printf("Chat Sequence #: %u\n", argdata->chatSeq);
	printf("Game State: %u\n", argdata->gamestate);
	printf("Dealer Cards: ");
	printDeck(argdata->dCards, "\n\t");
	printf("Deck Total: %d\n\n", deckTotal(argdata->dCards));
	printPlayers(argdata);

	return 1;
}
