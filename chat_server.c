#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include "networking.h"
#include "memctl.h"

#define MAX_PLAYERS 1
#define PLAYERCOUNT 1
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
  sprintf(msg, "[SERVER] %s", message);
  sendAll(msg);
  free(msg);
}


//helper to send a message to specific player
void sendTo(int playerID, char * message) {
  struct sockpair player = players[playerID];
  printf("sending to [%d]: \"%s\"\n", playerID, message); 
  write(player.sock_id, message, strlen(message));
  
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
  int n = 0;
  for (n = 0; n < PLAYERCOUNT; n++){
    if (!strcmp(name, names[n]))
      return n;
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
    printf("current players: %d\n", current_players);
    client_conn = server_connect( sd );
    
    struct sockpair sp;
    sp.sock_id = client_conn;

    sp.shm_id = setupShm();
    sp.sem_id = setupSem();
    
    players[current_players] = sp;
    current_players++;
    serverAll("A player has joined the game!\n");
    printf("[SERVER] number of players in game: %d\n", current_players);
  }
 
  
  printf("[SERVER] enough players, game beginning\n");

  serverAll("Welcome to Mafia!");
  char * server_msg;
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
  int nameCheck[PLAYERCOUNT];
  memset(nameCheck, 0, sizeof(nameCheck));
  int nameFlag;
  serverAll("What is your name?");
  
  while (1){
    //    printf("Checking 0\n");
    nameFlag = 1;
    for (i = 0; i < current_players; i++){
      printf("Checking namecheck %d\n", nameCheck[i]);
      if (!nameCheck[i]){
	printf("1\n");
	nameFlag = 0;
	struct sockpair player = players[i];
	semdown(player.sem_id);
	printf("2\n");
	//	printf("Checking 1\n");
	char * shm = (char *) shmat(player.shm_id, 0, 0);
	//	printf("Checking 3\n");
	if ( strlen(shm) ) { //if shm not empty
	  //	  printf("Checking -1\n");
	  strcpy(names[i], shm);     
	  sprintf(server_msg, "Welcome, %s.", names[i]);
	  serverAll(server_msg);
	  char emptyStr[] = "";
	  shm = strcpy(shm, emptyStr);
	}
	//	printf("Checking 2\n");
	shmdt(shm);
	semup(player.sem_id);
      }
      
      sleep(1);
    }
    if (nameFlag == 1) break;
  }

  
  serverAll("Now assigning roles");
  //distribute 1-playercount among 1-8
  for (n = 0; n < current_players; n++){//initialize
    roles[n] = n;
  }
  printf("done\n");
  //grab a random player from the list...
  for (n = 0; n < 5; n++){//let's shake it up
    //swap rand.
    printf("trying to randomize\n");
    int first = abs(randInt()) % current_players;
    printf("first index %d", first);
    int second = abs(randInt()) % current_players;
    printf("second index %d", second);
    int temp = roles[first];
    roles[first] = roles[second];
    roles[second] = temp;
    printf("first is now %d", roles[first]);
    printf("second is now %d", roles[second]);
  }
  printf("done2\n");
  printf("roles[0] is %d\n", roles[0]);
  
  printf("roles[1] is %d\n", roles[1]);
  
  printf("roles[2] is %d\n", roles[2]);
  serverAll("randomized");
  sprintf(server_msg, "You are in the mafia! Your partner is %s. Survive!\n", IDToName(roles[1], names));
  printf("was sprinting the error\n");
  serverTo(roles[0], server_msg);
  sprintf(server_msg, "You are in the mafia! Your partner is %s. Survive!\n", IDToName(roles[0], names));
  serverTo(roles[1], server_msg);
  serverTo(roles[2], "You are the cop! Find out who the mafia are.\n");
  for (n = 3; n < PLAYERCOUNT; n++){
    serverTo(roles[n], "You are a townsperson! Find out who the mafia are.\n");
  }

  //  roles[0] = 1 means player 1 has role 0, which is mafia. index is the player id. 

  //MIDGAME
  int isAlive[PLAYERCOUNT];
  int numAlive = current_players;

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

  
  //main server check shared memory in a loop
  //when one person types msg, sends to everyone
  while (1) {
    
    //update msgs
    for (i = 0; i < current_players; i++) {
      struct sockpair player = players[i];
      semdown(player.sem_id);
      char * shm = (char *) shmat(player.shm_id, 0, 0);
      //      printf("reading shm: [%s]\n", shm);
      if ( strlen(shm) ) { //if shm not empty

	//parse the crap outta it RIGHT HER
	if (!strlen(msgs[i])){
	  strcpy(msgs[i], shm);
       
	  //sendAll(shm);
	  char emptyStr[] = "";
	  shm = strcpy(shm, emptyStr);
	}
      }
      shmdt(shm);
      semup(player.sem_id);
    }

    sleep(1);
    
    //MAIN GAME: PREP 
    switch (phase){

    case DAYPREP:
      dayCtr++;
      timeStart = time(NULL);
      playerNoms = (int *)calloc(n, sizeof(int));
      daytimeRemaining = 30;
      
      sprintf(server_msg, "It is currently day %d\n", dayCtr);
      serverAll(server_msg);
      serverAll("Discussion begins.\n");

      //clear nighttime chat logs...
      for (n = 0; n < current_players; n++){
	char emptyStr[] = "";
	strcpy(msgs[n], emptyStr);
      }
      
      phase = DAY;
      break;

    case VOTEPREP:
      memset(votes, 0, 8);
      timeStart = time(NULL);
      serverAll("Vote begins. Minimum 4 votes to execute. 30 second timer.\n");
      phase = VOTE;
      break;

    case NIGHTPREP:
      sprintf(server_msg, "It is currently night %d\n", dayCtr);
      serverAll(server_msg);
      phase = NIGHT;
      break;

    case MAFPREP:
      
      //mafia prompt (maybe write a sendMafia, something)
      if (isAlive[roles[0]])
	serverTo(roles[0], "Wake up, mafia. Pick a person to kill.\n");
      if (isAlive[roles[1]])
	serverTo(roles[1], "Wake up, mafia. Pick a person to kill.\n");
      phase = MAF;
      break;

    case COPPREP:
      serverTo(roles[2], "Wake up, cop. Pick a person to investigate.\n");
      phase = COP;
      break;
      
    case DAY:;
      
      int timeElapsed = (time(NULL) - timeStart);
	
      if ((daytimeRemaining - timeElapsed) % 5 == 0) {
	sprintf(server_msg, "Daytime remaining: %d", daytimeRemaining);
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
	if (!isAlive[n]) continue;
	
	strcpy(msg, msgs[n]);
	char emptyStr[] = "";
	strcpy(msgs[n], emptyStr);
	
	if (msg[0] == '\\'){//is actually just a \ indicating command
	  //find command
	  char * cmd;
	  cmd = strsep(&msg, " ");
	  if (!strcmp(cmd, "\\w")){
	    //nice
	    char * to = strsep(&msg, " ");
	    int actualTo = nameToID(to, names);
	    if (actualTo == -1 || actualTo == n){
	      serverTo(n, "Invalid name.");
	    }
	    else {
	      sprintf(server_msg, "%s is whipsering to %s", IDToName(n, names), IDToName(actualTo, names));
	      serverAll(server_msg);
	      sprintf(server_msg, "[%s] %s", IDToName(n, names), msg);
	      sendTo(actualTo, server_msg);
	    }
	  }
	  else if (!strcmp(cmd, "\\nom")){
	    //nicer
	    //token is new nom;
	    int newNom = nameToID(msg, names);
	    if (newNom == n){
	      serverTo(n, "You cannot nominate yourself!");
	    }
	    else if (newNom == -1){
	      serverTo(n, "Invalid nomination");
	    }
	    else {
	      sprintf(server_msg,"%s has been nominated by %s.", IDToName(newNom, names), IDToName(n, names));
	      serverAll(server_msg);
	      *(playerNoms + newNom) += 1;
	      if (* (playerNoms + newNom) == 3){
		//vote triggered
		sprintf(server_msg, "%s has been accused! Should they be executed? (yes/no)\n", IDToName(newNom, names));
		serverAll(server_msg);	    
		phase = VOTEPREP;
		
		daytimeRemaining -= timeElapsed;
	      }
	    }
	  }
	  else {//completely normal chat string
	    sprintf(server_msg,"[%s] \t %s", IDToName(n, names), msg);
	    sendAll(server_msg); //needs to be processed
	  }
	}
      
	timeStart = time(NULL);
      }
      
      break;
      
    case VOTE:

      for (n = 0; n < PLAYERCOUNT; n++){
	if (n == newNom || !isAlive[n]) continue;  

	strcpy(msg, msgs[n]);
	if (strlen(msg) == 0){
	  continue;
	}
	if (!strcmp(msg, "y")){
	  sprintf(server_msg,"%s has voted for execution.", IDToName(n, names));
	  serverAll(server_msg);
	  votes[0]++;
	}
	else if (!strcmp(msg, "n")){
	  
	  sprintf(server_msg, "%s has voted against execution", IDToName(n, names));
	  serverAll(server_msg);
	  votes[1]++;
	}
	  
      }
      
      curTime = time(NULL);
	

	
      if (curTime - timeStart >= 30){
	phase = 1;
	timeStart = curTime;
	  
	if (votes[0] < votes[1]){
	  serverAll("The verdict is innocent. The accused lives.\n");
	}
	else if (votes[0] == votes[1]){
	  serverAll("Tied vote. The accused lives.\n");
	}
	else if (votes[0] < 4){
	  serverAll("Not enough votes. The accused lives.\n");
	}
	else {
	  serverAll("The verdict is guilty. The accused shall be executed.\n");
	  isAlive[newNom] = 0;
	}

      }

      break;

    case NIGHT: //oops
      phase = MAFPREP;
      break;

    case MAF:; //one must be alive or game would be over
      
      int c;
      
      if (!isAlive[roles[1]]){
	while (1){
	  msg = msgs[roles[0]];
	  c = nameToID(msg, names);
	  char emptyStr[] = "";
	  strcpy(msgs[roles[0]], emptyStr);
	  if (!isAlive[c] || c == -1 || c == roles[0]) {
	    serverTo(roles[0], "Invalid name");
	  }
	  else break;
	}	
      }
      
      else if (!isAlive[roles[0]]){
	while (1){
	  msg = msgs[roles[1]];
	  c = nameToID(msg, names);
	  char emptyStr[] = "";
	  strcpy(msgs[roles[1]], emptyStr);
	  if (!isAlive[c] || c == -1 || c == roles[1]) {
	    serverTo(roles[1], "Invalid name");
	  }
	  else break;
	}	
      }
      
      else {
	
	int c1;
	int c2;
	
	while (1){
	  msg = msgs[roles[0]];
	  c1 = nameToID(msg, names);
	  msg = msgs[roles[1]];
	  c2 = nameToID(msg, names);
	  char emptyStr[] = "";
	  strcpy(msgs[roles[0]], emptyStr);
	  strcpy(msgs[roles[1]], emptyStr);
	
	  int validFlag = 1;
	  if (!isAlive[c1] || c1 == -1 || c1 == roles[0] || c1 == roles[1]) {
	    validFlag = 0;
	    serverTo(roles[0], "Invalid name");
	  }
	  
	  if (!isAlive[c2] || c2 == -1 || c2 == roles[0] || c2 == roles[1]) {
	    validFlag = 0;
	    serverTo(roles[1], "Invalid name");
	  }
	  
	  if (validFlag){
	  
	    sprintf(server_msg, "Your have chosen to target %s", IDToName(c1, names));
	    serverTo(roles[0], server_msg);
	    sprintf(server_msg, "Your partner has chosen to target %s", IDToName(c1, names));
	    serverTo(roles[0], server_msg);
	    sprintf(server_msg, "You have chosen to target %s", IDToName(c2, names));
	    serverTo(roles[1], server_msg);
	    sprintf(server_msg, "Your partner has chosen to target %s", IDToName(c1, names));
	    serverTo(roles[1], server_msg);
	         
	    if (c1 != c2){
	      serverTo(roles[0], "You must agree on the target!\n");
	      serverTo(roles[1], "You must agree on the target!\n");
	    }
	    else break;
	    
	  }
	  
	  c = c1;

	}
      }
	
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
     
      if (isAlive[roles[2]])
	phase = COPPREP;
      else
	phase = DAYPREP;
	
      break;
      
    case COP:

      while (1){
	
	msg = msgs[roles[2]]; 
	int copChoice = nameToID(msg, names);
	char emptyStr[] = "";
	strcpy(msgs[roles[2]], emptyStr);

	if (!isAlive[copChoice] || copChoice == -1 || copChoice == roles[2]){
	  serverTo(roles[2], "Invalid name");
	}
	else{
	  if ( copChoice == roles[0] || copChoice == roles[1] ){
	    serverTo(roles[2], "This person is a member of the mafia.\n");
	  }
	  else {
	    serverTo(roles[2], "This person is an innocent townsperson.\n");
	  }
	  break;
	}
      }
      
      phase = DAYPREP;
      break;
      
    }
    
    //check for endgame

    //end game
    //(exited a while loop - if sum of alive mafia members > sum of townspeople)
    if (isAlive[0] + isAlive[1] == 0){
      sendAll("Game over. The townspeople have won!\n");
    }
    else if (isAlive[0] + isAlive[1] > numAlive / 2){
      sendAll("Game over. (Defaulted) The mafia outnumber the townspeople, and have won!\n");
    }   

  }

  return 0;
}
