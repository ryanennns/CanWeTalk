/* splitWin.c is a simple example to show how to deal with split screens.
   Due to the limited time, this program is not finished yet.

   To compile:   gcc splitWin.c -lncurses

		Sam Hsu (11/17/10)
*/ // bruh i'm literally gonna clone this code and not understand a bit of it

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ncurses.h>

////////////////////////////////////////////
#include <pthread.h>
#include "../inc/chat-client.h"
#include "../../common/inc/common.h"

static int MASTER_ROW = 0;
static char* TEST_VALS[5] = { "test1", "test2", "test3", "test4", "test5" };

void* listenerThread(void* param);
void flush();
////////////////////////////////////////////     
WINDOW *create_newwin(int, int, int, int);
void destroy_win(WINDOW *);
void input_win(WINDOW *, char *);
void display_win(WINDOW *, char *, int, int);
void destroy_win(WINDOW *win);
void blankWin(WINDOW *win);
     
void appendToWindow( WINDOW* win, char* word, int shouldBlank )
{
  display_win(win, word, MASTER_ROW, shouldBlank);
  MASTER_ROW++;
}

int window_loop(int server_socket, const char* userName)
{
  pthread_t listener;

  static WINDOW *msg_win;
  static WINDOW *chat_win;
  int chat_startx, chat_starty, chat_width, chat_height;
  int msg_startx, msg_starty, msg_width, msg_height, i;
  int shouldBlank;
  char buf[BUFSIZ];

  initscr();                      /*Start curses mode*/
  cbreak();
  noecho();
  refresh();
  
  shouldBlank = 0;

  chat_height = 5;
  chat_width  = COLS - 2;
  chat_startx = 1;        
  chat_starty = LINES - chat_height;        
     
  msg_height = LINES - chat_height - 1;
  msg_width  = COLS;
  msg_startx = 0;        
  msg_starty = 0;        
  
  /* create the input window */
  msg_win = create_newwin(msg_height, msg_width, msg_starty, msg_startx);
  scrollok(msg_win, TRUE);

  /* create the output window */
  chat_win = create_newwin(chat_height, chat_width, chat_starty, chat_startx);
  scrollok(chat_win, TRUE);

  listenerParameters lp;
  lp.window = msg_win;
  lp.socket = server_socket;

  if (pthread_create( &listener, NULL, listenerThread, (void *)&lp))
  {
    printf("failed to create thread");
    fflush(stdout);
    return 5;
  }

  sleep(0.1); /* */

  int done = 1;
  while(done == 1)
  {
    // reset buffer to nill
    memset(buf,0,MAX_MSG);
    // get input from the user
    input_win(chat_win, buf);
    replace(buf, '|', ';');
    if( strlen(buf) < 40 )
    {
      char message[BUFSIZ];

      // strip a newline from the input, if it is present
      if (buf[strlen (buf) - 1] == '\n')
        buf[strlen (buf) - 1] = '\0';

      // format the message -- ONLY the username, msg and time()
      sprintf(message, "[%-5s] >>|%-40s|(HH:MM:SS)", userName, buf);

      // if the user inputs >>bye<<, we can set the done flag to 0
      if(strcmp(buf,">>bye<<") == 0)
        done = 0;

      // appendToWindow(lp.window, message, 0);

      // done or not, we write to the server
      write (server_socket, message, strlen (message));
    }
  }

  shouldBlank = 1;
  sprintf(buf,"Messaging is complete ... destroying window in 5 seconds");
  MASTER_ROW = 0;
  appendToWindow(msg_win, buf, 1);
  
  sleep(2);                   /* to get a delay */  
     
  destroy_win(chat_win);
  destroy_win(msg_win);
  endwin();

  printf("DONE\n");
}
     
WINDOW *create_newwin(int height, int width, int starty, int startx)
{
  WINDOW *local_win;
     
  local_win = newwin(height, width, starty, startx);
  box(local_win, 0, 0);               /* draw a box */
  wmove(local_win, 1, 1);             /* position cursor at top */
  wrefresh(local_win);     
  return local_win;
}
     
/* This function is for taking input chars from the user */
void input_win(WINDOW *win, char *word)
{
  int i, ch;
  int maxrow, maxcol, row = 1, col = 0;
     
  blankWin(win);                          /* make it a clean window */
  getmaxyx(win, maxrow, maxcol);          /* get window size */
  bzero(word, BUFSIZ);
  wmove(win, 1, 1);                       /* position cusor at top */
  for (i = 0; (ch = wgetch(win)) != '\n'; i++) 
  {
    word[i] = ch;                       /* '\n' not copied */
    if (col++ < maxcol-2)               /* if within window */
    {
      wprintw(win, "%c", word[i]);      /* display the char recv'd */
    }
    else                                /* last char pos reached */
    {
      col = 1;
      if (row == maxrow-2)              /* last line in the window */
      {
        scroll(win);                    /* go up one line */
        row = maxrow-2;                 /* stay at the last line */
        wmove(win, row, col);           /* move cursor to the beginning */
        wclrtoeol(win);                 /* clear from cursor to eol */
        box(win, 0, 0);                 /* draw the box again */
      } 
      else
      {
        row++;
        wmove(win, row, col);           /* move cursor to the beginning */
        wrefresh(win);
        wprintw(win, "%c", word[i]);    /* display the char recv'd */
      }
    }
  }
}  /* input_win */
     
void display_win(WINDOW *win, char *word, int whichRow, int shouldBlank)
{
  if(shouldBlank == 1) blankWin(win);                /* make it a clean window */
  wmove(win, (whichRow+1), 1);                       /* position cusor at approp row */
  wprintw(win, word);
  wrefresh(win);
} /* display_win */
     
void destroy_win(WINDOW *win)
{
  delwin(win);
}  /* destory_win */
     
void blankWin(WINDOW *win)
{
  int i;
  int maxrow, maxcol;
     
  getmaxyx(win, maxrow, maxcol);
  for (i = 1; i < maxcol-2; i++)  
  {
    wmove(win, i, 1);
    refresh();
    wclrtoeol(win);
    wrefresh(win);
  }
  box(win, 0, 0);             /* draw the box again */
  wrefresh(win);
}  /* blankWin */

void* listenerThread(void* param)
{
  listenerParameters lp = *((listenerParameters*)param);
  char b[PACKET_WIDTH];

  while( 1 )
  {
      // our local buffer, just in case
      //memset(b,0,PACKET_WIDTH);
      //int numBytesRead = read (lp.socket, b, sizeof(b));

      //if(strcmp(b, ">>bye<<") == 0) break;

      if( 1 > 0)
      {
        //printf("%s\n",b);
        //printf("%ld : %d", strlen(b), numBytesRead);
        appendToWindow( lp.window, "test word", 0 );
      }

    sleep(1);
  }
  pthread_exit((void*) 0);
}

void flush()
{
  fflush(stdout);
  fflush(stdin);
}

/*
  sleep(1);
  while(1)
  {
    appendToWindow(lp.window, "192.168.2.112   [asdf ] >> test                                     (HH:MM:SS)", 0);
    sleep(1);
  }

  pthread_exit((void*) 0);
*/