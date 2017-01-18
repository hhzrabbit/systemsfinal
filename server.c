#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "networking.h"
#include "memctl.h"

void process( char * s );
void sub_server( int sd );

int main() {
  int MAX_PLAYERS = 8;
  int current_players = 0;
  struct sockpair sockpairs[] = calloc(MAX_PLAYERS, sizeof(struct sockpair));
  
  int sd, chat_connection, listener_connection;

  sd = server_setup();

  //fill up the game
  while (current_players < MAX_PLAYERS) { 
    chat_connection = server_connect( sd );
    listener_connection = server_connect( sd );

    struct sockpair sp;
    sp.chat = chat_connection;
    sp.listener = listener.connection;

    sockpairs[current_players] = sp;
    current_players++;

  }
  
  while (1) {


  }

  free(sockpairs);
  return 0;
}

void sub_server( int sd ) {

  char buffer[MESSAGE_BUFFER_SIZE];
  while (read( sd, buffer, sizeof(buffer) )) {

    printf("[SERVER %d] received: %s\n", getpid(), buffer );
    process( buffer );
    write( sd, buffer, sizeof(buffer));    
  }
  
}
void process( char * s ) {

  while ( *s ) {
    *s = (*s - 'a' + 13) % 26 + 'a';
    s++;
  }
}
