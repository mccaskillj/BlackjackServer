/*-----------------------------------------------------------------------------
* state.c
* Jordan McCaskill
* CMPT 361
* Assignment 2
*
* All functions associated with maintiaining and changing the state of the
* game
-----------------------------------------------------------------------------*/

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <errno.h>

#include "common.h"
#include "playerlist.h"
#include "decklist.h"
#include "state.h"
#include "game.h"
#include "msg.h"

/*generate the state to send to the player*/
int generateState(struct argdata *argdata, char opcode, char *out)
{
	/*reset the state*/
	memset(out, 0, MAXPSIZE);

	char aPlayer = argdata->aPlayer;

	/*copy values from argdata into clean state*/
	strncpy(out, &opcode, 1);
	*((uint16_t *)(&out[GSEQU_OFF])) = htons(argdata->gameSeq); /*set seq number*/
	*((uint32_t *)(&out[MMBET_OFF])) = htonl(argdata->mBet); /*set min bet*/
	strncpy(&out[APLAY_OFF], &aPlayer, 1); /*set active player*/
	strncpy(&out[DCARD_OFF], argdata->dCards, 21); /*set dealer cards*/

	/*copy player values*/
	for (int i = 0; i < MAX_PLAYERS; i++) {
		if (argdata->seats[i] != NULL) {
			strncpy(&out[UNAME_OFF + (i * PSTEP_OFF)], argdata->seats[i]->name, MAXNAMESIZE);
			*((uint32_t *)(&out[PBANK_OFF + (i * PSTEP_OFF)])) = htonl(argdata->seats[i]->money);
			*((uint32_t *)(&out[PLBET_OFF + (i * PSTEP_OFF)])) = htonl(argdata->seats[i]->bet);
			strncpy(&out[PCARD_OFF + (i * PSTEP_OFF)], argdata->seats[i]->cards, P_DECK_SIZE);
		}
	}

	return 1;
}

/*-----------------------------------------------------------------
* Function: checkState
* Purpose: Check if the client state matches the expected
* Parameters: argdata - the main game state struct
*             input - the state to check against
* Return: int for success or failure
-----------------------------------------------------------------*/
static int checkState(struct argdata *argdata, char *input)
{
	char state[MAXPSIZE];
	uint16_t seqNumber;

	generateState(argdata, 0, state);

	/*get the client sequence number*/
	seqNumber = ntohs(*((uint16_t *)(&input[GSEQU_OFF])));

	/*compare the sequence number*/
	if (seqNumber != argdata->gameSeq + 1)
		return 0;

	/*compare the entire state*/
	if (memcmp(&input[STATECMP], &state[STATECMP], MAXPSIZE-STATECMP) != 0)
		return 0;

	return 1;
}

/*handle sending data to the clients*/
int sendToPlayer(int *fd, char *out,
	struct sockaddr_storage *client_addr,
	socklen_t *server_addr_len, int pSize)
{
	int retVal = sendto(*fd, out, pSize, 0,
		(struct sockaddr *)client_addr, *server_addr_len);
	while (retVal == -1 && errno == EINTR) {
		retVal = sendto(*fd, out, pSize, 0,
			(struct sockaddr *)client_addr, *server_addr_len);
	}

	if (retVal == -1) {
		perror("sendto:");
	}

	return 1;
}

/*broadcast the state to all players*/
int broadcast(int *fd, struct argdata *argdata, char *out)
{
	struct playernode *player;

	generateState(argdata, 0, out);

	/*send state to all players with connections*/
	for (int i = 0; i < MAX_PLAYERS; i++) {
		if (argdata->seats[i] != NULL &&
			argdata->seats[i]->player_addr != NULL) {

			player = argdata->seats[i];
			sendToPlayer(fd, out, player->player_addr,
				player->server_addr_len, MAXPSIZE);
		}
	}

	argdata->rebroadcast = 0;

	return 0;
}

