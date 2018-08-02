/*-----------------------------------------------------------------------------
* decklist.h
* Jordan McCaskill
* CMPT 361
* Assignment 2
*
* Defines, structs and prototypes for decklist.c
-----------------------------------------------------------------------------*/

#ifndef DECKLIST_H
#define DECKLIST_H

#define DECKSIZE 52

/*struct for the nodes of the linked list*/
struct decknode {
	struct decknode *next;
	char val;
};

/*struct for the header of the linked list*/
struct deckhead {
	int size;
	struct decknode *first;
	struct decknode *last;
};

/*-----------------------------------------------------------------
* Function: deckinit
* Purpose: Create the deck and all of the cards to be used by the
*          game during execution
* Parameters: decks - Integer for number of total decks to create
* Return: Pointer to the header of the list of cards
-----------------------------------------------------------------*/
struct deckhead *deckinit(int decks);

/*-----------------------------------------------------------------
* Function: getCard
* Purpose: Draw a random card from the deck
* Parameters: list - pointer to the header of the main deck
*             discard - pointer to the header of the discard deck
* Return: a character with the value of the card stored in it
-----------------------------------------------------------------*/
char getCard(struct deckhead *list, struct deckhead *discard);

/*-----------------------------------------------------------------
* Function: deckDestroy
* Purpose: Destroy the deck and free up all memory it contains
* Parameters: deck - pointer to the header of the deck
* Return: int value for success or failure
-----------------------------------------------------------------*/
int deckDestroy(struct deckhead *deck);

#endif /*DECKLIST_H*/
