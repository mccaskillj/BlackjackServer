#include <stdlib.h>

#include "common.h"
#include "decklist.h"
#include "playerlist.h"

struct argdata *argdataBuild(){
	struct argdata *argdata = malloc(sizeof(struct argdata));
	argdata->gameSeq = 0;
	argdata->chatSeq = 0;
	argdata->decks = 2;
	argdata->money = 100;
	argdata->timer = 15;
	argdata->mBet = 1;
	argdata->aPlayer = 0;
	argdata->port = calloc(6, sizeof(char));
	argdata->dCards = calloc(21, sizeof(char));
	argdata->discard = deckinit(0);
	argdata->players = playerinit();
	argdata->seats = malloc(sizeof(struct playernode *)*7);
	for (int i = 0; i<7; i++){
		argdata->seats[i] = NULL;
	}

	return argdata;
}

int argdataDestroy(struct argdata *argdata){
	free(argdata->port);
	free(argdata->seats);
	free(argdata->dCards);
	deckDestroy(argdata->mainDeck);
	deckDestroy(argdata->discard);
	playerListDestroy(argdata->players);
	argdata->port = NULL;
	argdata->discard = NULL;
	argdata->players = NULL;
	argdata->seats = NULL;
	argdata->dCards = NULL;
	free(argdata);
	argdata = NULL;

	return 1;
}