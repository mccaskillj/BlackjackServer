/*-----------------------------------------------------------------------------
* game.c
* Jordan McCaskill
* CMPT 361
* Assignment 2
*
* All functions dealing with the logic and manipulation of the game rules
-----------------------------------------------------------------------------*/
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "game.h"
#include "decklist.h"
#include "common.h"
#include "playerlist.h"
#include "state.h"
#include "msg.h"

/*-----------------------------------------------------------------
* Function: openSeat
* Purpose: Get the index of the next open seat
* Parameters: argdata - the main game state struct
* Return: int for success or failure
-----------------------------------------------------------------*/
static int openSeat(struct argdata *argdata)
{
	int i;

	/*loop until first empty seat or end of list*/
	for (i = 0; i < MAX_PLAYERS && argdata->seats[i] != NULL; i++);

	/*check if end of list or empty seat*/
	if (i == MAX_PLAYERS)
		return -1;

	return i;

}

/*change the active player*/
int changePlayer(struct argdata *argdata)
{
	uint8_t i;

	/*loop through seats starting from currently active player*/
	for (i = argdata->aPlayer; i < MAX_PLAYERS; i++) {
		/*if player is active and has connection*/
		if (argdata->seats[i] != NULL &&
			argdata->seats[i]->player_addr != NULL &&
			argdata->seats[i]->active == ACTIVE)
			break;
	}

	/*check if dealer or player*/
	if (i == MAX_PLAYERS) {
		argdata->aPlayer = 0;
		return 0;
	}
	argdata->aPlayer = i + 1;
	return i + 1;

}

/*-----------------------------------------------------------------
* Function: checkName
* Purpose: Check if the name is only numbers and letters
* Parameters: name - name of the player
* Return: int for success or failure
-----------------------------------------------------------------*/
static int checkName(char *name)
{
	/*loop through all the letters*/
	for (int i = 0; i < MAXNAMESIZE; i++) {
		/*check if it is a number or a letter or null terminator*/
		if (!isalpha(name[i]) && !isdigit(name[i]) && name[i] != '\0')
			return -4;

	}

	return 1;
}

/*connect a player*/
int pConnect(struct argdata *argdata,
	struct sockaddr_storage **client_addr,
	socklen_t **server_addr_len, char *response)
{
	/*get an open seat*/
	int index = openSeat(argdata);

	char name[MAXNAMESIZE];
	struct playernode *player;

	/*check if index is error for no empty seats*/
	if (index == -1)
		return -2;

	/*check the name*/
	if (checkName(&response[CONNAMEOFF]) < 0)
		return -4;


	strncpy(name, &response[CONNAMEOFF], MAXNAMESIZE);
	player = findPlayer(argdata, name);

	/*check if player is active in a game or doesnt have enough money*/
	if (player->money < argdata->mBet)
		return -1;
	if (player->player_addr != NULL ||
		player->active != WAIT ||
		player->bet != 0)
		return -3;

	/*set the connections*/
	player->player_addr = *client_addr;
	player->server_addr_len = *server_addr_len;
	argdata->seats[index] = player;

	/*if the betting phase is active set to active and reset
	*  the timer*/
	if (argdata->gamestate == BETTING) {
		player->active = ACTIVE;
		argdata->startTime = 0;
	}

	return 1;
}

/*-----------------------------------------------------------------
* Function: getCardVal
* Purpose: get the game value of the card
* Parameters: card - the card
* Return: int for value of card or failure
-----------------------------------------------------------------*/
static int getCardVal(char card)
{
	/*get the value of the card*/
	int val = ((card - 1) % 13) + 1;

	/*determine its value in a hand of blackjack*/
	switch (val) {
	case ACE:
		return 11;
	case JACK:
	case QUEEN:
	case KING:
		return 10;
	default:
		return val;
	}
}

