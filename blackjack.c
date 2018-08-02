/*-----------------------------------------------------------------------------
* blackjack.c
* Jordan McCaskill
* CMPT 361
* Assignment 2
*
* Runs the main server loop and handles all set-up and tear-down
-----------------------------------------------------------------------------*/
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
#include <sys/time.h>

#include "playerlist.h"
#include "decklist.h"
#include "state.h"
#include "game.h"
#include "common.h"

/*global for the control of the while loop and handling interrupts*/
static int resume = 1;

/*-----------------------------------------------------------------
* Function: handler
* Purpose: Signal handler for interrupts
* Parameters: signo - the signal number
* Return: None
-----------------------------------------------------------------*/
static void handler(int signo)
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

/*-----------------------------------------------------------------
* Function: usage
* Purpose: Print the usage message and kill the program
* Parameters: program - name of the program
* Return: None
-----------------------------------------------------------------*/
static void usage(char *program)
{
	printf("usage: %s [-d n][-m n][-t n][-p n]\n", program);
	printf("\nwhere:\n");
	printf("\t-m n\tAmount of starting money (default = $%d)\n", DEF_MONEY);
	printf("\t-d n\tThe number of decks (default = %d)\n", DEF_DECKS);
	printf("\t-t n\tThe wait timer duration where 10 sec < n < 45 sec\n\t\t(default = %d sec)\n",
		DEF_TIMER);
	printf("\t-p n\tThe port which accepts connections (default = %s)\n", DEF_PORT);
	printf("\t-b n\tThe minimum bet (default = $%d)\n", DEF_MBET);
	printf("\t-h\tShow this usage message\n");
	exit(1);
}

/*-----------------------------------------------------------------
* Function: createFD
* Purpose: create the socket file descriptor
* Parameters: argdata - the main game state struct
*             fd - file descriptor for the socket
* Return: int for success or failure
-----------------------------------------------------------------*/
static int createFD(struct argdata *argdata, int *fd)
{
	int sErr;
	struct addrinfo hints, *res, *cur;

	memset(&hints, 0, sizeof(hints));

	/*declare socket options*/
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	hints.ai_flags = AI_PASSIVE | AI_V4MAPPED;

	/*get the information about the connection*/
	sErr = getaddrinfo(NULL, argdata->port, &hints, &res);
	/*check for error from getaddrinfo*/
	if (sErr != 0) {
		fprintf(stderr, "getaddrinfo: %s%d\n",
			gai_strerror(sErr), sErr);
		freeaddrinfo(res);
		return 0;
	}

	/*check all the connections returned from getaddrinfo*/
	for (cur = res; cur != NULL; cur = cur->ai_next) {

		/*create the socket*/
		*fd = socket(cur->ai_family, cur->ai_socktype,
			cur->ai_protocol);
		/*check for error in socket creation*/
		if (*fd == -1) {
			perror("socket");
			continue;
		}

		/*bind the socket*/
		sErr = bind(*fd, cur->ai_addr, cur->ai_addrlen);
		/*check for error binding*/
		if (sErr == -1) {
			perror("bind");
			close(*fd);
			continue;
		}

		break;
	}

	/*check if loop exited because of failure to connect*/
	if (cur == NULL) {
		fprintf(stderr, "Socket creation failed\n");
		freeaddrinfo(res);
		return 0;
	}

	/*free addrinfo struct*/
	freeaddrinfo(res);
	res = NULL;

	return 1;
}

/*-----------------------------------------------------------------
* Function: getTimeDiff
* Purpose: get the amount of time that passed while blocking on
*          select
* Parameters: timeout - the timeval struct passed to select
* Return: int for success or failure
-----------------------------------------------------------------*/
static int getTimeDiff(struct timeval *timeout)
{
	int total = 0;
	/*calculate the seconds difference in microseconds*/
	total += SEL_SEC_TIMEOUT - timeout->tv_sec * USEC_SEC;
	/*calculate the microseconds difference*/
	total += SEL_USEC_TIMEOUT - timeout->tv_usec;

	return total;
}

/*-----------------------------------------------------------------
* Function: getPacket
* Purpose: receive a packet from the socket
* Parameters: argdata - the main game state struct
*             fd - file descriptor for the socket
*             buff - the buffer to read into
*             client_addr - the clients address information
*             server_addr_len - the length of client_addr
* Return: int for success or failure
-----------------------------------------------------------------*/
static int getPacket(struct argdata *argdata, int *fd, char *buff,
	struct sockaddr_storage **client_addr, socklen_t **server_addr_len)
{
	struct timeval timeout;
	int timePassed;
	int received;

	/*declare the read set*/
	fd_set masterReadSet;

	FD_ZERO(&masterReadSet);
	FD_SET(*fd, &masterReadSet);

	/*set the timeout*/
	timeout.tv_sec = SEL_SEC_TIMEOUT;
	timeout.tv_usec = SEL_USEC_TIMEOUT;

