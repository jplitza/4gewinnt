#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#ifndef WIN32
#include <sys/ioctl.h>
#endif

short unsigned int field[10][9], colpos = 0, curplayer = 1;
char *msg;

WINDOW * gamewin, * msgwin;

void draw_field() /* {{{ */
{
  // draw game grid
  short unsigned int x, y;
  for(x = 0; x <= 7; x++)
    for(y = 0; y <= 6; y++)
    {
      if(y == 0)
        mvwaddch(gamewin, 1, 2+x*2, ' ');
      else
      {
        mvwaddch(gamewin, 1+y, 1+x*2, ACS_VLINE);
        if(x < 7)
        {
          if(field[x][y-1] != 0)
            mvwaddch(gamewin, 1+y, 2+x*2, ACS_DIAMOND | COLOR_PAIR(field[x][y-1]));
          else
            mvwaddch(gamewin, 1+y, 2+x*2, ' ');
        }
      }
    }
  mvwaddch(gamewin, 1, 2+colpos*2, ACS_DIAMOND | COLOR_PAIR(curplayer));
  wmove(gamewin, 0, 0);
  wrefresh(gamewin);
} /* }}} */

void draw_msgwin() /* {{{ */
{
  unsigned int x, y;
  getmaxyx(stdscr, y, x);
  mvwprintw(msgwin, 0, 0, " %-*s ", x-2, msg);
  wattron(msgwin, A_REVERSE);
  mvwaddstr(msgwin, 1, 2, " Q Quit ");
  wattroff(msgwin, A_REVERSE);
  waddstr(msgwin, "  ");
  wattron(msgwin, A_REVERSE);
  waddstr(msgwin, " R Restart ");
  wattroff(msgwin, A_REVERSE);
  waddstr(msgwin, "  ");
  wattron(msgwin, A_REVERSE);
  waddstr(msgwin, " <space> drop stone ");
  wattroff(msgwin, A_REVERSE);
  waddstr(msgwin, "  ");
  wattron(msgwin, A_REVERSE);
  waddstr(msgwin, " <- move left");
  wattroff(msgwin, A_REVERSE);
  waddstr(msgwin, "  ");
  wattron(msgwin, A_REVERSE);
  waddstr(msgwin, " -> move right");
  wattroff(msgwin, A_REVERSE);
  wrefresh(msgwin);
} /* }}} */

#ifndef WIN32
void resize_handler(int sig) /* {{{ */
{
  // handle window resizes and redraw accordingly
  struct winsize ws;
  ioctl(0, TIOCGWINSZ, &ws);
  resizeterm(ws.ws_row, ws.ws_col);
  mvwin(msgwin, ws.ws_row - 2, 0);
  wresize(msgwin, 2, ws.ws_col);
  wresize(gamewin, ws.ws_row - 2, ws.ws_col);
  draw_field();
  draw_msgwin();
} /* }}} */
#endif

int get_stone(short unsigned int col) /* {{{ */
{
  short unsigned int y;

  if(col > 6 || field[col][0] != 0)
    return -1;

  for(y = 0; y < 6; y++)
    if(field[col][y] != 0)
      return y - 1;
  return 5;
} /* }}} */

int set_stone(short unsigned int col) /* {{{ */
{
  if(get_stone(col) == -1)
    return 0;
  field[col][get_stone(col)] = curplayer;
  curplayer ^= 3;
  return 1;
} /* }}} */

int ai_check(short unsigned int a[]) /* {{{ */
{
  // strange check of an array of fields:
  // - if more than one 0 is contained, abort
  // - if stones of both players are contained, abort
  // - return the index of the 0 otherwise
  unsigned short int i, first = 0;
  short int gotzero = -1;
  for(i = 0; i < sizeof(a); i++)
  {
    /*if(a[i] == 0)
    {
      if(gotzero == -1)
      {
        gotzero = i;
        continue;
      }
      else
        return -2;
    }

    if(first == 0)
      first = a[i];
    else if(first != a[i])
      return -1;*/

    if(a[i] == 0 && gotzero == -1)
      gotzero = i;
    else if(a[i] == 0)
      return -1;
    else if(first == 0)
      first = a[i];
    else if(first != a[i])
      return -1;
  }
  return gotzero;
} /* }}} */