/*generate the error message*/
int generateError(char error, char *out, char *gMsg)
{
	memset(out, 0, MAXPSIZE);
	char value = 6;
	char *msg;

	strncpy(out, &value, 1);
	strncpy(&out[1], &error, 1);

	/*determine the error and attach the message*/
	switch (error) {
	case GENERALERR:
		if (gMsg == NULL)
			msg = "General: error is unspecified";
		else
			msg = gMsg;
		break;
	case MONEYERR:
		msg = "No Money: insufficient money to make bet or play";
		break;
	case TABLEERR:
		msg = "Table Full: no seats remain at table";
		break;
	case SEATERR:
		msg = "Name: name is already in use/unusable";
		break;
	case NAMEERR:
		msg = "Invalid Name: name entered is not valid";
		break;
	case TIMEERR:
		msg = "Timeout: failed to make move in required time";
		break;
	default:
		msg = "";
		break;
	}

	strncpy(&out[2], msg, ERRORMSG - 1);

	return 1;
}

/*-----------------------------------------------------------------
* Function: generateMessage
* Purpose: Generate the message to broadcast back to clients
* Parameters: argdata - the main game state struct
*             msg - the incoming message
* Return: int for success or failure
-----------------------------------------------------------------*/
static int generateMessage(struct argdata *argdata, char *msg)
{
	/*flip the server sequence number*/
	*((uint32_t *)(&msg[MSGOFF])) = htonl(argdata->chatSeq);

	return 1;
}

/*-----------------------------------------------------------------
* Function: matchIP
* Purpose: Match the IP and host to a player
* Parameters: argdata - the main game state struct
*             client_addr - the clients address information
*             server_addr_len - the length of client_addr
* Return: int for success or failure
-----------------------------------------------------------------*/
static int matchIP(struct argdata *argdata,
	struct sockaddr_storage *client_addr,
	socklen_t *server_addr_len)
{
	uint8_t i;
	int err;
	char argIP[MAXIPSIZE];
	char posIP[MAXIPSIZE];
	char argPort[MAXHOSTSIZE];
	char posPort[MAXHOSTSIZE];

	/*loop through players to attempt to match the IP and port*/
	for (i = 0; i < MAX_PLAYERS; i++) {
		if (argdata->seats[i] != NULL && argdata->seats[i]->player_addr != NULL) {
			/*get host and port for the player*/
			err = getnameinfo((struct sockaddr *)argdata->seats[i]->player_addr,
					*argdata->seats[i]->server_addr_len, argIP, sizeof(argIP),
					argPort, sizeof(argPort), NI_NUMERICHOST | NI_NUMERICSERV);

			/*check for error getting host and port*/
			if (err != 0)
				continue;

			/*get host and port for the connecting client*/
			err = getnameinfo((struct sockaddr *)client_addr, *server_addr_len,
					posIP, sizeof(argIP), posPort,
					sizeof(argPort), NI_NUMERICHOST | NI_NUMERICSERV);

			/*check for error getting host and port*/
			if (err != 0)
				continue;

			/*check if the host and port match*/
			if (strncmp(argIP, posIP, MAXIPSIZE) == 0 &&
				strncmp(argPort, posPort, MAXHOSTSIZE) == 0)
				return i;
		}
	}

	return -1;
}

/*-----------------------------------------------------------------
* Function: connection
* Purpose: Handle a connection packet
* Parameters: fd - file descriptor for the socket
*             argdata - the main game state struct
*             response - the output character array
*             out - the output character array
*             client_addr - the clients address information
*             server_addr_len - the length of client_addr
*             received - the amount of bytes received
* Return: int for success or failure
-----------------------------------------------------------------*/
static int connection(int *fd, struct argdata *argdata, char *response, char *out,
			struct sockaddr_storage **client_addr,
			socklen_t **server_addr_len, int received)
{
	int retVal;

	/*check that the packet size is correct*/
	if (received != CONPSIZE) {
		generateError(0, out, NULL);
		sendToPlayer(fd, out, *client_addr, *server_addr_len, ERRPSIZE);
	}

	/*connect the player*/
	retVal = pConnect(argdata, client_addr, server_addr_len, response);

	/*check if connection did not go through*/
	if (retVal <= 0) {
		generateError(retVal * -1, out, NULL);
		sendToPlayer(fd, out,*client_addr, *server_addr_len, ERRPSIZE);
	} else {
		/*if betting phase then broadcast betting*/
		if (argdata->gamestate == BETTING) {
			broadcastBetting(fd, argdata, out);
		/*otherwise broadcast state*/
		} else {
			broadcast(fd, argdata, out);
		}
		*client_addr = NULL;
		*server_addr_len = NULL;
	}

	return 1;
}

