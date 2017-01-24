#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <unistd.h>

#include "networking.h"

int server_sock; //server socket id
int chatLine = 0; //where to print chat
int display_height;


void displayMsg( WINDOW * cur_window, WINDOW * display, char * message ) {
  int cur_y, cur_x;
  getyx(cur_window, cur_y, cur_x);
  
  if ( chatLine + 3 > display_height ) { //going out of bounds
    wmove(display, 1, 1);
    wdeleteln(display);
    wmove(display, display_height-2, 1);
    winsertln(display);
    box(display, 0, 0);
    chatLine--;
  }
  
  mvwprintw(display, 1 + chatLine, 1, message);
  wrefresh(display);
  chatLine++;
  wmove(cur_window, cur_y, cur_x);
  wrefresh(cur_window);
}

int main( int argc, char ** argv ) {
  initscr();
  cbreak();
  
  int max_y, max_x;
  getmaxyx( stdscr, max_y, max_x );

  display_height = max_y - 3;
  
  //newwin( height, width, start_y, start_x );
  WINDOW * display = newwin(max_y - 3, max_x, 0, 0); 
  WINDOW * chat = newwin(3, max_x, max_y - 3, 0);
  refresh();
  
  box(display, 0, 0);
  box(chat, 0, 0);

  //keypad(chat, true);
  wmove(chat, 1, 1); //move cursor inside chat box

    
  //ACTUAL CLIENT STUFF STARTS HERE
  char *host;
  if (argc != 2 ) {
    displayMsg(chat, display, "host not specified, connecting to 127.0.0.1");
    host = "127.0.0.1";
  }
  else {
    host = argv[1];
  }
  
  //order matters here bc server connects to chat first
  server_sock = client_connect( host );  
  displayMsg(chat, display, "Please wait while people join the game...");

  wrefresh(chat);
  wrefresh(display);
  
  char buffer[MESSAGE_BUFFER_SIZE];
  while ( read(server_sock, buffer, sizeof(buffer)) ) {
    if ( ! strcmp(buffer, "***BEGIN***") ) {
      break;
    }
    displayMsg(chat, display, buffer);
    memset(buffer, 0, MESSAGE_BUFFER_SIZE);
  }


  int f = fork();
  if (f == 0) { //this is the main client (child)
    char servMsg[MESSAGE_BUFFER_SIZE];
    while ( read(server_sock, servMsg, sizeof(servMsg) ) ) {
      displayMsg(chat, display, servMsg);
      memset(servMsg, 0, MESSAGE_BUFFER_SIZE);

    }
  }
  
  

  //this is the chat that sends to server
  while (1) {
    //THE CRITICAL BUG FIXER :/
    //displayMsg(chat, display, "Active parent");
    //wprintw(chat, ">>> ");
    wrefresh(chat);
    
    wgetstr(chat, buffer);
    
    write (server_sock, buffer, sizeof(buffer) );
    wmove(chat, 1, 1);
    wclrtoeol(chat);

  }

  endwin(); //never happens lmao
  return 0;
}
