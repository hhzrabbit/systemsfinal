//NEED TO FREE MEMORY REMIND ME

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
#include "networking.h"
#include "memctl.h"

#define MAX_PLAYERS 3
#define PLAYERCOUNT 3
struct sockpair {
  int sock_id;
  int shm_id;
  int sem_id;
};

void sendAll( char * message );
void sendTo( int playerID, char * message );
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

//helper to send a message to specific player
void sendTo(int playerID, char * message) {
  struct sockpair player = players[playerID];
  char * header = (char *)malloc(MESSAGE_BUFFER_SIZE);
  sprintf(header, "<Player %d> ", playerID);
  message = strcat(header, message);
  printf("sending to [%d]: \"%s\"\n", playerID, message);
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
    printf("%s\n", "did an error");
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
/*
//3 votes triggers execution panel
int nomineeListen(){
char * nominated = receiveVote(); //gets received vote. receiveVote checks alive or dead...whether to approve or veto.
if (nominated != NULL) {//turn the name to an id.
//strcasestr(nominated, "vote");//string is sanitized in receiveVote();
//s = strsep(&s, "\n")
return nameToID(nominated);
}
return -1;
}
*/
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
    client_conn = server_connect( sd );
    
    struct sockpair sp;
    sp.sock_id = client_conn;

    sp.shm_id = setupShm();
    sp.sem_id = setupSem();

    players[current_players] = sp;
    current_players++;
    sendAll("A player has joined the game!\n");
    printf("[SERVER] number of players in game: %d\n", current_players);
  }
  printf("[SERVER] enough players, game beginning\n");

  sendAll("Welcome to Mafia!");
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
  memset(nameCheck, 0, 32);
  int nameFlag;
  sendAll("What are your names?");
  
  while (1){
    printf("Checking 0\n");
    nameFlag = 1;
    for (n = 0; n < current_players; n++){
      printf("Checking namecheck %d\n", nameCheck[n]);
      if (!nameCheck[n]){
	nameFlag = 0;
	struct sockpair player = players[i];
	semdown(player.sem_id);
	printf("Checking 1\n");
	char * shm = (char *) shmat(player.shm_id, 0, 0);
	printf("Checking 3\n");
	if ( strlen(shm) ) { //if shm not empty
	  printf("Checking -1\n");
	  strcpy(names[n], shm);     
	  sprintf(server_msg, "Welcome, %s.", names[n]);
	  sendAll(server_msg);
	  char emptyStr[] = "";
	  shm = strcpy(shm, emptyStr);
	}
	printf("Checking 2\n");
	shmdt(shm);
	semup(player.sem_id);
      }
      
    sleep(1);
    }
    if (nameFlag == 1) break;
  }

  
  sendAll("Now assigning roles");
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
  sendAll("randomized");
  sprintf(server_msg, "You are in the mafia! Your partner is %s. Survive!\n", IDToName(roles[1], names));
  printf("was sprinting the error\n");
  sendTo(roles[0], server_msg);
  sprintf(server_msg, "You are in the mafia! Your partner is %s. Survive!\n", IDToName(roles[0], names));
  sendTo(roles[1], server_msg);
  sendTo(roles[2], "You are the cop! Find out who the mafia are.\n");
  for (n = 3; n < PLAYERCOUNT; n++){
    sendTo(roles[n], "You are a townsperson! Find out who the mafia are.\n");
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
	if (!strlen(msgs[i]){
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
      sendAll(server_msg);
      sendAll("Discussion begins.\n");
      
      phase = DAY;
      break;

    case VOTEPREP:
      
      phase = VOTE;
      break;

    case NIGHTPREP:
      sprintf(server_msg, "It is currently night %d\n", dayCtr);
      sendAll(server_msg);
      phase = NIGHT;
      break;

    case MAFPREP:
      
      //mafia prompt (maybe write a sendMafia, something)
      sendTo(roles[0], "Wake up, mafia. Pick a person to kill.\n");
      sendTo(roles[1], "Wake up, mafia. Pick a person to kill.\n");
      choice = -1;
      phase = MAF;
      break;

    case COPPREP:
      sendTo(roles[2], "Wake up, cop. Pick a person to investigate.\n");
      choice = -1;
      phase = COP;
      break;
      
    case DAY:;
      
      int timeElapsed = (time(NULL) - timeStart);
	
      if ((daytimeRemaining - timeElapsed) % 5 == 0) {
	sprintf(server_msg, "Daytime remaining: %d", daytimeRemaining);
	sendAll(server_msg);
      }
      //DAY ENDS
      if (timeElapsed >= daytimeRemaining) {
	sendAll("Daytime has ended. Go to sleep.");
	phase = NIGHTPREP;
	free(playerNoms);
      }

      //let's parse that chat shall we.
      for (n = 0; n < PLAYERCOUNT; n++){
	  
	strcpy(msg, msgs[n]);

	if (msg[0] = '\\'){//is actually just a \ indicating command
	  //find command
	  char * cmd;
	  cmd = strsep(&msg, " ");
	  if (!strcmp(cmd, "\\w")){
	    //nice
	  }
	  else if (!strcmp(cmd, "\\nom")){
	    //nicer
	    //token is new nom;
	    int newNom;
	    if (msg) newNom = nameToID(msg, names);
	    if (newNom != -1) {
	      sprintf(server_msg,"%s has been nominated.", IDToName(newNom, names));
	      sendAll(server_msg);
	      *(playerNoms + newNom) += 1;
	      if (* (playerNoms + newNom) == 3){
		//vote triggered
		sprintf(server_msg, "%s has been accused! Should they be executed? (yes/no)\n", IDToName(newNom, names));
		sendAll(server_msg);
		
		daytimeRemaining -= timeElapsed;
		timeStart = time(NULL);
		sendAll("Vote begins. Minimum 4 votes to execute. 30 seconds\n");
		yesVotes = 0;
		noVotes = 0;
		phase = 2;
		memset(votes, 0, 8);
	      }
	      else {
	      
	      }
	    }
	    else {//completely normal chat string
	      sendAll(msg); //needs to be processed
	    }
	  }
	}
      
	timeStart = time(NULL);
      }
      
      break;
      
    case VOTE:

      for (n = 0; n < PLAYERCOUNT; n++){
	  

	strcpy(msg, msgs[n]);
	if (strlen(msg) == 0){
	  continue;
	}
	if (!strcmp(msg, "y")){
	  sprintf(server_msg,"%s has voted for execution.", IDToName(n, names));
	  sendAll(server_msg);
	  votes[0]++;
	}
	else if (!strcmp(msg, "n")){
	  
	  sprintf(server_msg, "%s has voted against execution", IDToName(n, names));
	  sendAll(server_msg);
	  votes[1]++;
	}
	  
      }
      
      curTime = time(NULL);
	

	
      if (curTime - timeStart >= 30){
	phase = 1;
	timeStart = curTime;
	  
	if (votes[0] < votes[1]){
	  sendAll("The verdict is innocent. The accused lives.\n");
	}
	else if (votes[0] == votes[1]){
	  sendAll("Tied vote. The accused lives.\n");
	}
	else if (votes[0] < 4){
	  sendAll("Not enough votes. The accused lives.\n");
	}
	else {
	  sendAll("The verdict is guilty. The accused shall be executed.\n");
	  dead[newNom] = 1;
	}

      }

      
      break;

    case NIGHT: //oops
      phase = MAFPREP;
      break;

    case MAF:
      int c1;
      int c2;
      while (1){
	msg = msgs[roles[0]];
	c1 = nameToID(msg, names);
	msg = msgs[roles[1]];
	c2 = nameToID(msg, names);
	char emptyStr[] = "";
	strcpy(msg[roles[0]], emptyStr);
	strcpy(msg[roles[1]], emptyStr);
	
	int validFlag = 1;
	if (c1 == -1 || c1 == roles[0] || c1 == roles[1]) {
	  validFlag = 0;
	  sendTo(roles[0], "Invalid name");
	}
	if (c2 == -1 || c2 == roles[0] || c2 == roles[1]) {
	  validFlag = 0;
	  sendTo(roles[1], "Invalid name");
	}
	if (validFlag){
	  
	  sprintf(server_msg, "Your have chosen to target %s", idToName(c1));
	  sendTo(roles[0], server_msg);
	  sprintf(server_msg, "Your partner has chosen to target %s", idToName(c1));
	  sendTo(roles[0], server_msg);
	  sprintf(server_msg, "You have chosen to target %s", idToName(c2));
	  sendTo(roles[1], server_msg);
	  sprintf(server_msg, "Your partner has chosen to target %s", idToName(c1));
	  sendTo(roles[1], server_msg);
	         
	  if (c1 != c2){
	    sendTo(roles[0], "You must agree on the target!\n");
	    sendTo(roles[1], "You must agree on the target!\n");
	  }
	  else {
	    break;
	  }
	}
	
      sendTo(roles[0], "You have chosen to kill <person>. Go to sleep.\n");
      sendTo(roles[1], "You have chosen to kill <person>. Go to sleep.\n");
      if (isAlive[roles[2]])
	phase = COPPREP;
      else
	phase = DAYPREP;

      isAlive[c1] = 0; //ooh killem
     
      break;
      }
      
    case COP:

      while (1){
	
	msg = msgs[roles[2]]; 
	int copChoice = nameToID(msg, names);
	char emptyStr[] = "";
	strcpy(msgs[roles[2]], emptyStr);

	if (copChoice == -1 || copChoice == roles[2]){
	  sendTo(roles[2], "Invalid name");
	}
	else{
	  if ( copChoice == roles[0] || copChoice == roles[1] ){
	    sendTo(roles[2], "This person is a member of the mafia.\n");
	  }
	  else {
	    sendTo(roles[2], "This person is an innocent townsperson.\n");
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