/*-----------------------------------------------------------------
* Function: recBet
* Purpose: Handle a bet packet
* Parameters: fd - file descriptor for the socket
*             argdata - the main game state struct
*             response - the output character array
*             out - the output character array
*             client_addr - the clients address information
*             server_addr_len - the length of client_addr
*             received - the amount of bytes received
* Return: int for success or failure
-----------------------------------------------------------------*/
static int recBet(int *fd, struct argdata *argdata, char *response, char *out,
			struct sockaddr_storage **client_addr, socklen_t **server_addr_len, int received)
{
	int retVal;
	uint32_t bet;

	/*check if state size matches and is big enough*/
	if (received != MAXPSIZE ||
		checkState(argdata, response) == 0) {

		generateError(0, out, "General: Packet does not match expected");
		sendToPlayer(fd, out, *client_addr,
			*server_addr_len, ERRPSIZE);
		argdata->gameSeq += 2;
		return 1;
	}

	/*check if player exists*/
	retVal = matchIP(argdata, *client_addr, *server_addr_len);
	if (retVal == -1 ||
		argdata->seats[retVal]->bet != 0 ||
		argdata->seats[retVal]->active != ACTIVE) {
		generateError(0, out, NULL);
		sendToPlayer(fd, out, *client_addr, *server_addr_len, ERRPSIZE);
		argdata->gameSeq += 2;
		return 1;
	}

	/*check if game is allowing betting*/
	if (argdata->gamestate == BETTING) {
		bet = ntohl(*((uint32_t *)(&response[RESPA_OFF])));

		/*check if bet is valid*/
		if (bet > argdata->seats[retVal]->money ||
			bet < argdata->mBet) {

			generateError(0, out, NULL);
			sendToPlayer(fd, out, *client_addr, *server_addr_len, ERRPSIZE);
			argdata->gameSeq += 2;
			return 1;
		}
		argdata->seats[retVal]->bet = bet;
		broadcastBetting(fd, argdata, out);

		argdata->gameSeq += 2;

	} else {
		generateError(0, out, NULL);
		sendToPlayer(fd, out, *client_addr, *server_addr_len, ERRPSIZE);
		argdata->gameSeq += 2;
	}

	return 1;
}

/*-----------------------------------------------------------------
* Function: recStand
* Purpose: Handle a stand packet
* Parameters: fd - file descriptor for the socket
*             argdata - the main game state struct
*             response - the output character array
*             out - the output character array
*             client_addr - the clients address information
*             server_addr_len - the length of client_addr
*             received - the amount of bytes received
* Return: int for success or failure
-----------------------------------------------------------------*/
static int recStand(int *fd, struct argdata *argdata, char *response, char *out,
			struct sockaddr_storage **client_addr,
			socklen_t **server_addr_len, int received)
{

	int retVal;

	/*check if state matches*/
	if (received != MAXPSIZE || checkState(argdata, response) == 0) {
		generateError(0, out, "General: Packet does not match expected");
		sendToPlayer(fd, out, *client_addr, *server_addr_len, ERRPSIZE);
		return 1;
	}

	/*check if player exists*/
	retVal = matchIP(argdata, *client_addr, *server_addr_len);
	if (retVal == -1 ||
		argdata->seats[retVal]->bet == 0 ||
		argdata->seats[retVal]->active != ACTIVE) {
		generateError(0, out, NULL);
		sendToPlayer(fd, out, *client_addr, *server_addr_len, ERRPSIZE);
		return 1;
	}

	/*check if player is active*/
	if (retVal == argdata->aPlayer - 1) {
		argdata->gameSeq += 2;
		changePlayer(argdata);
		generateState(argdata, 0, out);
		broadcast(fd, argdata, out);
		argdata->startTime = 0;
	}

	return 1;
}

