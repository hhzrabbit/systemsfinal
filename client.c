#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/sem.h>

#include "networking.h"
#include "memctl.h" //functions dealing with shm and semaphore

int main( int argc, char *argv[] ) {

  int f = fork(); //split into chat & game client

  if (f == 0) { //child process - chat, LISTENS TO USER, SENDS MSGS TO SERVER FOR PROCESSING
    char *host;
    if (argc != 2 ) {
      printf("host not specified, conneting to 127.0.0.1\n");
      host = "127.0.0.1";
    }
    else host = argv[1];
    
    int sd;
    
    sd = client_connect( host );

    char buffer[MESSAGE_BUFFER_SIZE];
  
    while (1) {
      //read in chat input
      printf("> ");
      fgets( buffer, sizeof(buffer), stdin ); //block here
      char *p = strchr(buffer, '\n');
      *p = 0;
      
      //talk to the server
      write( sd, buffer, sizeof(buffer) );
      read( sd, buffer, sizeof(buffer) );
      printf( "received: %s\n", buffer );
    }
  }
  else { //parent process - ONLY LISTENING TO SERVER INSTRUCTIONS
    char *host;
    if (argc != 2 ) {
      printf("host not specified, conneting to 127.0.0.1\n");
      host = "127.0.0.1";
    }
    else host = argv[1];
  
    int sd;
    char buffer[MESSAGE_BUFFER_SIZE];

    sd = client_connect( host );
    
    while (1){ 
      //LISTEN TO THE SERVER FOR MESSAGES
     
      read( sd, buffer, sizeof(buffer) ); 
      printf( "received: %s\n", buffer ); 


    }
  }
    return 0;
}