/*get the total of the deck*/
int deckTotal(char *deck)
{

	int total = 0;
	int aces = 0;

	int val;

	/*move through all the cards in the deck*/
	for (int i = 0; i < P_DECK_SIZE; i++) {
		/*if it is a null terminator then break*/
		if (deck[i] == '\0')
			break;

		/*get value of the card*/
		val = getCardVal(deck[i]);

		/*increment an aces counter*/
		if (val == ACE_VALUE)
			aces++;

		total = total + val;
	}

	/*if the value is over 21 and there are still aces then
	*  make the aces worth 10 less until not over*/
	while (total > BLACKJACK && aces != 0) {
		total = total - 10;
		aces--;
	}

	return total;
}

/*add a card to the deck*/
int addToDeck(char *deck, char card)
{
	int i;

	/*find the first open space and add it*/
	for (i = 0; i < P_DECK_SIZE; i++) {
		if (deck[i] == '\0') {
			strncpy(&deck[i], &card, 1);
			break;
		}
	}

	return 0;
}

/*-----------------------------------------------------------------
* Function: dealerPlays
* Purpose: play out the dealers hand
* Parameters: argdata - the main game state struct
* Return: int for success or failure
-----------------------------------------------------------------*/
static int dealerPlays(struct argdata *argdata)
{
	/*hit while the dealer can*/
	while (deckTotal(argdata->dCards) <= DEALERHIT)
		addToDeck(argdata->dCards, getCard(argdata->mainDeck,
			argdata->discard));

	return deckTotal(argdata->dCards);
}

/*broadcast betting state to players*/
int broadcastBetting(int *fd, struct argdata *argdata, char *out)
{
	char player;

	generateState(argdata, 0, out);

	/*loop through the players*/
	for (int i = 0; i < MAX_PLAYERS; i++) {
		/*if they are active and have not bet then send a state
		*  that has them as active*/
		if (argdata->seats[i] != NULL &&
			argdata->seats[i]->active == ACTIVE &&
			argdata->seats[i]->bet == 0) {

			player = i + 1;
			strncpy(&out[APLAY_OFF], &player, 1);
			sendToPlayer(fd, out, argdata->seats[i]->player_addr,
				argdata->seats[i]->server_addr_len, MAXPSIZE);
		}
		/*send everyone else connected the normal state*/
		else if (argdata->seats[i] != NULL &&
			argdata->seats[i]->player_addr != NULL) {
			player = 0;
			strncpy(&out[APLAY_OFF], &player, 1);
			sendToPlayer(fd, out, argdata->seats[i]->player_addr,
				argdata->seats[i]->server_addr_len, MAXPSIZE);
		}
	}

	argdata->rebroadcast = 0;

	return 1;
}

/*-----------------------------------------------------------------
* Function: seatsEmpty
* Purpose: check if the seats are empty
* Parameters: argdata - the main game state struct
* Return: int for success or failure
-----------------------------------------------------------------*/
static int seatsEmpty(struct argdata *argdata)
{
	/*loop through the seats until an active or observing player
	*  is found*/
	for (int i = 0; i < MAX_PLAYERS; i++) {
		if (argdata->seats[i] != NULL &&
			argdata->seats[i]->active != KICK)
			return 0;
	}

	argdata->aPlayer = 0;
	return 1;
}

/*-----------------------------------------------------------------
* Function: hasActivePlayers
* Purpose: check if there are active players
* Parameters: argdata - the main game state struct
* Return: int for success or failure
-----------------------------------------------------------------*/
static int hasActivePlayers(struct argdata *argdata)
{
	/*loop through looking for active players*/
	for (int i = 0; i < MAX_PLAYERS; i++) {
		if (argdata->seats[i] != NULL && argdata->seats[i]->active == ACTIVE)
			return 1;
	}

	return 0;
}

