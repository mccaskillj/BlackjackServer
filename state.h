/*-----------------------------------------------------------------------------
* state.h
* Jordan McCaskill
* CMPT 361
* Assignment 2
*
* Defines and prototypes for state.c
-----------------------------------------------------------------------------*/

#ifndef STATE_H
#define STATE_H

#include "common.h" /*for argdata struct*/

/*defines for IP addresses and host port sizes*/
#define MAXIPSIZE 1025
#define MAXHOSTSIZE 32

/*Error codes*/
#define GENERALERR 0
#define MONEYERR 1
#define TABLEERR 2
#define SEATERR 3
#define NAMEERR 4
#define TIMEERR 5

/*opcodes*/
#define UPDATE 0
#define CONNECT 1
#define BET 2
#define STAND 3
#define HIT 4
#define QUIT 5
#define ERROR 6
#define MESSAGE 7
#define ACK 8

/*-----------------------------------------------------------------
* Function: generateState
* Purpose: Generates the state to send to the client
* Parameters: argdata - the main game state struct
*             opcode - the op-code for the packet
*             out - the output character array
* Return: int for success or failure
-----------------------------------------------------------------*/
int generateState(struct argdata *argData, char opcode, char *out);

/*-----------------------------------------------------------------
* Function: opSwitch
* Purpose: determines the opcode of the packet and sends it to be
*          processed
* Parameters: fd - file descriptor for the socket
*             argdata - the main game state struct
*             response - the output character array
*             out - the output character array
*             client_addr - the clients address information
*             server_addr_len - the length of client_addr
*             received - the amount of bytes received
* Return: int for success or failure
-----------------------------------------------------------------*/
int opSwitch(int *fd, struct argdata *argdata, char *response, char *out,
	struct sockaddr_storage **client_addr,
	socklen_t **server_addr_len, int received);

/*-----------------------------------------------------------------
* Function: broadcast
* Purpose: broadcast the state to all clients
* Parameters: fd - file descriptor for the socket
*             argdata - the main game state struct
*             out - the output character array
* Return: int for success or failure
-----------------------------------------------------------------*/
int broadcast(int *fd, struct argdata *argdata, char *out);

/*-----------------------------------------------------------------
* Function: generateError
* Purpose: generate the error message to send to the client
* Parameters: out - the output character array
*             error - the error code
*             gMsg - the optional error message for general error
* Return: int for success or failure
-----------------------------------------------------------------*/
int generateError(char error, char *out, char *gMsg);

/*-----------------------------------------------------------------
* Function: sendToPlayer
* Purpose: 
* Parameters: fd - file descriptor for the socket
*             out - the output character array
*             client_addr - the clients address information
*             server_addr_len - the length of client_addr
*             pSize - size of the packet to print
* Return: int for success or failure
-----------------------------------------------------------------*/
int sendToPlayer(int *fd, char *out,
	struct sockaddr_storage *client_addr,
	socklen_t *server_addr_len, int pSize);

#endif /*STATE_H*/
