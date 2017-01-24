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
#### Option 1: Networking between computers
One computer acts as the server. On this computer, run:
```
$ ./server.out
```
On eight other computers, run:
```
$ ./client.out <IP of server>
```
For example, if your server is running on 149.89.150.101, run:
```
$ ./client.out 149.89.150.101
```

#### Option 2: Networking using localhost
Open nine terminal sessions on one computer.

On one terminal sessions, run:
```
$ ./server.out
```
On eight other terminal sessions, run:
```
$ ./client.out
```


### General game-flow information
* dscussion period
  * the program is an open chatroom. 
  * special commands:
    * `\w <name of player> <message>` -- whisper to another player (other players will see that you are whispering)
    * `\nom <name of player>` -- nominate a player for lynching (three players must nominate a player in order for an accusation to occur)
* voting period
  * options
    * `yes` or `y` -- vote to lynch the player
    * `no` or `n` -- vote to not lynch the player
  * four yes's needed to lynch
  * votes will be revealed after the decision is made
* night time
  * mafia -- chat and choose assassination target
    * mafia must agree on target
  * cop -- make a guess about who is mafia
    * type name of guess into prompt
    * cop is informed if guess is correct or not

### Files included in this project
* chat_server.c
* client_ncurses.c
* networking.c
* networking.h
* memctl.c
* memctl.h
* makefile

### Features / stuff to brag about
* whispering between players
* if you type an unknown command or non-existent player name, the server will let you know and prevent you from doing harm
* ctrl-C on the server will clear out the shared memory and semaphores


### Bugs
* If a player disconnects during the game, undefined behavior occurs
* At the start of the game, each user is prompted to enter their name. Sometimes not every player receives the message "What is your name?" but regardless, the first input will always be taken as the player's name
