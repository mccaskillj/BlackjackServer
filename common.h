#ifndef COMMON_H
#define COMMON_H

#include <stdint.h> /*uintxx_t*/

#define OPCOD_OFF 0
#define RESPA_OFF 1
#define GSEQU_OFF 5
#define MMBET_OFF 7
#define APLAY_OFF 11
#define DCARD_OFF 12
#define UNAME_OFF 33
#define PBANK_OFF 45
#define PLBET_OFF 49
#define PCARD_OFF 74
#define PSTEP_OFF 41
#define MAXPSIZE 320

struct argdata
{
	struct deckhead *mainDeck;
	struct deckhead *discard;
	struct playerhead *players;
	uint16_t gameSeq;
	uint16_t chatSeq;
	int decks;
	uint32_t money;
	int timer;
	uint32_t mBet;
	char *port;
	struct playernode **seats;
	char *dCards;
	char aPlayer;
};

struct argdata *argdataBuild();
int argdataDestroy(struct argdata *argdata);

#endif /*COMMON_H*/