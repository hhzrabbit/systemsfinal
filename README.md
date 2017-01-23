# Mafia 

Systems Final Project | Fall 2016 | Mr. Dyrland-Weaver

### Project Members: 
* Haley Zeng (pd 4)
* Joel Ye (pd 10)
* William Xiang (pd 10)

### About the project
Mafia is a multiplayer game in which a group of townspeople try to live to the end and/or fulfill their objectives. Some have special abilities to use during the night, while others can only influence the outcome of the game based on their vote in a democratic lynching process during the day. Discussion takes place in public chat, and the day ends when time runs out or a consensus to lynch is reached. *Detailed gameplay information available [here](http://www.menconi.com/games/mafia.html).*

### Non-standard libraries needed
* libncurses5-dev
* libncursesw5-dev

### To compile
For each computer, on the top level of our project directory, run:
```
$ make
```

### To run
One computer acts as the server. On this computer, run:
```
$ ./server.out
```
On eight other computers, run:
```
$ ./client.out
```

### Files included in this project
* chat_server.c
* client_ncurses.c
* networking.c
* networking.h
* memctl.c
* memctl.h
* makefile

### Bugs
* If a player who is alive disconnects during the game, undefined behavior occurs
