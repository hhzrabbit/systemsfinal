#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>

struct message { char * userID, char * content, char wasRead}

int main() {
  while (1) {
    char msg[100];
    fgets(msg, sizeof(msg), stdin);
    printf(msg);
  }


}
