#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include "networking.h"
#include "memctl.h"

#define MAX_PLAYERS 8
#define PLAYERCOUNT 8
struct sockpair {
  int sock_id;
  int shm_id;
  int sem_id;
};

void sendAll( char * message );
void sendTo( int playerID, char * message );
void serverSend( char * message );
void serverAll( char * message );
static void sighandler(int signo);
int nameToID(char * name, char * names[]);
char * IDToName(int id, char * names[]);
int randInt();

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

void serverAll(char * message){
  char * msg = (char *) malloc( MESSAGE_BUFFER_SIZE );
  memset(msg, 0, MESSAGE_BUFFER_SIZE);
  sprintf(msg, "[SERVER] %s", message);
  sendAll(msg);
  free(msg);
}


//helper to send a message to specific player
void sendTo(int playerID, char * message) {
  struct sockpair player = players[playerID];
  char *p = strchr(message, '\0');
  *p = '|';
  *(p+1) = '\0'; //risky bois
  //printf("sending to [%d]: \"%s\"\n", playerID, message); 
  write(player.sock_id, message, strlen(message));
  *p = '\0';
}

void serverTo(int playerID, char * message){
  char * msg = (char *) malloc( MESSAGE_BUFFER_SIZE );
  sprintf(msg, "[SERVER] %s", message);
  sendTo(playerID, msg);
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
    printf("Error: %d\n", signo);
    exit(0);
  }
}


int randInt(){
  
  int fd = open("/dev/random", O_RDONLY);
  if (fd == -1) printf("RandDev File opened incorrectly.");
  
  int randomBytes;

  int err = read(fd, &randomBytes, 4);
  if (err == -1) printf("Random data read incorrectly.");
  
  err = close(fd);
  if (err == -1) printf("RandDev File closed incorrectly.");

  return randomBytes;  

}

int nameToID(char * name, char * names[]){
  int k = 0;
  for (k = 0; k < PLAYERCOUNT; k++){
    if (!strcmp(name, names[k]))
      return k;
  }
  return -1;
}
  
