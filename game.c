#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "game.h"
#include "decklist.h"
#include "common.h"
#include "playerlist.h"

int openSeat(struct argdata *argdata){
	int i;
	for (i = 0; i < 7 && argdata->seats[i] != NULL; i++);

	if (i == 7){
		return -1;
	}

	return i;

}

int changePlayer(struct argdata *argdata){
	uint8_t i;
	for (i = argdata->aPlayer+1;i<7;i++){
		if (argdata->seats[i] != NULL &&
				argdata->seats[i]->player_addr != NULL &&
				argdata->seats[i]->active == 1){
			break;
		}
	}

	if (i == 7){
		argdata->aPlayer = 0;
		return 0;
	} else {
		argdata->aPlayer = 1;
		return i;
	}

	return -1;
}

int checkName(char* name){
	for (int i = 0; i<12; i++){
		if(!isalpha(name[i]) && !isdigit(name[i]) && name[i] != '\0'){
			return -4;
		}
	}

	return 1;
}

int makeBet(struct argdata *argdata, char* response){


	return 0;
}

int pConnect(struct argdata *argdata, struct sockaddr_storage **client_addr, socklen_t **server_addr_len, char* response){
	int index = openSeat(argdata);
	char name[13];
	struct playernode *player;
	
	if (index == -1)
		return -2;
	
	if(checkName(&response[1]) <0)
		return -4;
	
	strncpy(name,&response[1],12);
	player = findPlayer(argdata, name);
	if (player->player_addr != NULL)
		return -3;
	player->player_addr = *client_addr;
	player->server_addr_len = *server_addr_len;
	argdata->seats[index] = player;

	return 1;
}

int getCardVal(char card){
	int val = ((card - 1) % 13) + 1;
	switch(val){
	case 1:
		return 11;
	case 11:
	case 12:
	case 13:
		return 10;
	default:
		return val;
	}

	return 0;
}

int deckTotal(char *deck){
	int total = 0;
	int aces = 0;
	int val;
	for (int i = 0; i<21; i++){
		if (deck[i] == '\0')
			break;
		val = getCardVal(deck[i]);
		if (val == 11)
			aces++;
		total = total + val;
	}

	while(total > 21 && aces != 0){
		total = total - 10;
		aces--;
	}

	return total;
}

int addToDeck(char* deck, char card){
	int i;
	for(i = 0; i<21; i++){
		if (deck[i] == '\0'){
			strncpy(&deck[i], &card, 1);
			break;
		}
	}

	return 0;
}

int dealerPlays(struct argdata *argdata){
	while (deckTotal(argdata->dCards) <= 16){
		addToDeck(argdata->dCards, getCard(argdata->mainDeck,argdata->discard));
	}

	return deckTotal(argdata->dCards);
}

int seatsEmpty(struct argdata *argdata){
	for (int i = 0; i <7; i++){
		if (argdata->seats[i] == NULL){
			return 0;
		}
	}

	return 1;
}