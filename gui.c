#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <string.h>
#include <signal.h>
#include <string.h>
#include <getopt.h>
#ifdef WIN32
#include <windows.h>
#include "basename.h"
#else
#include <libgen.h>
#include <unistd.h>
#include <sys/ioctl.h>
#endif

#include "gamelogic.h"
#include "netclient.h"
#include "gui.h"

static struct option longopts[] = {
#ifdef DEBUG
  { "verbose",      0, 0, 'v' },
  { "debug",        0, 0, 'v' },
#endif
  { "help",         0, 0, 'h' },
  { "client",       0, 0, 'c' },
  { "server",       0, 0, 'S' },
  { "local",        0, 0, 'l' },
  { "ai",           0, 0, 'a' },
  { "host",         1, 0, 's' },
  { "port",         1, 0, 'p' },
  { 0, 0, 0, 0}
};

/*
 * Draw the ncurses window containing the play field
 * name: draw_field
 * @param   none
 * @return  none
 */
void draw_field()
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
  mvwaddch(gamewin, 1, 2+colpos*2, ACS_DIAMOND | COLOR_PAIR(curplayer == 3 ? 0 : curplayer));
  wmove(gamewin, 0, 0);
  wrefresh(gamewin);
}

void draw_msgwin()
{
  unsigned int x, y;
  getmaxyx(stdscr, y, x);
  mvwprintw(msgwin, 0, 1, "Mode: ");
  if(mode > m_local)
    wattrset(msgwin, COLOR_PAIR(ownplayer+2));
  switch(mode)
  {
    case m_local:  waddstr(msgwin, "local");  break;
    case m_client: waddstr(msgwin, "client"); break;
    case m_server: waddstr(msgwin, "server"); break;
    case m_ai:     waddstr(msgwin, "AI");     break;
  }
  wattrset(msgwin, COLOR_PAIR(0));
  mvwaddch(msgwin, 0, 14, ACS_VLINE);
  wprintw(msgwin, " %-*s ", x-15, msg);
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
}

#ifndef _WIN32
/*
 * Get the new terminal size and redraw the whole screen with the new size
 * (POSIX only - at least not Windows)
 * name: resize_handler
 * @param   sig   signal passed by signal()
 * @return  none
 */
void resize_handler(int sig)
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
}
#endif

/*
 * Start another program in a forked process
 * name: fork_process
 * @param   path    path to executable (beginning with ./ normally)
 * @return  1 on error, 0 otherwise
 */
int fork_process(const char *dir, const char *path, const char *argv)
{
#ifdef _WIN32
  char *npath = malloc(strlen(dir)+strlen(path)+strlen(argv)+2);
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  memset(&si, 0, sizeof(si));
  si.cb = sizeof(si);
  memset(&pi, 0, sizeof(pi));
  if(strlen(dir) > 0)
    sprintf(npath, "%s\\%s %s", dir, path, argv);
  else
    sprintf(npath, "%s %s", path, argv);
  if(!CreateProcess(NULL, npath, NULL, NULL, 0, 0, NULL, NULL, &si, &pi))
    return 1;
#else
  char *npath = malloc(strlen(dir)+strlen(path)+2);
  int pid = fork();
  if(pid == 0)
  {
    sprintf(npath, "%s/%s", dir, path);
    execl(npath, path, argv, (char *)NULL);
    perror("exec");
  }
  else if(pid == -1)
    return 1;
#endif
  return 0;
}

/*
 * Print error message if given, usage information for gui call and exit
 * name: usage
 * @param   err       error number defining the type of error
 * @param   program   executable name
 * @return  none, exits
 */
void usage(int err, const char *program)
{
  if(err == 1)
    fputs("You cannot specify more than one mode!\n", stderr);
  else if(err == 2)
    fputs("You can only specify --server in local mode!\n", stderr);
  else if(err == 3)
    fputs("You cannot specify --port in local mode!\n", stderr);
  fprintf(stderr,
          "Usage: %s [options]\n\n"
#ifdef DEBUG
          "  -v, --verbose, --debug   increase output verbosity\n"
          "                           (can be given more than once)\n"
#endif
          "  -c, --client             start in client mode\n"
          "  -S, --server             start in server mode\n"
          "  -l, --local              start in local mode (default)\n"
          "  -a, --ai                 start in human vs. computer mode\n"
          "  -s, --host               specify server to connect to (only for client mode)\n"
          "                           (default: localhost)\n"
          "  -p, --port               specify port to connect to or port to listen\n"
          "                           (only in client and server mode, default: 6666)\n"
          "  -h, --help               show this help\n",
          basename((char *)program));
  exit(err);
}

/*
 * Is called when program is executed.
 * This function includes argument parsing, initialization and main action loop
 * name: main
 * @param   argc    number of arguments
 * @param   argv    array of arguments
 * @return  0, or the world will go under
 */
