#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <unistd.h>

int main() {
  initscr();
  cbreak();

  int max_y, max_x;
  getmaxyx( stdscr, max_y, max_x );

  //newwin( height, width, start_y, start_x );
  WINDOW * display = newwin(max_y - 3, max_x, 0, 0); 
  WINDOW * chat = newwin(3, max_x, max_y - 3, 0);
  refresh();
  
  box(display, 0, 0);
  wrefresh(display);

  box(chat, 0, 0);
  wrefresh(chat);

  //  keypad(chat, 
  
  wmove(chat, 1, 1); //move cursor inside chat box
  int chatLine = 0; //where to print chat
  int display_height = max_y - 3;

  
  while (1) {
    char str[100];
    wgetstr(chat, str);

    if ( strcmp(str, "quit") ) {
      if ( chatLine + 3 > display_height) { //going out of bounds
	wmove(display, 1, 1);
	wdeleteln(display);
	wmove(display, display_height-2, 1);
	winsertln(display);
	box(display, 0, 0);
	wrefresh(display);
	chatLine--;
      }
      
      mvwprintw(display, 1 + chatLine, 1, "%s", str);
      chatLine++;
      wmove(chat, 1, 1);
      wclrtoeol(chat);
      wrefresh(display);
    } else {
      endwin();
      break;
    }

  }

  
  
  return 0;
}