	/*block on the socket using select*/
	int ready = select(*fd + 1, &masterReadSet, NULL, NULL, &timeout);
	/*determine the amount of time select blocked for and update timers*/
	timePassed = getTimeDiff(&timeout);
	argdata->startTime += timePassed;
	argdata->rebroadcast += timePassed;
	/*check if select exited with an error*/
	if (ready == -1) {
		perror("select");
		free(*client_addr);
		free(*server_addr_len);
		*client_addr = NULL;
		*server_addr_len = NULL;
		return 0;
	}
	/*check if select timed out*/
	else if (ready == 0) {
		free(*client_addr);
		free(*server_addr_len);
		*client_addr = NULL;
		*server_addr_len = NULL;
		return 0;
	}
	/*retreive the data from the active socket*/
	else if (FD_ISSET(*fd, &masterReadSet)) {
		received = recvfrom(*fd, buff, MAXPSIZE, 0,
			(struct sockaddr *)*client_addr, *server_addr_len);
		/*initial check that packet is the correct size for a state*/
		if (received != MAXPSIZE &&
			received != CONPSIZE &&
			received != ERRPSIZE &&
			received != MESPSIZE &&
			received != ACKPSIZE){
			free(*client_addr);
			free(*server_addr_len);
			*client_addr = NULL;
			*server_addr_len = NULL;
			return 0;
		}
	}

	return received;
}

/*-----------------------------------------------------------------
* Function: server
* Purpose: set-up the server and then run the main server loop
* Parameters: argdata - the main game state struct
* Return: int for success or failure
* Reference: http://beej.us/guide/bgnet/output/html/singlepage/
*                 bgnet.html#sendtorecv
-----------------------------------------------------------------*/
static int server(struct argdata *argdata)
{
	int fd;
	int received;
	struct sockaddr_storage *client_addr;
	socklen_t *server_addr_len;

	/*create the socket*/
	if (createFD(argdata, &fd) == 0)
		return 0;

	/*set space for the output and input strings*/
	char *state = malloc(sizeof(char)*MAXPSIZE);

	if (state == NULL)
		resume = 0;

	char *response = calloc(MAXPSIZE, sizeof(char));

	if (response == NULL)
		resume = 0;

	/*print the initial state*/
	printState(argdata);

	/*start the server loop*/
	while (resume) {
		/*set space for connection structs*/
		client_addr = malloc(sizeof(struct sockaddr_storage));
		if (client_addr == NULL) {
			resume = 0;
			continue;
		}
		server_addr_len = malloc(sizeof(socklen_t));
		if (server_addr_len == NULL) {
			resume = 0;
			continue;
		}
		*server_addr_len = sizeof(client_addr);

		/*get a packet from the socket and check if anything
		*  was received*/
		if ((received = getPacket(argdata, &fd, response,
			&client_addr, &server_addr_len)) == 0) {
			proceedGame(&fd, argdata, state);
			continue;
		}

		/*parse based off the opcode and free the connection
		*  structs if they are unused*/
		opSwitch(&fd, argdata, response, state,
			&client_addr, &server_addr_len, received);

		/*move game state forward*/
		proceedGame(&fd, argdata, state);

		/*print the state*/
		printState(argdata);

		/*zero out the response for reuse*/
		memset(response, '\0', MAXPSIZE);
	}

	/*check if interrupt was received or termination was due to failure*/
	if (resume == 0)
		printf("Exiting\n");

	/*free input and output strings*/
	if (response != NULL)
		free(response);
	response = NULL;

	if (state != NULL)
		free(state);
	state = NULL;

	close(fd);

	return 1;
}

/*-----------------------------------------------------------------
* Function: main
* Purpose: main function
* Parameters: argc - the number of arguments
*             argv - the list of arguments
* Return: int for success or failure
-----------------------------------------------------------------*/
int main(int argc, char *argv[])
{
	int c;
	int val;

	/*set up signal handler*/
	struct sigaction pSig;

	pSig.sa_handler = handler;
	sigemptyset(&pSig.sa_mask);
	pSig.sa_flags = SA_SIGINFO;
	sigaction(SIGINT, &pSig, NULL);

	/*build argdata*/
	struct argdata *argdata = argdataBuild();

	/*parse the command line arguments*/
	while ((c = getopt(argc, argv, "d:m:t:p:b:h")) != -1)
		switch (c) {
		case 'd': /*decks*/
			val = strtol(optarg, NULL, 10);
			if (errno == ERANGE)
				usage(basename(argv[0]));
			if (val <= 0 || val > MAXDECKS)
				usage(basename(argv[0]));
			argdata->decks = val;
			break;
		case 'm': /*money*/
			val = strtol(optarg, NULL, 10);
			if (errno == ERANGE)
				usage(basename(argv[0]));
			if (val <= 1 || val > MAXMONEY)
				usage(basename(argv[0]));
			argdata->money = val;
			/*4294967296*/
			break;
		case 't': /*timer*/
			val = strtol(optarg, NULL, 10);
			if (errno == ERANGE)
				usage(basename(argv[0]));
			if (val < MINTIMER || val > MAXTIMER)
				usage(basename(argv[0]));
			argdata->timer = val;
			break;
		case 'p': /*port*/
			if (strlen(optarg) > 5)
				usage(basename(argv[0]));
				strcpy(argdata->port, optarg);
			break;
		case 'b': /*minimum bet*/
			val = strtol(optarg, NULL, 10);
			if (errno == ERANGE)
				usage(basename(argv[0]));
			if (val < 1 || val > MAXMONEY)
				usage(basename(argv[0]));
			argdata->mBet = val;
			break;
		case 'h': /*help*/
			usage(basename(argv[0]));
			break;
		case '?': /*unknown*/
			usage(basename(argv[0]));
		default:
			exit(1);
		}

	/*create the deck*/
	argdata->mainDeck = deckinit(argdata->decks);

	/*start the server*/
	server(argdata);

	/*destroy argdata*/
	argdataDestroy(argdata);

	return 0;
}
