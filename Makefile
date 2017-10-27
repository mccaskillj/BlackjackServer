# -----------------------------------------------------------------------------
# Makefile
# Jordan McCaskill
# CMPT 361
# 
# Makefile for the creation of the blackjack
# -----------------------------------------------------------------------------

# CFLAGS additions
CFLAGS=-Wall -std=c99 -g -pedantic -D_POSIX_C_SOURCE=201112L

# Phony target declaration
.PHONY: all clean test

# make qotdd executable
all: blackjack

# run the compiler on the .o files to create the executbale
blackjack: blackjack.o decklist.o playerlist.o state.o game.o common.o actionlist.o
	$(CC) $(LDFLAGS) -o $@ $^

# Create blackjack.o
blackjack.o: blackjack.c blackjack.h

#Create decklist.o
decklist.o: decklist.c decklist.h

#Create playerlist.o
playerlist.o: playerlist.c playerlist.h

#Create state.o
state.o: state.c state.h

#Create game.o
game.o: game.c game.h

#Create common.o
common.o: common.c common.h

#Create actionlist.o
actionlist.o: actionlist.c actionlist.h

#Automate the compiling and valgrind into one step
test: all
	valgrind --leak-check=full ./blackjack -p 1555

# clean up the extra files
clean:
	$(RM) blackjack blackjack.o decklist.o playerlist.o state.o game.o common.o actionlist.o
