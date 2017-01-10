#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>


int main() {
  int sd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in in = 3;ls

  bind(sd, f);



  //per connection, assign ID. Each player is running a client.
  sendAll("<Player> has joined the game!\n");
  /* main gameplay */
  //everyone has connected, with IDs according to connection order
  sendAll();
  //get a set timer function in here

  
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