char * IDToName(int id, char * names[]){
  return names[id];
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
    //    printf("current players: %d\n", current_players);
    client_conn = server_connect( sd );
    
    struct sockpair sp;
    sp.sock_id = client_conn;

    sp.shm_id = setupShm();
    sp.sem_id = setupSem();
    
    players[current_players] = sp;
    current_players++;
    serverAll("A player has joined the game!");
    printf("[SERVER] Number of players in game: %d\n", current_players);
  }
 
  
  printf("[SERVER] Enough players, game beginning.\n");

  serverAll("Welcome to Mafia!");
  char * server_msg =(char *)malloc(MESSAGE_BUFFER_SIZE);
  memset(server_msg, 0, MESSAGE_BUFFER_SIZE);
  close(sd);
  
  int i;

  for (i = 0; i < current_players; i++){
    struct sockpair player = players[i];
    int sock_id = player.sock_id;
    char beginCode[] = "***BEGIN***|"; //and need that '|'
    write(sock_id, beginCode, sizeof(beginCode)); //COMMENCE!!!

    
    int f = fork();
    
    if (f == 0){ //SUBSERVER
      char buffer[MESSAGE_BUFFER_SIZE];
      
      while (read( sock_id, buffer, MESSAGE_BUFFER_SIZE )) {

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

  } //END SUBSERVER

  //EARLY GAME
  //get a set timer function in here
  int playerCount = PLAYERCOUNT;
  int roles[PLAYERCOUNT];
  int dead[PLAYERCOUNT]; //0 is alive, 1 is dead
  int n;
  char * names[PLAYERCOUNT]; //idth index is name
  for (n = 0; n < PLAYERCOUNT; n++){
    names[n] = (char *)malloc(MESSAGE_BUFFER_SIZE);
  }
  
  int nameCheck[PLAYERCOUNT];
  memset(nameCheck, 0, sizeof(nameCheck));
  
  int nameFlag = 0;
  
  serverAll("What is your name?");
  
  while (!nameFlag){
    nameFlag = 1;
    for (i = 0; i < current_players; i++){
      //      printf("Checking namecheck for %d, %d\n", i, nameCheck[i]);
      if (!nameCheck[i]){
        
	nameFlag = 0;
	struct sockpair player = players[i];
	semdown(player.sem_id);

	char * shm = (char *) shmat(player.shm_id, 0, 0);

	if ( strlen(shm) ) { //if shm not empty
	  strcpy(names[i], shm);	  
	  sprintf(server_msg, "Welcome %s.", names[i]);
	  serverTo(i, server_msg);
	  nameCheck[i] = 1;
	  char emptyStr[] = "";
	  shm = strcpy(shm, emptyStr);
	}
	
	shmdt(shm);
	semup(player.sem_id);
      }
    }
    nanosleep((const struct timespec[]){{0, 100000000L}}, NULL);
  }
  
  serverAll("Now assigning roles.");
  //distribute 1-playercount among 1-8
  for (n = 0; n < current_players; n++){//initialize
    roles[n] = n;
  }

  //Fisher-Yates
for (n = PLAYERCOUNT - 1; n >= 0; --n){
    int rand = abs(randInt()) % (n + 1);
    int temp = roles[n];
    roles[n] = roles[rand];
    roles[rand] = temp;
}
  
  sprintf(server_msg, "You are in the mafia! Your partner is %s. Survive!", IDToName(roles[1], names));
  serverTo(roles[0], server_msg);
  sprintf(server_msg, "You are in the mafia! Your partner is %s. Survive!", IDToName(roles[0], names));
  serverTo(roles[1], server_msg);
  serverTo(roles[2], "You are the cop! Find out who the mafia are.");
  for (n = 3; n < PLAYERCOUNT; n++){
    serverTo(roles[n], "You are a townsperson! Find out who the mafia are.");
  }

  
  //  roles[0] = 1 means player 1 has role 0, which is mafia. index is the player id. 

  //MIDGAME
  int isAlive[PLAYERCOUNT];
  int numAlive = current_players;
  for (n = 0; n < PLAYERCOUNT; n++){
    isAlive[n] = 1;
  }
  
#define	DAYPREP 10
#define VOTEPREP 11
#define NIGHTPREP 12
#define MAFPREP 13
#define COPPREP 14
#define DAY 15
#define VOTE 16
#define NIGHT 17
#define MAF 18
#define COP 19
  
  //GAME VARS
  //day/night cycle
  int dayCtr = 0;
  int phase = DAYPREP;

  
  int c1;
  int c2;
  int success;
  int timeStart;
  int daytimeRemaining;
  int nighttimeRemaining;
  int * playerNoms;
  int yesVotes;
  int noVotes;
  int curTime;
  int choice;
  char * msg;
  char msgs[PLAYERCOUNT][256]; //8 thing array
  int votes[2]; //yes no votes
  int newNom;
  int timeElapsed;
  int voted[PLAYERCOUNT];
  
  msg = (char *)malloc(MESSAGE_BUFFER_SIZE);
  memset(msg, 0, MESSAGE_BUFFER_SIZE);
  
  //main server check shared memory in a loop
  //when one person types msg, sends to everyone
  while (1) {
    
    //update msgs
    for (i = 0; i < current_players; i++) {
      struct sockpair player = players[i];
      semdown(player.sem_id);
      char * shm = (char *) shmat(player.shm_id, 0, 0);
      //printf("Reading shm: [%s]\n", shm);

      if ( strlen(shm) ) { //if shm not empty
	printf("i is currently %d\n", i);
	if (!strlen(msgs[i])){
	  strcpy(msgs[i], shm);
	  memset(shm, 0, MESSAGE_BUFFER_SIZE);
	  printf("verify msgs %s\n", msgs[i]);
	  //sendAll(shm);
	  //	  char emptyStr[] = "";
	  //	  shm = strcpy(shm, emptyStr);
	}
      }
      shmdt(shm);
      semup(player.sem_id);
    }

    nanosleep((const struct timespec[]){{0, 100000000L}}, NULL);
    
    //MAIN GAME: PREP 
    switch (phase){

    case DAYPREP:
      dayCtr++;
      timeStart = time(NULL);
      playerNoms = (int *)calloc(PLAYERCOUNT, sizeof(int));
      daytimeRemaining = 60;
      timeElapsed = 0;
      sprintf(server_msg, "It is currently day %d", dayCtr);
      serverAll(server_msg);
      serverAll("Discussion begins.");
      
      //clear nighttime chat logs...
      for (n = 0; n < current_players; n++){
	memset(msgs[n], 0, MESSAGE_BUFFER_SIZE);
      }
      
      phase = DAY;
      break;

    case VOTEPREP:
      memset(votes, 0, 8);
      memset(playerNoms, 0, PLAYERCOUNT * 4);
      timeStart = time(NULL);
      serverAll("Vote begins. Minimum 4 votes to execute. 30 second timer.");
      phase = VOTE;
      for (n = 0; n < PLAYERCOUNT; n++){
	voted[n] = 0;
      }
      break;

    case NIGHTPREP:
      sprintf(server_msg, "It is currently night %d", dayCtr);
      serverAll(server_msg);
      phase = NIGHT;
      break;

    case MAFPREP:
      
      //mafia prompt (maybe write a sendMafia, something)
      if (isAlive[roles[0]])
	serverTo(roles[0], "Wake up, mafia. Pick a person to kill.");
      if (isAlive[roles[1]])
	serverTo(roles[1], "Wake up, mafia. Pick a person to kill.");
      phase = MAF;
      success = 0;
      c1 = -1;
      c2 = -1;
      break;

    case COPPREP:
      serverAll("The cop is now investigating a suspect...");
      serverTo(roles[2], "Wake up, cop. Pick a person to investigate.");
      phase = COP;
      success = 0;
      break;
      
    case DAY:;
      
      timeElapsed = (time(NULL) - timeStart);
	
      if ((daytimeRemaining - timeElapsed) % 5 == 0) {
	sprintf(server_msg, "Daytime remaining: %d", daytimeRemaining - timeElapsed);
	serverAll(server_msg);
      }
      //DAY ENDS
      if (timeElapsed >= daytimeRemaining) {
	serverAll("Daytime has ended. Go to sleep.");
	phase = NIGHTPREP;
	free(playerNoms);
      }
      
      
      //let's parse that chat shall we.
      for (n = 0; n < PLAYERCOUNT; n++){
	//	printf("%s is alive?: <%d>\n", IDToName(n, names), isAlive[n]);
	if (!isAlive[n] || !strlen(msgs[n])) continue;

	//printf("N is currently %d\n", n);
	//printf("msgs[0] is %s\n", msgs[0]);
	//printf("msgs[1] is %s\n", msgs[1]);
	//printf("msgs[2] is %s\n", msgs[2]);

	//printf("and msg is %s\n", msg);
	//char empty[] = "";
	strcpy(msg, msgs[n]);
	//strcpy(msg, empty);
	memset(msgs[n], 0, MESSAGE_BUFFER_SIZE);
	printf("for player %d\n", n);
	printf("message is: %s\n", msg);
	printf("message[0] is %c\n", msg[0]);
	
	if (msg[0] == '\\'){

	  char * cmd;
	  char * cpy = (char *)malloc(MESSAGE_BUFFER_SIZE);
	  char * cpyAnchor = cpy;
	  strcpy(cpy, msg);
	  cmd = strsep(&cpy, " ");

	  /*
	    if (!strcmp(cmd, "\\h")) {
	    
	    }

	   */

	  
	  if (!strcmp(cmd, "\\w")){ //whispering
	    char * to = strsep(&cpy, " ");
	    int actualTo = nameToID(to, names);
	    if (actualTo == -1 || actualTo == n){
	      serverTo(n, "Invalid name.");
	    }
	    else {
	      sprintf(server_msg, "%s is whispering to %s", IDToName(n, names), IDToName(actualTo, names));
	      serverAll(server_msg);
	      sprintf(server_msg, "[%s][whisper][%s] %s", IDToName(n, names), IDToName(actualTo, names), cpy);
	      sendTo(actualTo, server_msg);
	      sendTo(n, server_msg);
	    }
	  }
	  
	  else if (!strcmp(cmd, "\\nom")){ //nomination

	    int newNom = nameToID(cpy, names);
	    if (newNom == n){
	      serverTo(n, "You cannot nominate yourself!");
	    }
	    else if (newNom == -1){
	      serverTo(n, "Invalid nomination.");
	    }
	    else {
	      sprintf(server_msg,"%s has been nominated by %s.", IDToName(newNom, names), IDToName(n, names));
	      serverAll(server_msg);
	      *(playerNoms + newNom) += 1;
	      if (* (playerNoms + newNom) == 3){
		//vote triggered
		sprintf(server_msg, "%s has been accused! Should they be executed? (yes/no)", IDToName(newNom, names));
		serverAll(server_msg);
		phase = VOTEPREP;
		
		daytimeRemaining -= timeElapsed;
	      }
	    }
	  }
	  
	  else {
	    serverTo(n, "Invalid command.");
	  }

	  free(cpyAnchor);
	  
	}
	else {//completely normal chat string
	  sprintf(server_msg,"[%s] \t %s", IDToName(n, names), msg);
	  printf("message is %s\n", server_msg);
	  sendAll(server_msg);
	}
	
      }
      
      break;
      
    case VOTE:
      

      for (n = 0; n < PLAYERCOUNT; n++){
	if (n == newNom || !isAlive[n] || voted[n]) continue;  
	
	strcpy(msg, msgs[n]);
	if (strlen(msg) == 0){
	  continue;
	}
	if (!strcmp(msg, "y") || !strcmp(msg, "yes")){
	  sprintf(server_msg,"%s has voted for execution.", IDToName(n, names));
	  serverAll(server_msg);
	  votes[0]++;
	  voted[n] = 1;
	}
	else if (!strcmp(msg, "n") || !strcmp(msg, "no")){
	  
	  sprintf(server_msg, "%s has voted against execution", IDToName(n, names));
	  serverAll(server_msg);
	  votes[1]++;
	  voted[n] = 1;
	}
	  
      }

      curTime = time(NULL);
      timeElapsed = curTime - timeStart;
	
      if ((30 - timeElapsed) % 5 == 0) {
	sprintf(server_msg, "Vote time remaining: %d", 30 - timeElapsed);
	serverAll(server_msg);
      }
	
      if (timeElapsed >= 30){
	phase = DAY;
	timeStart = curTime;
	  
	if (votes[0] < votes[1]){
	  serverAll("The verdict is innocent. The accused lives.");
	}
	else if (votes[0] == votes[1]){
	  serverAll("Tied vote. The accused lives.");
	}
	else if (votes[0] < 4){
	  serverAll("Not enough votes. The accused lives.");
	}
	else {
	  serverAll("The verdict is guilty. The accused shall be executed.");
	  printf("person who died: %s\n", IDToName(newNom, names));
	  isAlive[newNom] = 0;
	  numAlive -= 1;
	}

      }

      break;

    case NIGHT: //oops
      phase = MAFPREP;
      break;

    case MAF:; //one must be alive or game would be over

      int c;
      if (!success){

	//CASE 1 ALIVE
	if (!isAlive[roles[1]]){
	  msg = msgs[roles[0]];
	  if (strlen(msg)){
	    c = nameToID(msg, names);
	    memset(msgs[roles[0]], 0, MESSAGE_BUFFER_SIZE);
	    if (!isAlive[c] || c == -1 || c == roles[0]) {
	      serverTo(roles[0], "Invalid name. CASE 1 ALIVE IF");
	    }
	    else success = 1;
	  }	
	}
      
	else if (!isAlive[roles[0]]){
	  msg = msgs[roles[1]];
	  if (strlen(msg)){
	    c = nameToID(msg, names);
	    memset(msgs[roles[1]], 0, MESSAGE_BUFFER_SIZE);
	    if (!isAlive[c] || c == -1 || c == roles[1]) {
	      serverTo(roles[1], "Invalid name. CASE 1 ALIVE ELSEIF");
	    }
	    else success = 1;
	  }	
	}

	//CASE 2 ALIVE
	else {	  
	  strcpy(msg, msgs[roles[0]]);
	  memset(msgs[roles[0]], 0, MESSAGE_BUFFER_SIZE);
	  if (strlen(msg)){

	    char * cmd;
	    char * cpy = (char *)malloc(MESSAGE_BUFFER_SIZE);
	    char * cpyAnchor = cpy;
	    strcpy(cpy, msg);
	    cmd = strsep(&cpy, " ");
	    if (!strcmp(cmd, "\\c")){
	      c1 = nameToID(cpy, names);
	      memset(msgs[roles[0]], 0, MESSAGE_BUFFER_SIZE);
	      printf("c1: %d\n", c1);
	      if (!isAlive[c1] || c1 == -1 || c1 == roles[0] || c1 == roles[1]) {
		serverTo(roles[0], "Invalid name ELSE 1");
		c1 = -1;
	      }
	      else {
		sprintf(server_msg, "You have chosen to target %s", IDToName(c1, names));
		serverTo(roles[0], server_msg);
		sprintf(server_msg, "Your partner has chosen to target %s", IDToName(c1, names));
		serverTo(roles[1], server_msg);
	      }
	    }
	    else {
	      sprintf(server_msg,"[%s] \t %s", IDToName(roles[0], names), msg);
	      sendTo(roles[0], server_msg);
	      sendTo(roles[1], server_msg);//it's chat
	    }
	    
	    free(cpyAnchor);	    
	  }
 
	  strcpy(msg, msgs[roles[1]]);
	  memset(msgs[roles[1]], 0, MESSAGE_BUFFER_SIZE);
	  if (strlen(msg)){
	    char * cmd;
	    char * cpy = (char *)malloc(MESSAGE_BUFFER_SIZE);
	    char * cpyAnchor = cpy;
	    strcpy(cpy, msg);
	    cmd = strsep(&cpy, " ");
	    if (!strcmp(cmd, "\\c")){
	      printf("Yeah\n");
	      c2 = nameToID(cpy, names);
	      memset(msgs[roles[1]], 0, MESSAGE_BUFFER_SIZE);
	      if (!isAlive[c2] || c2 == -1 || c2 == roles[0] || c2 == roles[1]) {
		serverTo(roles[1], "Invalid name ELSE 2");
		c2 = -1;
	      }
	      else {
		sprintf(server_msg, "You have chosen to target %s", IDToName(c2, names));
		serverTo(roles[1], server_msg);
		sprintf(server_msg, "Your partner has chosen to target %s", IDToName(c2, names));
		serverTo(roles[0], server_msg);
	      }
	    }
	    
	    else {
	      sprintf(server_msg,"[%s] \t %s", IDToName(roles[1], names), msg);
	      sendTo(roles[0], server_msg);
	      sendTo(roles[1], server_msg);//it's chat
	    }

	    
	    free(cpyAnchor);
	  }
	  if (c1 == -1 || c2 == -1){
	    serverTo(roles[0], "Waiting for selection.");
	    serverTo(roles[1], "Waiting for selection.");
	    sleep(5);
	  }
	  else {        
	    if (c1 != c2){
	      serverTo(roles[0], "You must agree on the target!");
	      serverTo(roles[1], "You must agree on the target!");
	    }
	    else {
	      success = 1;
	      c= c1;
	    }  
	  }
	}
	//that ends
      }
      //we're successful
      else {
	
	if (isAlive[roles[0]]){
	  sprintf(server_msg, "You have chosen to kill %s. Go to sleep.", IDToName(c, names));
	  serverTo(roles[0], server_msg);
	}
	
	if (isAlive[roles[1]]){       
	  sprintf(server_msg, "You have chosen to kill %s. Go to sleep.", IDToName(c, names));
	  serverTo(roles[1], server_msg);
	}

	isAlive[c] = 0; //ooh killem
	
	numAlive -= 1;


	sprintf(server_msg, "%s has been killed by the mafia.", IDToName(c, names));
	serverAll(server_msg);
	if (c == roles[2]) {
	  sprintf(server_msg, "%s was a cop.", IDToName(c, names));
	} else {
	  sprintf(server_msg, "%s was a townsman.", IDToName(c, names));
	}
	serverAll(server_msg);
	
	if (isAlive[roles[2]])
	  phase = COPPREP;
	else
	  phase = DAYPREP;
	  
      }	
    break;
      
  case COP:

    msg = msgs[roles[2]]; 

    if (strlen(msg)){
      int copChoice = nameToID(msg, names);
      memset(msgs[roles[2]], 0, MESSAGE_BUFFER_SIZE);
	
      if (!isAlive[copChoice] || copChoice == -1 || copChoice == roles[2]){
	serverTo(roles[2], "Invalid name");
      }
      
      else{
	if ( copChoice == roles[0] || copChoice == roles[1] ){
	  serverTo(roles[2], "This person is a member of the mafia.");
	}
	else {
	  serverTo(roles[2], "This person is an innocent townsperson.");
	}
	phase = DAYPREP;
      }
    }
    
    break;
      
  }
    
  //check for endgame
  
  //end game
  //(exited a while loop - if sum of alive mafia members > sum of townspeople)
  if (isAlive[0] + isAlive[1] == 0){
  sendAll("Game over. The townspeople have won!");
  exit(0);
  }
  else if (isAlive[0] + isAlive[1] > numAlive / 2){
  sendAll("Game over. (Defaulted) The mafia outnumber the townspeople, and have won!");
  exit(0);

  }   
  
}

  
return 0;
}
