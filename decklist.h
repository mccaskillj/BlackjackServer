#ifndef DECKLIST_H
#define DECKLIST_H

struct decknode
{
	struct decknode *next;
	char val;
};

struct deckhead
{
	int size;
	struct decknode *first;
	struct decknode *last;
};

struct deckhead *deckinit(int decks);
struct decknode *deckNodeCreate(char val);
void deckAppend(struct deckhead *list, char val);
char getCard(struct deckhead *list, struct deckhead *discard);
int deckDestroy(struct deckhead *deck);
void reshuffle(struct deckhead *list, struct deckhead *discard);
int getSize(struct deckhead *deck);

/*DECKLIST_H*/
#endif