#ifndef MEMCTL_H
#define MEMCTL_H

union semun {
  int val;
  struct semid_ds *buf;
  unsigned short *array;
  struct seminfo *__buf;
};

int setupShm();
int setupSem();
void semup();
void semdown();

int key;

#endif
