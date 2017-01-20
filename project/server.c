#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#DEFINE PLAYERCOUNT 8
int main() {


  //per connection, assign ID. Each player is running a client.
  sendAll("<Player> has joined the game!\n");
  /* main gameplay */
  //everyone has connected, with IDs according to connection order
  sendAll("Welcome to Mafia!");
  //get a set timer function in here
  int playerCount = PLAYERCOUNT;
  int roles[PLAYERCOUNT];
  int dead[PLAYERCOUNT]; //0 is alive, 1 is dead
  int n;
  sendAll("Now assigning roles");
  //distribute 1-playercount among 1-8
  for (n = 0; n < playerCount, n++){//grab a random player from the list...
    roles[n] = n + randInt() % (playerCount - n);
  }

  sendTo(roles[0], "You are in the mafia! Your partner is %s. Survive!\n", idToName(roles[1]));
  sendTo(roles[1], "You are in the mafia! Your partner is %s. Survive!\n", idToName(roles[0]));
  sendTo(roles[2], "You are the cop! Find out who the mafia are.\n");
  for (n = 3; n < playerCount, n++){
    sendTo(roles[n], "You are a townsperson! Find out who the mafia are.\n");
  }

  //  roles[0] = 1 means player 1 has role 0, which is mafia. index is the player id. 

  int day, night;
  day = 1;
  night = 0;
  //day/night cycle
  int dayCtr = 1;
  int phase = 1;
  while(1){
    
    int dayStart = time(NULL); //this needs to account for voting timeouts (fork?)
    int * playerNoms = (int *)calloc(n, sizeof(int)); //also initialize outside please
    int phaseTimeRemaining = 30;
    int voteStart;
    int yesVotes;
    int noVotes;
    int curTime;
    char * msg;
    char msgs[PLAYERCOUNT][256]; //8 thing array
    int votes[2]; //yes no votes
    
    while (phase == 1 || phase == 2){ //day normal / day voting

      //update msgs
      for (n = 0; n < PLAYERCOUNT; n++){
	strcpy(msgs[n], readMsg(n));
      }
      
      if (phase == 1){
	sendAll("It is currently day %d\n", dayCtr);
	sendAll("Discussion begins.\n");
      
	//timer 
	int timeElapsed = (time(NULL) - timeStart);

	//all information passes through server.
	//read SHMs
	for (n = 0; n < PLAYERCOUNT; n++){
	  
	  strcpy(msg, msgs[n]);
	  if (strlen(msg) == 0){
	    continue;
	  }
	  if (msg[0] = '\\'){//is actually just a \ indicating command
	      //find command
	      char cmd[256];
	      cmd = strsep(token, ' ');
	      if (!strcmp(cmd, "\w")){
		//nice
	      }
	      else if (!strcmp(cmd, '\nom')){
		//nicer
		//token is new nom;
		int newNom;
		if (isValid(token)) newNom = NameToID(token);
		if (newNom != -1) {
		  sendAll("%s has been nominated.", idToName(newNom));
		  *(playerNoms + newNom) += 1;
		  if (* (playerNoms + newNom) == 3){
		    //vote triggered
		    votePrompt(newNom);
		    daytimeRemaining -= timeElapsed;
		    voteStart = time(NULL);
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
		  sendAll(token); //needs to be processed
		}
	      }
	    }
      
	    timeStart = time(NULL);
	  }
	}

	if ((daytimeRemaining - timeElapsed) % 5 = 0) sendAll("Daytime remaining: %d", daytimeRemaining);
  
	//DAY ENDS
	if (timeElapsed >= daytimeRemaining) {
	  sendAll("Daytime has ended. Go to sleep.");
	  phase = 3;
	  free(playerNoms);
	}
      }
      
      else { //voting

	for (n = 0; n < PLAYERCOUNT; n++){
	  
	  strcpy(msg, msgs[n]);
	  if (strlen(msg) == 0){
	    continue;
	  }
	  if (!strcmp(msg, "y")){
	    sendAll("%s has voted for execution.", idToName(n));
	    votes[0]++;
	  }
	  else if (!strcmp(msg, "n")){
	    sendAll("%s has voted against execution.", idToName(n));
	    votes[1]++;
	  }
	  
	}
	curTime = time(NULL);
	

	
	if (curTime - voteStart >= 30){
	  phase = 1;
	  dayStart = curTime;
	  
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
      }
    }
    while (phase == 3){ //night
      sendAll("It is currently night %d\n", dayCtr);
      //mafia prompt (maybe write a sendMafia, something)
      sendTo(roles[0], "Wake up, mafia. Pick a person to kill.\n");
      sendTo(roles[1], "Wake up, mafia. Pick a person to kill.\n");
      
      listen(roles[0]);
      //something something...
      
      sendTo(roles[0], "You have chosen to kill <person>. Go to sleep.\n");
      sendTo(roles[1], "You have chosen to kill <person>. Go to sleep.\n");

      //cop prompt
      sendTo(roles[2], "Wake up, cop. Pick a person to investigate.\n");

      int copChoice = 0;
      while (copChoice != 2){//don't pick yourself silly...
      }
      
      if (roles[0] == copChoice || roles[1] == copChoice){
	sendTo(roles[2], "This person is a member of the mafia.\n");
      }
      else {
	sendTo(roles[2], "This person is an innocent townsperson.\n");
      }
      phase = 1;
    }
    dayCtr++;
  }
  

  int status[playerCount];
  int numAlive;
  //end game
  //(exited a while loop - if sum of alive mafia members > sum of townspeople)
  if (status[0] + status[1] == 0){
    sendAll("Game over. The townspeople have won!\n");
  }
  else if (status[0] + status[1] > numAlive / 2){
    sendAll("Game over. (Defaulted) The mafia outnumber the townspeople, and have won!\n");
  }
  
  return 0;
}

//to avoid clutter in the main file, script is stored outside
char * getScript(int line){
  int fd = open("script.txt", O_RDONLY);
  if (fd == -1) printf("Script file corrupted\n");
  //need to iterate through read lines...
  char lineRet[64];
  read(fd, lineRet, 64);
  close(fd);
  return lineRet;
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

int nameToID(char * name){
  return 0;
}

char * IDToName(int id){
  return "hello";
}


//for now, presume everyone votes (eventually add timer)
void votePrompt(int pid){
  char * condemned = IDToName(pid);
  sendAll("%s has been accused! Should they be executed? (yes/no)\n", condemned);
}

void voteResponse(){

}

//Server tracks timer, forks children to listen for responses. Children connect to clients (one per) and server kills children when timer runs out. When server receives a signal, it increments, and when it reaches 3, run a vote panel
