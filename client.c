#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/sem.h>

#include "networking.h"

union semun {
  int val;
  struct semid_ds *buf;
  unsigned short *array;
  struct seminfo *__buf;
};

int setupShm(){
  int key, shmid;
  int * shm;

  key = ftok("README.md", getpid());

  //make shm
  shmid = shmget(key, 4, IPC_CREAT | 0644);
  printf("shared memory created, id %d\n", shmid);

  //clear out shm
  shm = shmat(shmid, 0, 0);
  * shm = 0;
  
  printf("shared memory value set: %d\n", * shm);

  return shmid;
}

int setupSem(){
  int key, semid, sc;
  union semun su;

  key = ftok("README.md", getpid());

  //make sem
  semid = semget(key, 1, IPC_CREAT | 0644);
  printf("semaphore created, id %d\n", semid);


  //set sem value
  su.val = 1;
  sc = semctl(semid, 0, SETVAL, su);
  printf("semaphore value set: %d\n", sc);

  return semid;
}


int main( int argc, char *argv[] ) {
  int shmid, semid;

  shmid = setupShm(); //creates shm for chat & game client to use
  semid = setupSem(); //creates sem to control shm
  
  int f = fork(); //split into chat & game client

  if (f == 0) { //child process - chat
    char buffer[MESSAGE_BUFFER_SIZE];
  
    while (1) {
      //read in chat input
      printf("> ");
      fgets( buffer, sizeof(buffer), stdin ); //block here
      char *p = strchr(buffer, '\n');
      *p = 0;

      //process chat input
      printf("%s\n", buffer);
    }
  }
  else { //parent process - game client
    char *host;
    if (argc != 2 ) {
      printf("host not specified, conneting to 127.0.0.1\n");
      host = "127.0.0.1";
    }
    else host = argv[1];
  
    int sd;

    sd = client_connect( host );
    
    while (1){ 

      //check the shm
      //process the shm's contents as necessary
    

      //talk to the server
      /* write( sd, buffer, sizeof(buffer) ); */
      /* read( sd, buffer, sizeof(buffer) ); */
      /* printf( "received: %s\n", buffer ); */


    }
  }
    return 0;
}

