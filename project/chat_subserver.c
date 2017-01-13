#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>


//void process( char * s );
//void sub_server( int sd );

int main() {
  int sd;
  char *addr;
  sd = shmget( 1337, 256, IPC_CREAT | 0644 );

  addr = shmat(sd, 0, 0);

  int f = fork();
  if (f == 0) {
    while (1) {
      printf("child/n");
      sleep(1);
    }
  } else {
    wait();
    while (1) {
      printf("parent/n");
      sleep(1);
    }
  }
  
  /*
  int sd, connection;
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
      close( connection );
    }
  }
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

void process( char * msg ) {
  if (msg[0] == '/') {
    char ** command;
    int i = 0;
    s = msg;
    //    while (msg[i] = strsep(&s, " ") {
  }
  */

}