/*-----------------------------------------------------------------
* Function: setToActive
* Purpose: set all players to active or kick them if needed
* Parameters: argdata - the main game state struct
* Return: int for success or failure
-----------------------------------------------------------------*/
static int setToActive(struct argdata *argdata)
{
	/*loop through players*/
	for (int i = 0; i < MAX_PLAYERS; i++) {
		/*kick them if they are flagged to be kicked*/
		if (argdata->seats[i] != NULL &&
			argdata->seats[i]->active == KICK) {

			if (argdata->seats[i]->player_addr != NULL) {
				free(argdata->seats[i]->player_addr);
				free(argdata->seats[i]->server_addr_len);
				argdata->seats[i]->player_addr = NULL;
				argdata->seats[i]->server_addr_len = NULL;
			}
			argdata->seats[i] = NULL;

			msgRemovePlayer(argdata->msglist, i);

		/*otherwise set them to active if they are seated*/
		} else if (argdata->seats[i] != NULL) {
			argdata->seats[i]->active = ACTIVE;
		}
	}

	return 1;
}

/*-----------------------------------------------------------------
* Function: dealCards
* Purpose: deal out cards to begin game
* Parameters: argdata - the main game state struct
* Return: int for success or failure
-----------------------------------------------------------------*/
static int dealCards(struct argdata *argdata)
{
	char card;

	/*loop through players*/
	for (int i = 0; i < MAX_PLAYERS; i++) {
		/*if they aren't spectating then deal them cards*/
		if (argdata->seats[i] != NULL &&
			argdata->seats[i]->bet != 0) {

			card = getCard(argdata->mainDeck, argdata->discard);
			addToDeck(argdata->seats[i]->cards, card);
			card = getCard(argdata->mainDeck, argdata->discard);
			addToDeck(argdata->seats[i]->cards, card);
		}
	}

	/*deal the dealer*/
	card = getCard(argdata->mainDeck, argdata->discard);
	addToDeck(argdata->dCards, card);

	return 1;
}

