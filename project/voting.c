#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

//presuming that stuff is set up, multiple clients.
//Functions needed
//Send message to all clients
//Gather responses from all clients


//voting process
void daytime(){//this is probably in the main function
  int timeStart = time(NULL); //this needs to account for voting timeouts (fork?)
  int daytimeRemaining = 30 - (time(NULL) - timeStart);

  int * playerNoms = (int *)calloc(n, sizeof(int)); //also initialize outside please
  
  int newNom = dayListen();
  
  if (newNom != -1) {
    *(playerNoms + newNom) += 1;
    if (* (playerNoms + newNom) == 3){
      //start vote
      int timeOutStart = time(NULL); //hmmm
      votePrompt(newNom);
    }
  }


  
  if (daytimeRemaining % 5 = 0) sendAll("Daytime remaining: %d\n", daytimeRemaining);
  
  if (timer == 0) sendAll("Daytime has ended. Go to sleep.\n");
}

//3 votes triggers execution panel
int nomineeListen(){
  char * nominated = receiveVote();
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
  listen();
}

void voteResponse(){

}

void sendAll(char * msg){

}

//Server tracks timer, forks children to listen for responses. Children connect to clients (one per) and server kills children when timer runs out. When server receives a signal, it increments, and when it reaches 3, run a vote panel
