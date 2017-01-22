#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <signal.h>

#include "networking.h"
#include "memctl.h"

#define MAX_PLAYERS 1

struct sockpair {
  int sock_id;
  int shm_id;
  int sem_id;
};

void sendAll( char * message );
void sendTo( int playerID, char * message );
static void sighandler(int signo);

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
  printf("sending to [%d]: %s\n", playerID, message);
  struct sockpair player = players[playerID];
  write(player.sock_id, message, strlen(message));
}

//remove shm and sem in case fail
static void sighandler(int signo) {
  if (signo == SIGINT || signo == SIGQUIT || signo == SIGSEGV) {
    int i;
    for (i = 0; i < current_players; i++) {
      struct sockpair player = players[i];
      shmctl(player.shm_id, IPC_RMID, 0);
      semctl(player.sem_id, 0, IPC_RMID, 0);
      close(player.sock_id);
    }
    exit(0);
  }
}



int main() {

  //signal setup
  signal(SIGINT, sighandler);
  signal(SIGKILL, sighandler);
  signal(SIGSEGV, sighandler);
  
  int sd, client_conn;
  sd = server_setup();
  
  //fill up the game
  while (current_players < MAX_PLAYERS) { 
    client_conn = server_connect( sd );
    
    struct sockpair sp;
    sp.sock_id = client_conn;

    sp.shm_id = setupShm();
    sp.sem_id = setupSem();

    players[current_players] = sp;
    current_players++;
    printf("number of players in game: %d\n", current_players);
  }
  printf("enough players\n");
  close(sd);
  
  int i;
  for (i = 0; i < current_players; i++){
    struct sockpair player = players[i];
    int sock_id = player.sock_id;
    int f = fork();
    
    if (f == 0){ //SUBSERVER
      char beginCode[] = "***BEGIN***";
      write(sock_id, beginCode, sizeof(beginCode));
      char buffer[MESSAGE_BUFFER_SIZE];
      sprintf(buffer, "<Player %d> -- ", i);
      
      while (read( sock_id, buffer, sizeof(buffer) )) {
	printf("Received from player %d: %s\n", i, buffer);
	//put stuff into shm
	semdown(player.sem_id);
	char * shm = (char *) shmat(player.shm_id, 0, 0);
	//messages are seperated by newline
	
	//	printf("Current shm: [%s]\n", shm);
	//	printf("adding buffer: [%s]\n", buffer);
      	shm = strcat(shm, buffer);
	
	//	printf("updated shm: %s\n", shm);
	shmdt(shm);
	semup(player.sem_id);
      }
    }
  }
  
  //main server check shared memory in a loop
  //when one person types msg, sends to everyone
  while (1) {
    //i declared above
    for (i = 0; i < current_players; i++) {
      struct sockpair player = players[i];
      semdown(player.sem_id);
      char * shm = (char *) shmat(player.shm_id, 0, 0);
      //      printf("reading shm: [%s]\n", shm);
      if ( strlen(shm) ) { //if shm not empty
	sendAll(shm);
	char emptyStr[] = "";
	shm = strcpy(shm, emptyStr);
      }
      shmdt(shm);
      semup(player.sem_id);
    }
    sleep(1);
  }
  
  return 0;
}
