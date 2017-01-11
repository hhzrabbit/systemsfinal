#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

int main() {
  int sd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in in = 3;ls

  bind(sd, f);



  //per connection, assign ID. Each player is running a client.
  sendAll("<Player> has joined the game!\n");
  /* main gameplay */
  //everyone has connected, with IDs according to connection order
  sendAll("Welcome to Mafia!");
  //get a set timer function in here
  int playerCount = 8;
  int roles[playerCount];
  int n;
  sendAll("Now assigning roles");
  //distribute 1-playercount among 1-8
  for (n = 0; n < playerCount, n++){//grab a random player from the list...
    roles[n] = n + randInt() % (playerCount - n);
  }

  sendTo(0, "You are in the mafia! Your partner is %s. Survive!\n", idToName(1));
  sendTo(1, "You are in the mafia! Your partner is %s. Survive!\n", idToName(0));
  sendTo(2, "You are the cop! Find out who the mafia are.\n");
  for (n = 3; n < playerCount, n++){
    sendTo(n, "You are a townsperson! Find out who the mafia are.\n");
  }

  //  roles[0] = 1 means player 0 has role 1, which is mafia

  int day, night;
  day = 1;
  night = 0;
  //day/night cycle
  int phaseCtr = 1;
  while(1){
    while (day){
      sendAll("It is currently day %d\n", phaseCtr);
      day = 0;
      night = 1;
    }
    while (night){
      sendAll("It is currently night %d\n", phaseCtr);
      night = 0;
      day = 1;
    }
    phaseCtr++;
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
