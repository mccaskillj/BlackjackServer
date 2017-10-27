#include <stdlib.h>
#include <stdio.h>

#include "decklist.h"

struct deckhead *deckinit(int decks){
	int i;
	struct deckhead *list = malloc(sizeof(struct deckhead));
	list->first = NULL;
	list->last = NULL;
	list->size = 0;

	for (i=0; i< decks * 52; i++){
		deckAppend(list,(char)(i%52+1));
	}
	
	return list;
}

struct decknode *deckNodeCreate(char val){
	struct decknode *node = malloc(sizeof(struct decknode));
	node->next = NULL;
	node->val =val;

	return node;
}

void deckAppend(struct deckhead *list, char val){
	struct decknode *newNode = deckNodeCreate(val);
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
}

int getSize(struct deckhead *deck){
	int cnt = 0;
	struct decknode * p;
	if (deck->first == NULL)
		return 0;
	for (p = deck->first; p != deck->last; cnt++, p = p->next);

	return cnt;
}

void reshuffle(struct deckhead *list, struct deckhead *discard){
	list->last->next = discard->first;
	list->last = discard->last;
	discard->first = NULL;
	discard->last = NULL;
	list->size = list->size + discard->size;
	discard->size = 0;
}

char getCard(struct deckhead *list, struct deckhead *discard){
	if (list->size <= 15){
		reshuffle(list,discard);
	}
	struct decknode *next; 
	struct decknode *ret;
	int num, i;

	num = rand() % list->size;
	if (num == 0){
		ret = list->first;
		list->first=list->first->next;
		ret->next=NULL;
		list->size--;
	}
	else{
		next = list->first;
		for (i=1; i<num-1; i++)
			next=next->next;

		if (num == list->size){
			list->last = next;
		}

		ret = next->next;
		next->next=ret->next;
		ret->next=NULL;
		list->size--;
	}

	if (discard->first == NULL){
		discard->first = ret;
		discard->last = ret;
		discard->size++;
	}
	else
	{
		discard->last->next = ret;
		discard->last = ret;
		discard->size++;
	}

	return ret->val;
}

int deckDestroy(struct deckhead *deck){
	struct decknode *pos = deck->first;
	for(;deck->first != NULL;){
		deck->first = pos->next;
		pos->next = NULL;
		free(pos);
		pos=deck->first;
	}

	free(deck);
	deck = NULL;

	return 1;
}