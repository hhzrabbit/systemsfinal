#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "networking.h"

struct socket {
  int writeEnd; //sd to server (chat)
  int readEnd; //sd from server
};

int main( int argc, char *argv[] ) {

  struct socket me;
  
  char *host;
  if (argc != 2 ) {
    printf("host not specified, conneting to 127.0.0.1\n");
    host = "127.0.0.1";
  }
  else
    host = argv[1];
  
  //order matters here bc server connects to chat first
  me.writeEnd = client_connect( host );
  me.readEnd = client_connect( host );

  printf("%s\n", "Please wait while people join the game...");
  
  char buffer[MESSAGE_BUFFER_SIZE];
  while ( read(me.readEnd, buffer, sizeof(buffer)) ) {
    if ( ! strcmp(buffer, "***BEGIN***") ) {
      break;
    }
  }
  
  
  int f = fork();
  if (f == 0) { //this is the main client (child)
    close(me.writeEnd);
    char buffer[MESSAGE_BUFFER_SIZE];
    while ( read(me.readEnd, buffer, sizeof(buffer)) ) {
      printf("%s", buffer); //message should have \n already
    }
  }

  
  //this is the chat that sends to server
  close(me.readEnd);
  while (1) {
    printf("enter message: ");
    fgets( buffer, sizeof(buffer), stdin );
    write (me.writeEnd, buffer, sizeof(buffer) );
  }
  
}