/*-----------------------------------------------------------------
* Function: recHit
* Purpose: Handle a hit packet
* Parameters: fd - file descriptor for the socket
*             argdata - the main game state struct
*             response - the output character array
*             out - the output character array
*             client_addr - the clients address information
*             server_addr_len - the length of client_addr
*             received - the amount of bytes received
* Return: int for success or failure
-----------------------------------------------------------------*/
static int recHit(int *fd, struct argdata *argdata, char *response, char *out,
		struct sockaddr_storage **client_addr,
		socklen_t **server_addr_len, int received)
{

	int retVal;
	char card;

	/*check if state matches*/
	if (received != MAXPSIZE ||
		checkState(argdata, response) == 0) {

		generateError(0, out, "General: Packet does not match expected");
		sendToPlayer(fd, out, *client_addr, *server_addr_len, ERRPSIZE);
		return 1;
	}

	/*check if player exists*/
	retVal = matchIP(argdata, *client_addr, *server_addr_len);
	if (retVal == -1 ||
		argdata->seats[retVal]->bet == 0 ||
		argdata->seats[retVal]->active != ACTIVE) {

		generateError(0, out, NULL);
		sendToPlayer(fd, out, *client_addr, *server_addr_len, MAXPSIZE);
		return 1;
	}

	/*check if player is active*/
	if (retVal + 1 == argdata->aPlayer) {
		argdata->gameSeq += 2;
		card = getCard(argdata->mainDeck, argdata->discard);
		addToDeck(argdata->seats[retVal]->cards, card);
		if (deckTotal(argdata->seats[retVal]->cards) >= BLACKJACK)
			changePlayer(argdata);

		generateState(argdata, 0, out);
		broadcast(fd, argdata, out);
		argdata->startTime = 0;

	} else {
		generateError(0, out, NULL);
		sendToPlayer(fd, out, *client_addr, *server_addr_len, ERRPSIZE);
	}

	return 1;
}

/*-----------------------------------------------------------------
* Function: recQuit
* Purpose: Handle a quit packet
* Parameters: fd - file descriptor for the socket
*             argdata - the main game state struct
*             response - the output character array
*             out - the output character array
*             client_addr - the clients address information
*             server_addr_len - the length of client_addr
*             received - the amount of bytes received
* Return: int for success or failure
-----------------------------------------------------------------*/
static int recQuit(int *fd, struct argdata *argdata, char *response, char *out,
		struct sockaddr_storage **client_addr,
		socklen_t **server_addr_len, int received)
{

	int retVal;

	/*check if the state matches*/
	if (received != MAXPSIZE ||
		checkState(argdata, response) == 0) {
		generateError(0, out, "General: Packet does not match expected");
		sendToPlayer(fd, out, *client_addr, *server_addr_len, ERRPSIZE);
		return 1;
	}

	/*check if the player exists*/
	retVal = matchIP(argdata, *client_addr, *server_addr_len);
	if (retVal == -1) {
		generateError(0, out, NULL);
		sendToPlayer(fd, out, *client_addr, *server_addr_len, ERRPSIZE);
		return 1;
	}

	free(argdata->seats[retVal]->player_addr);
	free(argdata->seats[retVal]->server_addr_len);
	argdata->seats[retVal]->player_addr = NULL;
	argdata->seats[retVal]->server_addr_len = NULL;

	/*check if they have been dealt cards or bet already*/
	if (argdata->seats[retVal]->bet == 0) {
		argdata->seats[retVal]->active = WAIT;
		memset(argdata->seats[retVal]->cards, '\0', P_DECK_SIZE);
		argdata->seats[retVal] = NULL;
	} else {
		argdata->seats[retVal]->active = KICK;
	}

	if (argdata->aPlayer == retVal + 1)
		changePlayer(argdata);

	msgRemovePlayer(argdata->msglist, retVal);

	return 1;
}

static int broadcastMessage(int *fd, struct argdata *argdata, char *out)
{
	for (int i = 0; i < MAX_PLAYERS; i++) {
		if (argdata->seats[i] != NULL &&
			argdata->seats[i]->player_addr != NULL) {
			sendToPlayer(fd, out, argdata->seats[i]->player_addr,
				argdata->seats[i]->server_addr_len, MESPSIZE);
			msgAppend(argdata->msglist, i, argdata->chatSeq,
				&argdata->seats[i]->player_addr,
				&argdata->seats[i]->server_addr_len, out);
		}
	}

	return 1;
}

