#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "networking.h"


int server_sock; //server socket id


int main( int argc, char *argv[] ) {
  char *host;
  if (argc != 2 ) {
    printf("host not specified, conneting to 127.0.0.1\n");
    host = "127.0.0.1";
  }
  else
    host = argv[1];
  
  //order matters here bc server connects to chat first
  server_sock = client_connect( host );

  printf("%s\n", "Please wait while people join the game...");
  
  char buffer[MESSAGE_BUFFER_SIZE];
  while ( read(server_sock, buffer, sizeof(buffer)) ) {
    if ( ! strcmp(buffer, "***BEGIN***") ) {
      printf("%s\n", buffer);
      break;
    }
  }
  
  
  int f = fork();
  if (f == 0) { //this is the main client (child)
    char buffer[MESSAGE_BUFFER_SIZE];
    while ( read(server_sock, buffer, sizeof(buffer) ) ) {
      printf("%s", buffer); //message should have \n already
    }
  }

  
  //this is the chat that sends to server
  while (1) {
    printf("enter message: ");
    fgets( buffer, sizeof(buffer), stdin );
    write (server_sock, buffer, sizeof(buffer) );
  }
  
}