int ai_turn() /* {{{ */
{
  short unsigned int x, y, temp, array[4];
  short int find;
  // this double-loop checks if there are 3 stones in a row
  // if there are, it is the most wise to complete it - either blocking
  // the enemy from getting 4-in-a-row or getting it myself
  for(x = 0; x < 7; x++)
    for(y = 0; y < 6; y++)
    {
      temp = field[x][y];

      // horizontal match
      if(x < 7 - 3)
      {
        array[0] = field[x][y];
        array[1] = field[x+1][y];
        array[2] = field[x+2][y];
        array[3] = field[x+3][y];
        find = ai_check(array);
        if(find > -1 && get_stone(x+find) == y)
        {
          msg = malloc(10);
          sprintf(msg, "!%d,%d", x+find, y);
          return set_stone(x+find);
        }
      }

      // vertical match
      if(y < 6 - 2 && y > 1 &&
         temp != 0 &&
         temp == field[x][y+1] &&
         temp == field[x][y+2] &&
            0 == field[x][y-1])
        return set_stone(x);

      // bottom-right diagonal match
      if(x < 7 - 3 && y < 6 - 3)
      {
        array[0] = field[x][y];
        array[1] = field[x+1][y+1];
        array[2] = field[x+2][y+2];
        array[3] = field[x+3][y+3];
        find = ai_check(array);
        if(find > -1 && get_stone(x+find) == y+find)
        {
          msg = malloc(10);
          sprintf(msg, "!%d,%d", x+find, y+find);
          return set_stone(x+find);
        }
      }

      // bottom-left diagonal match
      if(x > 2 && y < 6 - 3)
      {
        array[0] = field[x][y];
        array[1] = field[x-1][y+1];
        array[2] = field[x-2][y+2];
        array[3] = field[x-3][y+3];
        find = ai_check(array);
        if(find > -1 && get_stone(x-find) == y+find)
        {
          msg = malloc(10);
          sprintf(msg, "!%d,%d", x-find, y+find);
          return set_stone(x-find);
        }
      }
      draw_msgwin();
    }

  // no definite field could be found, randomly choose ;)
  srand((unsigned) time(NULL));
  while(!set_stone(rand() % 7));
  return 1;
} /* }}} */

short int get_winner() /* {{{ */
{
  // determine if there is a winner and return id, -1 on draw and 0 otherwise
  // TODO: I think this can be done nicer
  short unsigned int x, y, temp;
  for(x = 0; x < 7; x++)
    for(y = 0; y < 6; y++)
    {
      temp = field[x][y];
      if(temp == 0)
        continue;

      // horizontal match
      if(x < 7 - 3 &&
         temp == field[x+1][y] &&
         temp == field[x+2][y] &&
         temp == field[x+3][y])
        return temp;

      // vertical match
      if(y < 6 - 3 &&
         temp == field[x][y+1] &&
         temp == field[x][y+2] &&
         temp == field[x][y+3])
        return temp;

      // bottom-right diagonal match
      if(x < 7 - 3 &&
         y < 6 - 3 &&
         temp == field[x+1][y+1] &&
         temp == field[x+2][y+2] &&
         temp == field[x+3][y+3])
        return temp;

      // bottom-left diagonal match
      if(x > 2 &&
         y < 6 - 3 &&
         temp == field[x-1][y+1] &&
         temp == field[x-2][y+2] &&
         temp == field[x-3][y+3])
        return temp;
    }
  for(x = 0; x < 7; x++)
  {
    if(field[x][0] == 0)
      break;
    else if(x == 6)
      return -1;
  }
  return 0;
} /* }}} */

int main(int argc, char *argv[])
{
  short unsigned int running = 1, input, use_ai;
  int h, w;

  memset(field, 0, sizeof(field));

  // initialize ncurses {{{
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  // colors
  start_color();
  init_color(COLOR_BLUE, 0, 0, 999);
  init_color(COLOR_RED, 999, 0, 0);
  init_pair(0, COLOR_WHITE, COLOR_BLACK);
  init_pair(1, COLOR_BLUE, COLOR_BLACK);
  init_pair(2, COLOR_RED, COLOR_BLACK);
  // windows
  getmaxyx(stdscr, h, w);
  msgwin = newwin(2, w, h - 2, 0);
  gamewin = newwin(h - 2, w, 0, 0);
#ifndef WIN32
  signal(SIGWINCH, resize_handler);
#endif
  refresh();
  // }}}
  msg = "Welcome to Connect Four!";
  draw_field();
  draw_msgwin();

  if(argc > 1 && !strcmp(argv[1],"-ai"))
    use_ai = 1;

  while(running == 1)
  {
    if(use_ai == 1 && curplayer == 2 && !get_winner())
    {
      // AI's turn
      ai_turn();
    } else {
      // get user's keypress
      input = getch();
      switch(input)
      {
        case 'q':
          msg = "To quit, press capital Q.";
          break;
        case 'Q':
          // quit
          running = 0;
          break;
        case 'r':
          if(get_winner() == 0)
          {
            msg = "To restart, press capital R.";
            break;
          }
        case 'R':
          memset(field, 0, sizeof(field));
          curplayer = 1;
          colpos = 0;
          msg = "Welcome to Connect Four!";
          break;
        case KEY_LEFT:
          if(get_winner() != 0)
            break;
          if(colpos == 0)
            colpos = 6;
          else
            colpos--;
          break;
        case KEY_RIGHT:
          if(get_winner() != 0)
            break;
          if(colpos == 6)
            colpos = 0;
          else
            colpos++;
          break;
        case ' ':
          if(get_winner() != 0)
            break;
          set_stone(colpos);
          break;
      }
    }
    draw_field();
    switch(get_winner())
    {
      case -1:
        msg = "DRAW! Press R to restart";
        break;
      case 1:
        msg = "BLUE WINS! Press R to restart";
        break;
      case 2:
        msg = "RED WINS! Press R to restart";
        break;
    }
    draw_msgwin();
  }
  endwin();
  return 0;
}