/*-----------------------------------------------------------------
* Function: recMessage
* Purpose: Handle a message packet
* Parameters: fd - file descriptor for the socket
*             argdata - the main game state struct
*             response - the output character array
*             out - the output character array
*             received - the amount of bytes received
* Return: int for success or failure
-----------------------------------------------------------------*/
static int recMessage(int *fd, struct argdata *argdata, char *response, char *out,
		struct sockaddr_storage **client_addr,
		socklen_t **server_addr_len, int received)
{
	if (received != MESPSIZE) {
		generateError(0, out, NULL);
		sendToPlayer(fd, out, *client_addr, *server_addr_len, ERRPSIZE);
		return 1;
	}
	memcpy(out, response, MAXPSIZE);
	generateMessage(argdata, out);
	broadcastMessage(fd, argdata, out);

	return 1;
}

/*-----------------------------------------------------------------
* Function: recAck
* Purpose: Handle an ack packet
* Parameters: fd - file descriptor for the socket
*             argdata - the main game state struct
*             response - the output character array
*             out - the output character array
*             received - the amount of bytes received
* Return: int for success or failure
-----------------------------------------------------------------*/
static int recAck(int *fd, struct argdata *argdata, char *response, char *out,
		struct sockaddr_storage **client_addr,
		socklen_t **server_addr_len, int received)
{
	int retVal;
	uint16_t seq;

	if (received != ACKPSIZE) {
		generateError(0, out, NULL);
		sendToPlayer(fd, out, *client_addr, *server_addr_len, ERRPSIZE);
		return 1;
	}

	retVal = matchIP(argdata, *client_addr, *server_addr_len);
	if (retVal == -1) {
		generateError(0, out, NULL);
		sendToPlayer(fd, out, *client_addr, *server_addr_len, ERRPSIZE);
		return 1;
	}

	seq = ntohs(*((uint16_t *)(&response[ACK_SEQ])));

	msgRemoveEvent(argdata->msglist, retVal, seq);

	return 1;
}

/*determine the course of action from the opcode*/
int opSwitch(int *fd, struct argdata *argdata, char *response, char *out,
			struct sockaddr_storage **client_addr,
			socklen_t **server_addr_len, int received)
{
	/*get the opcode*/
	char opcode = response[OPCOD_OFF];

	/*switch based on the opcode*/
	switch (opcode) {
	case UPDATE: /*status update*/
		generateState(argdata, 0, out);
		sendToPlayer(fd, out, *client_addr, *server_addr_len, MAXPSIZE);
		break;
	case CONNECT: /*connect request*/
		connection(fd, argdata, response, out, client_addr,
			server_addr_len, received);
		break;
	case BET: /*bet*/
		recBet(fd, argdata, response, out, client_addr,
			server_addr_len, received);
		break;
	case STAND: /*stand*/
		recStand(fd, argdata, response, out, client_addr,
			server_addr_len, received);
		break;
	case HIT: /*hit*/
		recHit(fd, argdata, response, out, client_addr,
			server_addr_len, received);
		break;
	case QUIT: /*quit*/
		recQuit(fd, argdata, response, out, client_addr,
			server_addr_len, received);
		break;
	case ERROR: /*error*/
		fprintf(stderr, "%s\n", "Why is the client sending an error, Nick?");
		break;
	case MESSAGE: /*message*/
		recMessage(fd, argdata, response, out, client_addr,
			server_addr_len, received);
		break;
	case ACK: /*ack*/
		recAck(fd, argdata, response, out, client_addr,
			server_addr_len, received);
		break;
	default:
		break;
	}

	/*free the connection information if it is no longer needed*/
	if (*client_addr != NULL)
		free(*client_addr);
	if (*server_addr_len != NULL)
		free(*server_addr_len);
	*client_addr = NULL;
	*server_addr_len = NULL;

	return 1;
}
