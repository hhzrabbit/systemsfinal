#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <unistd.h>

char ** parseMsg(char * servMsg) {
  char ** strings = (char **) malloc (255*255);
  char * s = servMsg;
  char * p = s;
  
  int i = 0;
  while (p != NULL) {
    p = strsep( &s, "|" );
    strings[i] = p;
    p = s;
    i++;
  }
  strings[i] = NULL;
  return strings;
}

int main() {
  char buff[] = "string|";
  char ** strings = (char **) malloc (255 * 255);
  strings = parseMsg(buff);
  int i = 0;
  while (strings[i]) {
    printf("%s\n", strings[i]);
    i++;
  }
  
  return 0;

}
