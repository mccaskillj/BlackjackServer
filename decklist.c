/*-----------------------------------------------------------------------------
* decklist.c
* Jordan McCaskill
* CMPT 361
* Assignment 2
*
* All functions dealing with creation and manipulation of the deck of cards
-----------------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "decklist.h"

/*-----------------------------------------------------------------
* Function: deckNodeCreate
* Purpose: create a node for the deck
* Parameters: val - the value of the card in the node
* Return: pointer to the new node
-----------------------------------------------------------------*/
static struct decknode *deckNodeCreate(char val)
{
	/*set space for new node*/
	struct decknode *node = malloc(sizeof(struct decknode));

	if (node == NULL)
		exit(EXIT_FAILURE);

	node->next = NULL;
	node->val = val;

	return node;
}

/*-----------------------------------------------------------------
* Function: deckAppend
* Purpose: append a node to the list
* Parameters: list - the deck list
*             val - the value of the card
* Return: None
-----------------------------------------------------------------*/
static void deckAppend(struct deckhead *list, char val)
{
	/*create the node*/
	struct decknode *newNode = deckNodeCreate(val);
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
}

/*initialize the list*/
struct deckhead *deckinit(int decks)
{
	int i;
	/*set space for the list header*/
	struct deckhead *list = malloc(sizeof(struct deckhead));

	if (list == NULL)
		exit(EXIT_FAILURE);

	list->first = NULL;
	list->last = NULL;
	list->size = 0;

	/*seed random for the program*/
	srand(time(NULL));

	/*ad 52 cards to the list for each deck*/
	for (i = 0; i < decks * DECKSIZE; i++)
		deckAppend(list, (char)(i % DECKSIZE + 1));

	return list;
}

/*-----------------------------------------------------------------
* Function: reshuffle
* Purpose: Add the discard list back on to the main list
* Parameters: list - the list of cards
*             discard - the discard list of cards
* Return: None
-----------------------------------------------------------------*/
static void reshuffle(struct deckhead *list, struct deckhead *discard)
{
	/*point the end to the front*/
	list->last->next = discard->first;
	list->last = discard->last;
	/*reset discard*/
	discard->first = NULL;
	discard->last = NULL;
	/*change the size of list*/
	list->size = list->size + discard->size;
	/*set size of list to 0*/
	discard->size = 0;
}

/*get a card from the deck*/
char getCard(struct deckhead *list, struct deckhead *discard)
{
	/*if the value falls below the threshold then reshuffle*/
	if (list->size <= 15)
		reshuffle(list, discard);

	struct decknode *next;
	struct decknode *ret;
	int num, i;

	/*get a random pos in the list*/
	num = rand() % list->size;

	/*check if number is first card*/
	if (num == 0) {
		ret = list->first;
		list->first = list->first->next;
		ret->next = NULL;
		list->size--;
	/*look for index*/
	} else {
		next = list->first;
		for (i = 1; i < num - 1; i++)
			next = next->next;

		if (num == list->size)
			list->last = next;

		ret = next->next;
		next->next = ret->next;
		ret->next = NULL;
		list->size--;
	}

	/*add to discard list*/
	if (discard->first == NULL) {
		discard->first = ret;
		discard->last = ret;
		discard->size++;
	} else {
		discard->last->next = ret;
		discard->last = ret;
		discard->size++;
	}

	return ret->val;
}

/*destroy the deck*/
int deckDestroy(struct deckhead *deck)
{
	struct decknode *pos = deck->first;
	/*loop until no more cards exist*/
	for (; deck->first != NULL;) {
		deck->first = pos->next;
		pos->next = NULL;
		free(pos);
		pos = deck->first;
	}

	/*free the deck*/
	free(deck);
	deck = NULL;

	return 1;
}