int main(int argc, char *argv[])
{
  short unsigned int running = 1, input, port = 0;
  int h, w;
  char *server = NULL, c, *argument, *dir;
  extern char *optarg;
  extern int optind, optopt;

  mode = -1;

#ifdef DEBUG
  char *logfilename;
  debug = 0;

  while((c = getopt_long(argc, argv, ":vhcSlas:p:", longopts, NULL)) != -1)
#else
  while((c = getopt_long(argc, argv, ":hcSlas:p:", longopts, NULL)) != -1)
#endif
    // parse command line arguments
    switch(c)
    {
#ifdef DEBUG
      case 'v':
        debug += 1;
        if(logfile == NULL)
        {
          logfilename = malloc(strlen(basename(argv[0]))+5);
          sprintf(logfilename, "%s.log", basename(argv[0]));
          logfile = fopen(logfilename, "a");
        }
        break;
#endif
      case 'h':
        usage(0, argv[0]);
        break;
      case 'c':
        if(mode != -1)
          usage(1, argv[0]);
        mode = m_client;
        break;
      case 'S':
        if(mode != -1)
          usage(1, argv[0]);
        mode = m_server;
        break;
      case 'l':
        if(mode != -1)
          usage(1, argv[0]);
        mode = m_local;
        break;
      case 'a':
        if(mode != -1)
          usage(1, argv[0]);
        mode = m_ai;
        break;
      case 's':
        if(mode != m_client)
          usage(2, argv[0]);
        server = realloc(server, strlen(optarg)+1);
        strcpy(server, optarg);
        break;
      case 'p':
        if(mode == m_local)
          usage(3, argv[0]);
        port = atoi(optarg);
        break;
    }

  if(mode == -1)
    mode = m_local;
  if(mode == m_client && server == NULL)
  {
    server = realloc(server, 10);
    strcpy(server, "localhost");
  }
  if(mode != m_local && port == 0)
    port = 6666;

  // initialize mode
  switch(mode)
  {
    case m_local:
      curplayer = 1;
      break;
    case m_client:
      if(!client_initialize(server, port))
        return -1;
      break;
    case m_server:
      argument = malloc(6);
      sprintf(argument, "%d", port);
      if(fork_process((const char*)dirname(argv[0]), "server", argument))
        return -1;
      sleep(1);
      if(!client_initialize("localhost", port))
        return -1;
      break;
    case m_ai:
      argument = malloc(6);
      dir = dirname(argv[0]);
      sprintf(argument, "%d", port);
      if(fork_process(dir, "server", argument))
        return -1;
      sleep(1);
      if(!client_initialize("localhost", port))
        return -1;
      sprintf(argument, "-p%d", port);
      if(fork_process(dir, "ai", argument))
        return -1;
      break;
  }

  // initialize game grid to zeroes
  memset(field, 0, sizeof(field));

  // initialize ncurses
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  //  colors
  start_color();
  init_color(COLOR_BLUE, 0, 0, 999);
  init_color(COLOR_RED, 999, 0, 0);
  init_pair(0, COLOR_WHITE, COLOR_BLACK);
  init_pair(1, COLOR_BLUE, COLOR_BLACK);
  init_pair(2, COLOR_RED, COLOR_BLACK);
  init_pair(3, COLOR_WHITE, COLOR_BLUE);
  init_pair(4, COLOR_BLACK, COLOR_RED);
  //  windows
  getmaxyx(stdscr, h, w);
  msgwin = newwin(2, w, h - 2, 0);
  gamewin = newwin(h - 2, w, 0, 0);
#ifndef _WIN32
  signal(SIGWINCH, resize_handler);
#endif
  refresh();

  msg = "Welcome to Connect Four!";
  draw_field();
  draw_msgwin();

  // main loop
  while(running == 1)
  {
    if(mode != m_local && curplayer != 3)
    {
      if(client_available())
      {
        if(!client_exec_cmd())
        {
          curplayer = 3;
          msg = "Opponent disconnected.";
          client_quit();
          cbreak();
          continue;
        }
        cbreak();
      }
    }

    // get user's keypress
    if(mode != m_local && halfdelay(1) == ERR)
      continue;
    input = getch();
    switch(input)
    {
      case 'q':
        msg = "To quit, press capital Q.";
        break;
      case 'Q':
        // quit
        running = 0;
        if(mode != m_local && curplayer != 3)
          client_quit();
        break;
      case 'r':
        if(!get_winner() && mode == m_local)
        {
          msg = "To restart, press capital R.";
          break;
        }
      case 'R':
        if(mode != m_local)
        {
          msg = "You cannot restart a network game!";
          break;
        }
        memset(field, 0, sizeof(field));
        curplayer = 1;
        colpos = 0;
        msg = "Welcome to Connect Four!";
        break;
      case KEY_LEFT:
        if(get_winner() || curplayer == 3)
          break;
        if(colpos == 0)
          colpos = 6;
        else
          colpos--;
        break;
      case KEY_RIGHT:
        if(get_winner() || curplayer == 3)
          break;
        if(colpos == 6)
          colpos = 0;
        else
          colpos++;
        break;
      case KEY_DOWN:
      case ' ':
        if(get_winner() || (mode != m_local && curplayer != ownplayer))
          break;
        if(mode == m_local)
          local_set_stone(colpos);
        else if(mode != m_local)
          client_set_stone(colpos);
        break;
    }

    // check for winner or draw and show message
    switch(get_winner())
    {
      case -1:
        if(mode == m_local)
          msg = "DRAW! Press R to restart or Q to quit";
        else
          msg = "DRAW! Press Q to quit";
        break;
      case 1:
        if(mode == m_local)
          msg = "BLUE WINS! Press R to restart or Q to quit";
        else
          msg = "BLUE WINS! Press Q to quit";
        break;
      case 2:
        if(mode == m_local)
          msg = "RED WINS! Press R to restart or Q to quit";
        else
          msg = "RED WINS! Press Q to quit";
        break;
    }

    // update screen
    draw_field();
    draw_msgwin();
  }
  endwin();
  return 0;
}
