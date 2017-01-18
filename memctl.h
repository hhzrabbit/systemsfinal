#ifndef MEMCTL_H
#define MEMCTL_H

union semun {
  int val;
  struct semid_ds *buf;
  unsigned short *array;
  struct seminfo *__buf;
};

struct sockpair {
  int chat;
  int listener;
  int shm;
};

int setupShm();
int setupSem();
void semup();
void semdown();

int key;

#endif
