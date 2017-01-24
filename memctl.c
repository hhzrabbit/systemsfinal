#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <time.h>

#include "memctl.h"

int getRandom(){
  srand(time(NULL));
  return rand();
}

int setupShm(){
  int shmid;
  char * shm;

  key = ftok("makefile", getRandom());

  //make shm
  shmid = shmget(key, 100 * 255 * sizeof(char), IPC_CREAT | 0644); //size for 10 msgs
  //printf("shared memory created, id %d\n", shmid);

  //clear out shm
  shm = (char *) shmat(shmid, 0, 0);
  char empty_str[] = "";
  shm = strcpy(shm, empty_str);
  
  //printf("shared memory value set: %s\n", shm);

  return shmid;
}

int setupSem(){
  int semid, sc;
  union semun su;

  key = ftok("makefile", getRandom());

  //make sem
  semid = semget(key, 1, IPC_CREAT | 0644);
  //printf("semaphore created, id %d\n", semid);

  //set sem value
  su.val = 1;
  sc = semctl(semid, 0, SETVAL, su);
  //printf("semaphore value set: %d\n", sc);

  return semid;
}

void semup(int semid){
  struct sembuf sb;
  sb.sem_num = 0;
  sb.sem_op = 1;
  sb.sem_flg = SEM_UNDO;

  semop(semid, &sb, 1);
}

void semdown(int semid){
  struct sembuf sb;
  sb.sem_num = 0;
  sb.sem_op = -1;
  sb.sem_flg = SEM_UNDO;

  semop(semid, &sb, 1);
}
