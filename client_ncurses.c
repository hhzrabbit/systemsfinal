#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <unistd.h>

#include "networking.h"

int server_sock; //server socket id
int chatLine = 0; //where to print chat
int display_height;
WINDOW * display;
WINDOW * chat;

void displayMsg( char * message ) {
  if ( chatLine + 3 > display_height ) { //going out of bounds
    wmove(display, 1, 1);
    wdeleteln(display);
    wmove(display, display_height-2, 1);
    winsertln(display);
    chatLine--;
  }

  char newkek[100];
  sprintf(newkek, " (chatLine=%d)", chatLine);

  char newnewkek[1000];
  strcpy(newnewkek, message);
  strcat(newnewkek, newkek);
  mvwprintw(display, 1 + chatLine, 1, newnewkek);
  chatLine++;
  
  box(display, 0, 0);
  box(chat, 0, 0);

  wrefresh(display);
  wrefresh(chat);
}


char ** parseMsg(char * servMsg) {
  char ** strings = (char **) malloc (MESSAGE_BUFFER_SIZE * MESSAGE_BUFFER_SIZE);
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




int main( int argc, char ** argv ) {
  initscr();
  cbreak();
  
  int max_y, max_x;
  getmaxyx( stdscr, max_y, max_x );

  display_height = max_y - 3;
  
  //newwin( height, width, start_y, start_x );
  display = newwin(max_y - 3, max_x, 0, 0); 
  chat = newwin(3, max_x, max_y - 3, 0);
  refresh();

  box(display, 0, 0);
  box(chat, 0, 0);
  wmove(chat, 1, 1);
  
  keypad(chat, true);

    
  //ACTUAL CLIENT STUFF STARTS HERE
  char *host;
  if (argc != 2 ) {
    displayMsg("host not specified, connecting to 127.0.0.1");
    host = "127.0.0.1";
  }
  else {
    host = argv[1];
  }
  
  //order matters here bc server connects to chat first
  server_sock = client_connect( host );  
  displayMsg("Please wait while people join the game...");

  char buffer[MESSAGE_BUFFER_SIZE];
  while ( read(server_sock, buffer, sizeof(buffer)) ) {
    char beginCode[] = "***BEGIN***";
    char ** messages = parseMsg(buffer);

    int i = 0;
    char began = 0; //to make sure msgs don't get cut off
    while (messages[i+1]) { //i+1 to remove that trailing \n
      if (! strcmp(messages[i], beginCode)) {
	began = 1;
      }
      else {
	displayMsg(messages[i]);
      }
      i++;
    }
    memset(buffer, 0, MESSAGE_BUFFER_SIZE);    
    free(messages);
    messages = NULL;

    if (began) break;
  }


  int f = fork();
  if (f == 0) { //this is the main client (child)
    int f2 = fork();
    if (f2 == 0) { //the extreme bug eradicator
      while (1) {
	if (getppid() == 1) exit(0); //parent exited
	wrefresh(display);
	wrefresh(chat);
	sleep(1);
      }
    }
    char servMsg[MESSAGE_BUFFER_SIZE];
    while ( read(server_sock, servMsg, sizeof(servMsg) ) ) {
      char ** messages = parseMsg(servMsg);

      int i = 0;
      while (messages[i]) {
	displayMsg(messages[i]);
	i++;
      }

      free(messages);
      messages = NULL;
      memset(servMsg, 0, MESSAGE_BUFFER_SIZE);
    }
  }

 

  //this is the chat that sends to server
  while (1) {
    //wprintw(chat, ">>> ");
    wrefresh(chat);
    //wrefresh(display);
    wgetstr(chat, buffer);
    
    write (server_sock, buffer, sizeof(buffer) );
    wmove(chat, 1, 1);
    wclrtoeol(chat);
  }

  endwin(); //never happens lmao
  return 0;
}