/*-----------------------------------------------------------------
* Function: payout
* Purpose: payout the winners and or collect from the losers
* Parameters: fd - file descriptor for the socket
*             argdata - the main game state struct
*             out - the output character array
* Return: int for success or failure
-----------------------------------------------------------------*/
static int payout(int *fd, struct argdata *argdata, char *out)
{
	int dealer = deckTotal(argdata->dCards);

	int player;

	/*loop through the players*/
	for (int i = 0; i < MAX_PLAYERS; i++) {
		if (argdata->seats[i] != NULL && argdata->seats[i]->cards[0] != '\0') {
			player = deckTotal(argdata->seats[i]->cards);

			/*player busted*/
			if (player > BLACKJACK) {
				argdata->seats[i]->money -= argdata->seats[i]->bet;
				argdata->seats[i]->bet = 0;

			/*player has blackjack*/
			} else if (player == BLACKJACK &&
				argdata->seats[i]->cards[2] == '\0' ) {
				/*dealer has blackjack*/
				if (dealer == BLACKJACK &&
					argdata->dCards[2] == '\0') {
					argdata->seats[i]->bet = 0;
				/*dealer does not have blackjack*/
				} else {
					/*check if money goes out of range*/
					if ((uint64_t)argdata->seats[i]->money + (argdata->seats[i]->bet * 1.5) > MAXMONEY)
						argdata->seats[i]->money = MAXMONEY;
					else
						argdata->seats[i]->money += (int)(argdata->seats[i]->bet * 1.5);
					argdata->seats[i]->bet = 0;
				}
			/*player has 21 but not blackjack*/
			} else if (player == BLACKJACK) {
				/*dealer has blackjack*/
				if (dealer == BLACKJACK && argdata->dCards[2] == '\0') {
					argdata->seats[i]->money -= argdata->seats[i]->bet;
					argdata->seats[i]->bet = 0;
				/*dealer has 21 but not blackjack*/
				} else if (dealer == BLACKJACK) {
					argdata->seats[i]->bet = 0;
				/*player has more than dealer*/
				} else {
					/*check if money goes out of range*/
					if ((uint64_t)argdata->seats[i]->money + argdata->seats[i]->bet > MAXMONEY)
						argdata->seats[i]->money = MAXMONEY;
					else
						argdata->seats[i]->money += argdata->seats[i]->bet;
					argdata->seats[i]->bet = 0;
				}
			/*player is under 21*/
			} else {
				/*dealer has less than player or busted*/
				if (dealer > BLACKJACK ||
					dealer < player) {
					/*check if money goes out of range*/
					if ((uint64_t)argdata->seats[i]->money + argdata->seats[i]->bet > MAXMONEY)
						argdata->seats[i]->money = MAXMONEY;
					else
						argdata->seats[i]->money += argdata->seats[i]->bet;
					argdata->seats[i]->bet = 0;
				/*dealer tied player*/
				} else if (dealer == player) {
					argdata->seats[i]->bet = 0;
				/*dealer beat player*/
				} else {
					argdata->seats[i]->money -= argdata->seats[i]->bet;
					argdata->seats[i]->bet = 0;
				}
			}
		}

		/*if they are inactive kick them*/
		if (argdata->seats[i] != NULL &&
			(argdata->seats[i]->active == KICK ||
			argdata->seats[i]->money < argdata->mBet)) {

			/*if they are still connected send the state*/
			if (argdata->seats[i]->player_addr != NULL) {
				generateState(argdata, 0, out);
				sendToPlayer(fd, out, argdata->seats[i]->player_addr,
					argdata->seats[i]->server_addr_len, MAXPSIZE);
				if (argdata->seats[i]->money < argdata->mBet) {
					generateError(1, out, NULL);
					sendToPlayer(fd, out, argdata->seats[i]->player_addr,
						argdata->seats[i]->server_addr_len, ERRPSIZE);
				}
				free(argdata->seats[i]->player_addr);
				free(argdata->seats[i]->server_addr_len);
				argdata->seats[i]->player_addr = NULL;
				argdata->seats[i]->server_addr_len = NULL;
			}
			argdata->seats[i]->active = WAIT;
			memset(argdata->seats[i]->cards, '\0', P_DECK_SIZE);
			argdata->seats[i] = NULL;

			msgRemovePlayer(argdata->msglist, i);
		}
	}
	return 1;
}

/*-----------------------------------------------------------------
* Function: resetGame
* Purpose: reset all the round data
* Parameters: argdata - the main game state struct
* Return: int for success or failure
-----------------------------------------------------------------*/
static int resetGame(struct argdata *argdata)
{
	/*loop through and reset round data*/
	for (int i = 0; i < MAX_PLAYERS; i++) {
		if (argdata->seats[i] != NULL) {
			argdata->seats[i]->bet = 0;
			memset(argdata->seats[i]->cards, '\0', P_DECK_SIZE);
		}
	}

	memset(argdata->dCards, '\0', P_DECK_SIZE);

	return 1;
}

/*-----------------------------------------------------------------
* Function: needToBet
* Purpose: check if anyone still needs to bet
* Parameters: argdata - the main game state struct
* Return: int for success or failure
-----------------------------------------------------------------*/
static int needToBet(struct argdata *argdata)
{
	/*check if anyone active still has no bet*/
	for (int i = 0; i < MAX_PLAYERS; i++) {
		if (argdata->seats[i] != NULL &&
			argdata->seats[i]->active == ACTIVE &&
			argdata->seats[i]->bet == 0)
			return 1;
	}
	return 0;
}

