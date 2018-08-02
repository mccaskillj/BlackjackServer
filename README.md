# BlackjackServer

This server allows client side connections to play in a round robin blackjack table where each player plays one hand at a time with the ability to Bet, Hit and Stay. Players are removed when they run out of money. Messaging was built in however, remains untested as the client did not support the functionality. The server and client use a protocol which was designed as part of the assignment by the class. The client is currently not available here as I am working on getting permission for the executable or the code.


Protocol Description

All multibyte integers are transmitted in network byte order, i.e., big endian.

Connect Request (size 13B)
1 B: Connect Request
12 B: Username: if the username has fewer than 12 characters, it must be a NUL-terminated string 
Response: State (below) OR Error
the Username must appear at a seat
if mid-round, player will not be a "Player to Respond" until the next round

State (size 320B)
1 B: State Update (Opcode)
4 B: Response Arguments (0 for “State”)
2 B: Seq. # (+1 as the game progresses)
4 B: Minimum Bet
1 B: Player to Respond (0 is dealer; 1-based index into array below)
21 B: Dealer Cards
7 times…
12 B: Username
4 B: Bank
4 B: Bet
21 B: Cards
Client Response:
Player to Respond: Bet, Stand, or Hit (below)
If player’s Bet field is empty → Bet
Otherwise → Stand or Hit
Other Players: None
Note: State is sent out 3 times at the end of a round with the dealer’s cards; 1 second separates messages

The assignment bytes: <- sick joke 

Layout
'''
0             1                          5           7            11
      | opcode | Response Args | Seq # | Min Bet |
    11                     12                    33 
      | Active Player | Dealer Cards |
     33               45       49         53        74
P1  | Username | Bank |   Bet   | Cards |
     74               86       90         94        115
P2  | Username | Bank |   Bet   | Cards |
    115              127     131      135       156
P3  | Username | Bank |   Bet   | Cards |
    156             168      172      176       197
P4  | Username | Bank |   Bet   | Cards |
    197             209     213       217       238
P5  | Username | Bank |   Bet   | Cards |
    238             250      254      258       279
P6  | Username | Bank |   Bet   | Cards |
    279             291      295      299       320
P7  | Username | Bank |   Bet   | Cards |
'''


Bet (size same as State)
1 B: Bet (Opcode)
4 B: Response Arguments (amount of bet)
2 B: Seq. # + 1
All other fields are a copy of the last received “State” above
Server Response:
None

Stand (size same as State)
1 B: Stand (Opcode)
4 B: Response Arguments (0 for “Stand”)
2 B: Seq. # + 1
All other fields are a copy of the last received “State” above
Server Response:
None

Hit (size same as State)
1 B: Hit (Opcode)
4 B: Response Arguments (0 for “Hit”)
2 B: Seq. # + 1
All other fields are a copy of the last received “State” above
Server Response:
None

Quit (size same as State)
1 B: Quit (Opcode) // free seat in state
4 B: Response Arguments (0000)
2 B: Seq. # + 1
All other fields are a copy of the last received “State” above
Server Response:
None 

Error (size 142B)
1 B: Error (Opcode)
1 B: Error Code:
General error (0)
No money (1)
No room/seats full (2)
Name already taken/active (3)
Invalid name (4)
Timeout error (5)
140 B: Description (NUL-terminated human readable ASCII string; optional)

Message (size 165B)
1 B: Message (Opcode)
4 B: Client seq num
4 B: Server seq num (order in which the message goes into the list)
0 if the client is sending the message
Populated by the server on broadcast
12 B: Player
144 B: message (must be null terminated) (143 chars)

Message ack (size 5B)
1 B: ACK (Opcode) 
4 B: Server seq num
    
The messages are acked only when received by the server, the client retries until they receive their message back.

Sequence numbers are unsigned 4 byte in messages and acks, everywhere else they are 2 bytes.

Opcodes
Status update (0)
Connect request (1)
Bet (2)
Stand (3)
Hit (4)
Quit (5)
Error (6)
Message (7)
Ack (8)

Every packet is a fixed sized (even packets including variable length strings, anything after the nul terminator is zeroed out)

Card Values (in chars/bytes)

| Value | Encoded (1 byte) |
| ------| -----------------|
| None / Facedown | 0 |
| AH (ace of hearts) | 1 |
| 2H | 2 |
| 3H | 3 |
| ... | ... |
| AD | 14 |
| ... | ... |
| AS | 27 |
| ... | ... |
| AC | 40 |
| ... | ... |
| KC (king of clubs) | 52 |
