#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void process( char * s );
void sub_server( int sd );
int amtPlayers;
int inSession

int main() {

  int sd, connection;
  amtPlayers = 0;
  inSession = 0;

  sd = server_setup();
    
  while (1) {

    connection = server_connect( sd );

    int f = fork();
    if ( f == 0 ) {

      close(sd);
      sub_server( connection );

      exit(0);
    }
    else {
      amtPlayers++;
      close( connection );
      printf("Amount of players: %d\n", amtPlayers);
      if (amtPlayers >= 4)
	inSession = 1;
    }
  }
  return 0;
}


void sub_server( int sd ) {
  char buffer[MESSAGE_BUFFER_SIZE];
  
  //while ! inSession keep sending the amount of players to the clients

  //in session: 
  while (read( sd, buffer, sizeof(buffer))) {

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
