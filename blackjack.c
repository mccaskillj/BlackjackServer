#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>

#include "playerlist.h"
#include "blackjack.h"
#include "decklist.h"
#include "state.h"
#include "game.h"
#include "common.h"

int resume = 1;

void handler(int signo)
{
	/*switch statement to determine signal*/
	switch (signo) {
	case SIGINT:
		resume = 0;
		break;
	default: /*Should never get this case*/
		break;
	}
}

void usage(char *program)
{
	printf("usage: %s [-d n][-m n][-t n][-p n]\n", program);
	printf("\nwhere:\n");
	printf("\t-m n\tAmount of starting money (default = $100)\n");
	printf("\t-d n\tThe number of decks (default = 2)\n");
	printf("\t-t n\tThe wait timer duration where 10 sec < n < 45 sec\n\t\t(default = 15 sec)\n");
	printf("\t-p n\tThe port which accepts connections (default = 4420)\n");
	printf("\t-b n\tThe minimum bet (default = $1)\n");
	printf("\t-h\tShow this usage message\n");
	exit(1);
}

int createFD(struct argdata *argdata, int *fd){
	int sErr;
	struct addrinfo hints, *res, *cur;

	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP; 
	hints.ai_flags = AI_PASSIVE | AI_V4MAPPED;


	sErr = getaddrinfo(NULL, argdata->port, &hints, &res);
	if (sErr != 0){
		fprintf(stderr, "getaddrinfo: %s%d\n", gai_strerror(sErr), sErr);
		return 0;
	}

	for (cur = res; cur != NULL; cur=cur->ai_next){

		*fd = socket(cur->ai_family, cur->ai_socktype, cur->ai_protocol);
		if (*fd == -1) {
			perror("socket");
			continue;
		}

		sErr = bind(*fd, cur->ai_addr, cur->ai_addrlen);
		if (sErr == -1) {
			perror("bind");
			close(*fd);
			continue;
		}

		break;
	}

	if (cur == NULL){
		fprintf(stderr, "Socket creation failed\n");
		return 0;
	}

	freeaddrinfo(res);
	res = NULL;

	return 1;
}

int getPacket(int *fd, char* buff, struct sockaddr_storage **client_addr, socklen_t **server_addr_len){
	
	fd_set masterReadSet;
	FD_ZERO(&masterReadSet);
	FD_SET(*fd, &masterReadSet);

	int ready = select(*fd+1,&masterReadSet, NULL, NULL, NULL);
	if (ready == -1){
			perror("select");
			free(*client_addr);
			free(*server_addr_len);
			*client_addr = NULL;
			*server_addr_len = NULL;
			return 0;
		}
	else if (FD_ISSET(*fd, &masterReadSet)){
		recvfrom(*fd, buff, MAXPSIZE, 0, (struct sockaddr *)*client_addr, *server_addr_len);
	}

	return 1;
}

/*http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#sendtorecv*/
int server(struct argdata *argdata){
	int fd;
	struct sockaddr_storage *client_addr;
	socklen_t *server_addr_len;

	if(createFD(argdata, &fd) == 0){
		return 0;
	}

	char* state = malloc(sizeof(char)*MAXPSIZE);
	char* response = calloc(MAXPSIZE, sizeof(char));

	while(resume){
		client_addr = malloc(sizeof(struct sockaddr_storage));
		server_addr_len = malloc(sizeof(socklen_t));
		*server_addr_len = sizeof(client_addr);
		if(getPacket(&fd, response, &client_addr, &server_addr_len) == 0){
			continue;
		}

		if(opSwitch(&fd, argdata,response,state, &client_addr, &server_addr_len)==0){
			free(client_addr);
			free(server_addr_len);
			memset(response, '\0', MAXPSIZE);
			continue;
		}

		memset(response, '\0', MAXPSIZE);
		for(int i=0;i<MAXPSIZE;i++){
			printf("%02X ",(unsigned char)state[i]);
			if (i%20 == 19){
				printf("\n");
			}
		}
		printf("\n");
	}

	if (resume == 0){
		printf("Exiting\n");
	}

	free(response);
	response = NULL;
	free(state);
	state = NULL;

	close(fd);

	return 1;
}

int main(int argc, char *argv[])
{
	int c;
	int val;

	struct sigaction pSig;
	pSig.sa_handler = handler;
	sigemptyset(&pSig.sa_mask);
	pSig.sa_flags = SA_SIGINFO;
	sigaction(SIGINT, &pSig, NULL);

	struct argdata *argdata = argdataBuild();

	while ((c = getopt(argc, argv, "d:m:t:p:b:h")) != -1)
		switch (c) {
		case 'd':
			val = strtol(optarg, NULL, 10);
			if (errno == ERANGE)
				usage(basename(argv[0]));
			if (val <= 0 || val > 10)
				usage(basename(argv[0]));
			argdata->decks = val;
			break;
		case 'm':
			val = strtol(optarg, NULL, 10);
			if (errno == ERANGE)
				usage(basename(argv[0]));
			if (val <= 1 || val > 4294967295)
				usage(basename(argv[0]));
			argdata->money = val;
			/*4294967296*/
			break;
		case 't':
			val = strtol(optarg, NULL, 10);
			if (errno == ERANGE)
				usage(basename(argv[0]));
			if (val < 10 || val > 45)
				usage(basename(argv[0]));
			argdata->timer = val;
			break;
		case 'p':
			if (strlen(optarg) > 5)
				usage(basename(argv[0]));
				strcpy(argdata->port, optarg);
			break;
		case 'b':
			val = strtol(optarg, NULL, 10);
			if (errno == ERANGE)
				usage(basename(argv[0]));
			if (val < 1 || val > 4294967295)
				usage(basename(argv[0]));
			argdata->mBet = val;
			break;
		case 'h':
			usage(basename(argv[0]));
			break;
		case '?':
			usage(basename(argv[0]));
		default:
			exit(1);
		}

	argdata->mainDeck = deckinit(argdata->decks);

	server(argdata);

	argdataDestroy(argdata);

	return 0;
}