/*-----------------------------------------------------------------
* Function: sendBetTimeout
* Purpose: send message if the player timed out during betting
* Parameters: fd - file descriptor for the socket
*             argdata - the main game state struct
*             out - the output character array
* Return: int for success or failure
-----------------------------------------------------------------*/
static int sendBetTimeout(int *fd, struct argdata *argdata, char *out)
{
	/*loop through and look for active players with no bet*/
	for (int i = 0; i < MAX_PLAYERS; i++) {
		if (argdata->seats[i] != NULL &&
			argdata->seats[i]->active == ACTIVE &&
			argdata->seats[i]->bet == 0) {

			/*send the timeout message and kick them*/
			sendToPlayer(fd, out, argdata->seats[i]->player_addr,
				argdata->seats[i]->server_addr_len, ERRPSIZE);
			if (argdata->seats[i]->player_addr != NULL) {
				free(argdata->seats[i]->player_addr);
				free(argdata->seats[i]->server_addr_len);
			}

			msgRemovePlayer(argdata->msglist, i);

			argdata->seats[i]->player_addr = NULL;
			argdata->seats[i]->server_addr_len = NULL;
			argdata->seats[i]->active = WAIT;
			argdata->seats[i]->bet = 0;
			argdata->seats[i] = NULL;
		}
	}
	return 1;
}

/*move the game state along*/
int proceedGame(int *fd, struct argdata *argdata, char *out)
{
	switch (argdata->gamestate) {

	case BETTING: /*betting phase*/
		/*check if timeout has occurred*/
		if (argdata->startTime / USEC_SEC >= argdata->timer) {
			generateError(5, out, NULL);
			sendBetTimeout(fd, argdata, out);
			argdata->startTime = 0;
		}
		/*check if anyone needs to bet and move to play if not*/
		if (needToBet(argdata) == 0) {
			changePlayer(argdata);
			dealCards(argdata);
			argdata->gamestate = PLAY;
			broadcast(fd, argdata, out);
			argdata->startTime = 0;
		}
		break;

	case PLAY: /*play phase*/
		/*check if timeout has occurred*/
		if (argdata->startTime / USEC_SEC >= argdata->timer) {
			generateError(5, out, NULL);
			sendToPlayer(fd, out,
				argdata->seats[argdata->aPlayer-1]->player_addr,
				argdata->seats[argdata->aPlayer-1]->server_addr_len,
				ERRPSIZE);
			argdata->seats[argdata->aPlayer-1]->active = KICK;
			argdata->startTime = 0;
			changePlayer(argdata);
			broadcast(fd, argdata, out);
		}
		/*check that the table needs to continue playing*/
		if (argdata->aPlayer != 0 && hasActivePlayers(argdata) == 1)
			break;

	case WRAPUP: /*wrap-up phase*/
		dealerPlays(argdata);
		payout(fd, argdata, out);
		argdata->dealerSends = DEALER_SENDS;
		argdata->gameSeq += 1;
		broadcast(fd, argdata, out);
		argdata->gamestate = BROADCAST;
		printState(argdata);

	case BROADCAST:
		/*broadcast again if the dealer broadcasts are left and
		*  there are people to receive them*/
		if (argdata->dealerSends > 0 && seatsEmpty(argdata) == 0) {
			if (argdata->rebroadcast / USEC_SEC >= DEALER_CAST) {
				broadcast(fd, argdata, out);
				argdata->dealerSends--;
				return 1;
			}
			return 1;
		/*if not then just reset*/
		} else {
			resetGame(argdata);
			argdata->gameSeq += 1;
			argdata->gamestate = SETUP;
			printState(argdata);
		}

	case SETUP: /*setup phase*/
		/*don't set up if there is no one playing*/
		if (seatsEmpty(argdata) == 1)
			return 0;

		setToActive(argdata);
		argdata->gamestate = BETTING;
		broadcastBetting(fd, argdata, out);
		argdata->startTime = 0;
		return 1;
	}

	/*check if rebroadcast is needed*/
	if (argdata->rebroadcast / USEC_SEC >= REBROADCAST) {
		if (argdata->gamestate == BETTING)
			broadcastBetting(fd, argdata, out);
		else
			broadcast(fd, argdata, out);
		msgBroadcast(fd, argdata->msglist);
	}

	return 1;
}
