#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>

#include "networking.h"
#include "memctl.h"

#define MAX_PLAYERS 1

void sendAll( char * message );
void sendTo( int playerID, char * message );

//GLOBAL VARIABLES
int current_players = 0;
struct sockpair players[MAX_PLAYERS];


//helper to send a message to all players
void sendAll(char * message) {
  int i;
  for (i = 0; i < current_players; i++) {
    sendTo(i, message);
  }
}


//helper to send a message to specific player
void sendTo(int playerID, char * message) {
  struct sockpair player = players[playerID];
  write(player.listener, message, sizeof(message));
}


int main() {
  int sd, chat_connection, listener_connection;
  sd = server_setup();

  //fill up the game
  while (current_players < MAX_PLAYERS) { 
    chat_connection = server_connect( sd );
    listener_connection = server_connect( sd );
    
    struct sockpair sp;
    sp.chat = chat_connection;
    sp.listener = listener_connection;
    sp.shmid = setupShm();
    sp.semid = setupSem();

    players[current_players] = sp;
    current_players++;
    printf("number of players in game: %d\n", current_players);
  }
  printf("enough players\n");
  close(sd);
  
  int i;
  for (i = 0; i < current_players; i++){
    struct sockpair player = players[i];
    int chat = player.chat;
    int listener = player.listener;
    int f = fork();
    printf("%s\n\n", "testing");
    if (f == 0){ //SUBSERVER
      char beginCode[] = "***BEGIN***";
      write(listener, beginCode, sizeof(beginCode));
      close(listener);
      char buffer[MESSAGE_BUFFER_SIZE];
      while (read( chat, buffer, sizeof(buffer) )) {
	printf("Received from player %d: %s\n", i, buffer);
	//put stuff into shm
	char ** shm = shmat(player.shmid, 0, 0);
	//messages are seperated by newline
	*shm = strcat(*shm, buffer);
      }
    }
  }
  
  //main server check shared memory in a loop
  //when one person types msg, sends to everyone
  while (1) {
    //i declared above
    for (i = 0; i < current_players; i++) {
      struct sockpair player = players[i];
      semdown(player.semid);
      char ** shm = shmat(player.shmid, 0, 0);
      sendAll(*shm);
      *shm = ""; //clear shm after sending
      semup(player.semid);
    }
  }
  
  return 0;
}
