#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>

#include "common.h"
#include "playerlist.h"
#include "decklist.h"
#include "state.h"
#include "game.h"

int generateState(struct argdata *argdata, char opcode, char *out){
	memset(out, 0, MAXPSIZE);

	char aPlayer = argdata->aPlayer;

	strncpy(out,&opcode,1);
	*((uint16_t*)(&out[GSEQU_OFF])) = htons(argdata->gameSeq); /*set seq number*/
	*((uint32_t*)(&out[MMBET_OFF])) = htonl(argdata->mBet); /*set min bet*/
	strncpy(&out[APLAY_OFF],&aPlayer,1); /*set active player*/
	strncpy(&out[DCARD_OFF],argdata->dCards, 21); /*set dealer cards*/
	for (int i = 0; i < 7; i++){
		if (argdata->seats[i] != NULL){
			strncpy(&out[UNAME_OFF+(i*PSTEP_OFF)],argdata->seats[i]->name,12);
			*((uint32_t*)(&out[PBANK_OFF+(i*PSTEP_OFF)])) = htonl(argdata->seats[i]->money);
			*((uint32_t*)(&out[PLBET_OFF+(i*PSTEP_OFF)])) = htonl(argdata->seats[i]->bet);
			strncpy(&out[PCARD_OFF+(i*PSTEP_OFF)],argdata->seats[i]->cards,21);
		}
	}

	return 1;
}

int broadcast(int *fd, struct argdata *argdata, char* out){\
	struct playernode *player;
	for (int i = 0; i<7; i++){
		if(argdata->seats[i] != NULL && argdata->seats[i]->player_addr != NULL){
			player = argdata->seats[i];
			sendto(*fd, out, MAXPSIZE, 0, (struct sockaddr*)player->player_addr,*player->server_addr_len);
		}
	}
	return 0;
}

int generateError(char error, char *out){
	memset(out, 0, MAXPSIZE);
	char value = 6;
	char *msg;

	strncpy(out, &value, 1);
	strncpy(&out[1], &error, 1);

	switch (error){
	case 0:
		msg = "General: error is unspecified";
		break;
	case 1:
		msg = "No Money: insufficient money to make bet";
		break;
	case 2:
		msg = "Table Full: no seats remain at table";
		break;
	case 3:
		msg = "Name: name is already in use/unusable";
		break;
	case 4:
		msg = "Invalid Name: name entered is not valid";
		break;
	case 5:
		msg = "Timeout: failed to make move in required time";
		break;
	default:
		msg = "";
		break;
	}

	strncpy(&out[2], msg, 139);

	return 1;
}

int generateMessage(struct argdata *argdata, char* msg){
	*((uint32_t*)(&msg[5])) = htonl(argdata->chatSeq);

	return 1;
}

int matchIP(struct argdata *argdata, struct sockaddr_storage *client_addr, socklen_t *server_addr_len){
	int i, err;
	char argIP[1025];
	char posIP[1025];
	char argPort[32];
	char posPort[32];
	for (i = 0; i < 7; i++){
		if (argdata->seats[i] != NULL && argdata->seats[i]->player_addr != NULL){
			err = getnameinfo((struct sockaddr *)argdata->seats[i]->player_addr,
					*argdata->seats[i]->server_addr_len, argIP,sizeof(argIP),
					argPort, sizeof(argPort), NI_NUMERICHOST | NI_NUMERICSERV);
			if (err != 0)
				return -1;
			err = getnameinfo((struct sockaddr *)client_addr, *server_addr_len,
					posIP,sizeof(argIP), posPort,
					sizeof(argPort), NI_NUMERICHOST | NI_NUMERICSERV);
			if (err != 0)
				return -1;

			if(strncmp(argIP, posIP,1025) == 0 && strncmp(argPort, posPort, 32) == 0)
				return i;
		}
	}

	return -1;
}

int opSwitch(int *fd, struct argdata *argdata, char* response, char* out,
			struct sockaddr_storage **client_addr, socklen_t **server_addr_len){
	char opcode = response[0];
	char card, aPlayer;
	int retVal;
	uint32_t bet;
	switch(opcode){
	case 0: /*status update*/
		generateState(argdata,0,out);
		sendto(*fd, out, MAXPSIZE, 0, (struct sockaddr*)*client_addr,**server_addr_len);
		break;
	case 1: /*connect request*/
		retVal = pConnect(argdata, client_addr, server_addr_len, response);
		if (retVal <= 0){
			generateError(retVal*-1,out);
			sendto(*fd, out, MAXPSIZE, 0, (struct sockaddr*)*client_addr,**server_addr_len);
		}else{
			generateState(argdata,0,out);
			sendto(*fd, out, MAXPSIZE, 0, (struct sockaddr*)*client_addr,**server_addr_len);
			*client_addr = NULL;
			*server_addr_len = NULL;
		}
		break;
	case 2: /*bet*/
		retVal = matchIP(argdata,*client_addr, *server_addr_len);
		if (1){
			bet = ntohl(*((uint32_t *)(&response[RESPA_OFF])));
			argdata->seats[argdata->aPlayer-1]->bet = bet;
			generateState(argdata,0,out);
			aPlayer = 0;
			strncpy(&out[APLAY_OFF], &aPlayer, 1);
			sendto(*fd, out, MAXPSIZE, 0, (struct sockaddr*)*client_addr,**server_addr_len);
		}
		break;
	case 3: /*stand*/
		retVal = matchIP(argdata,*client_addr, *server_addr_len);
		if (retVal == argdata->aPlayer-1){
			changePlayer(argdata);
			generateState(argdata,0,out);
			broadcast(fd ,argdata, out);
		}
		break;
	case 4: /*hit*/
		retVal = matchIP(argdata,*client_addr, *server_addr_len);
		if (1){
			card = getCard(argdata->mainDeck,argdata->discard);
			addToDeck(argdata->seats[argdata->aPlayer-1]->cards,card);
			generateState(argdata,0,out);
			broadcast(fd, argdata, out);
		}
		break;
	case 5: /*quit*/
		retVal = matchIP(argdata, *client_addr, *server_addr_len);
		if (1){
			free(argdata->seats[argdata->aPlayer-1]->player_addr);
			free(argdata->seats[argdata->aPlayer-1]->server_addr_len);
			argdata->seats[argdata->aPlayer-1]->player_addr = NULL;
			argdata->seats[argdata->aPlayer-1]->server_addr_len = NULL;
			argdata->seats[argdata->aPlayer-1]->active = -1;

		}
		printf("%s\n", "kill player");
		break;
	case 6: /*error*/
		printf("%s\n", "why is the client sending an error, Nick?");
		break;
	case 7: /*message*/
		memcpy(out,response,MAXPSIZE);
		generateMessage(argdata, out);
		broadcast(fd, argdata, out);
		break;
	case 8: /*ack*/
		printf("%s\n", "got an ack");
		break;
	default:
		break;
	}

	if (*client_addr != NULL)
		free(*client_addr);
	if (*server_addr_len != NULL)
		free(*server_addr_len);
	*client_addr = NULL;
	*server_addr_len = NULL;

	return 1;
}