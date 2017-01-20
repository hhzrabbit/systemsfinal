#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "networking.h"

struct message { char * content; char wasRead; };

int main( int argc, char *argv[] ) {
  char *host;

  if (argc != 2 ) {
    printf("host not specified, conneting to 127.0.0.1\n");
    host = "127.0.0.1";
  } else {
    host = argv[1];
  }

  int sd = client_connect(host);

  while (1) {
    char msg[100];
    fgets(msg, sizeof(msg), stdin);
    *strchr(msg, '\n') = 0; //remove line break

    printf("Sending message: '%s\n'", msg);
    write(sd, msg, sizeof(msg));
  }


}
