/*-----------------------------------------------------------------------------
* common.h
* Jordan McCaskill
* CMPT 361
* Assignment 2
*
* Defines, structs and prototypes for common.c
-----------------------------------------------------------------------------*/

#ifndef COMMON_H
#define COMMON_H

#include <stdint.h> /*uintxx_t*/

/*Packet Sizes*/
#define MAXPSIZE 320
#define CONPSIZE 13
#define ERRPSIZE 142
#define MESPSIZE 165
#define ACKPSIZE 5
#define ERRORMSG 140

/*Game Values*/
#define MAX_PLAYERS 7
#define MAXDECKS 10
#define MAXMONEY 4294967295
#define MAXTIMER 45
#define MINTIMER 10
#define P_DECK_SIZE 21
#define MAXNAMESIZE 12

/*Offset Values*/
#define OPCOD_OFF 0
#define RESPA_OFF 1
#define GSEQU_OFF 5
#define MMBET_OFF 7
#define APLAY_OFF 11
#define DCARD_OFF 12
#define UNAME_OFF 33
#define PBANK_OFF 45
#define PLBET_OFF 49
#define PCARD_OFF 53
#define PSTEP_OFF 41
#define STATECMP 12
#define CONNAMEOFF 1
#define MSGOFF 5
#define ACK_SEQ 1

/*Timeout/Broadcast Values*/
#define DEALER_SENDS 3
#define REBROADCAST 1
#define DEALER_CAST 1
#define SEL_SEC_TIMEOUT 0
#define SEL_USEC_TIMEOUT 100000
#define USEC_SEC 1000000

/*default values*/
#define DEF_MONEY 100
#define DEF_DECKS 2
#define DEF_TIMER 15
#define DEF_MBET 1
#define DEF_PORT "4420"

/*Card values*/
#define ACE 1
#define TWO 2
#define THREE 3
#define FOUR 4
#define FIVE 5
#define SIX 6
#define SEVEN 7
#define EIGHT 8
#define NINE 9
#define TEN 10
#define JACK 11
#define QUEEN 12
#define KING 13
#define ACE_VALUE 11

/*Card suites*/
#define HEARTS 0
#define DIAMONDS 1
#define SPADES 2
#define CLUBS 3

/*Struct which handles all major portions of the flow
*  and status of the game. This struct should be
*  created using argdataBuild() and destroyed using
*  argdataDestroy() in order to eliminate memory leaks*/
struct argdata {
	struct deckhead *mainDeck; /*main card deck*/
	struct deckhead *discard; /*discard deck*/
	struct playerhead *players; /*player list*/
	uint16_t gameSeq; /*sequence number for the game*/
	uint16_t chatSeq; /*sequence number for the chat*/
	int decks; /*number of decks*/
	uint32_t money; /*amount of starting money*/
	int timer; /*the amount of time for a timeout*/
	uint32_t mBet; /*minimum bet*/
	char *port; /*port number*/
	struct playernode **seats; /*list of active players*/
	char *dCards; /*the dealer's cards*/
	char aPlayer; /*the currently active player*/
	int gamestate; /*the current state of the game (ie. betting etc)*/
	int startTime; /*the counter for the current play timeout*/
	int rebroadcast; /*the counter for the current rebroadcast*/
	int dealerSends; /*the amount of times the dealer will broadcast*/
	struct msghead *msglist; /*list of pending messages*/
};


/*-----------------------------------------------------------------
* Function: argdataBuild
* Purpose: Build the argdata struct
* Parameters: None
* Return: A pointer to the malloc'd argdata struct
-----------------------------------------------------------------*/
struct argdata *argdataBuild();

/*-----------------------------------------------------------------
* Function: argdataDestroy
* Purpose: Destroy the argdata struct
* Parameters: argdata - the argdata struct
* Return: integer for success or failure
-----------------------------------------------------------------*/
int argdataDestroy(struct argdata *argdata);

/*-----------------------------------------------------------------
* Function: printState
* Purpose: print out the state of the game
* Parameters: argdata - the main game state struct
* Return: int for success or failure
-----------------------------------------------------------------*/
int printState(struct argdata *argdata);

#endif /*COMMON_H*/
