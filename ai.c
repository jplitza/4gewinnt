#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#ifndef _WIN32
#include <libgen.h>
#else
#include "basename.h"
#endif

#include "gamelogic.h"
#include "netclient.h"
#include "ai.h"

static struct option longopts[] = {
#ifdef DEBUG
  { "debug",    0, 0, 'v' },
  { "verbose",  0, 0, 'v' },
#endif
  { "host",     1, 0, 's' },
  { "server",   1, 0, 's' },
  { "port",     1, 0, 'p' },
  { "help",     0, 0, 'h' },
  { "output",   0, 0, 'o' },
  { 0, 0, 0, 0 }
};

/*
 * Check array a for two criteria:
 *  - contains exactly one zero
 *  - all non-zero elements have the same value
 * name: ai_check_array
 * @param   a   array of numbers to be checked
 * @return  index number of zero on success, -1 otherwise
 */
int ai_check_array(short unsigned int a[])
{
  unsigned short int i, first = 0;
  short int gotzero = -1;
  for(i = 0; i < sizeof(a); i++)
  {
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
}

/*
 * Check a zero-field for possible completion of a foursome
 * name: ai_check
 * @param   x, y    coordinates of field
 * @return  1 on success, 0 otherwise
 */
int ai_check(short unsigned int x, short unsigned int y)
{
  unsigned short int array[4];
  short int find, i;
  if(field[x][y] != 0)
    return 0;
  for(i = 0; i < 4; i++)
  {
    // horizontal match
    if(x-i < 7 - 3 && x-i >= 0)
    {
      array[0] = field[x-i][y];
      array[1] = field[x-i+1][y];
      array[2] = field[x-i+2][y];
      array[3] = field[x-i+3][y];
      find = ai_check_array(array);
      if(find > -1)
        return 1;
    }

    // vertical match
    if(y < 6 - 3 &&
       field[x][y+1] == field[x][y+2] &&
       field[x][y+1] == field[x][y+3])
      return 1;

    // bottom-right diagonal match
    if(x-i < 7 - 3 && y-i < 6 - 3)
    {
      array[0] = field[x-i][y-i];
      array[1] = field[x-i+1][y-i+1];
      array[2] = field[x-i+2][y-i+2];
      array[3] = field[x-i+3][y-i+3];
      find = ai_check_array(array);
      if(find > -1)
        return 1;
    }

    // bottom-left diagonal match
    if(x+i > 2 && y-i < 6 - 3)
    {
      array[0] = field[x+i][y-i];
      array[1] = field[x+i-1][y-i+1];
      array[2] = field[x+i-2][y-i+2];
      array[3] = field[x+i-3][y-i+3];
      find = ai_check_array(array);
      if(find > -1)
        return 1;
    }
  }
  return 0;
}

/*
 * Place a stone after checking it doesn't let the opponent win
 * name: ai_set_stone
 * @param   x   column number of desired position
 * @return  1 if successfully placed, 0 otherwise
 */
int ai_set_stone(unsigned short int x)
{
  if(!ai_check(x, get_stone(x)-1))
    return client_set_stone(x);
  else
  {
#ifdef DEBUG
    if(debug >= 1)
      fprintf(logfile, "prevented %d.%d\n", x, get_stone(x));
#endif
    return 0;
  }
}

/*
 * Place a stone at the "best" position calculated
 * name: ai_turn
 * @param   none
 * @return  -1 if an unexpected error occured, 1 on success, 0 otherwise
 */
int ai_turn()
{
  short unsigned int x, i;
  short int temp;
  // this loop checks if there are 3 stones in a row
  // if there are, it is the most wise to complete it - either blocking
  // the enemy from getting 4-in-a-row or getting it myself
  for(x = 0; x < 7; x++)
  {
    temp = ai_check(x, get_stone(x));
    if(temp)
    {
#ifdef DEBUG
      if(debug >= 1)
        fprintf(logfile, "need to place %d.%d\n", temp, get_stone(temp));
#endif
      temp = ai_set_stone(x);
      if(temp == -1)
        return -1;
      else if(temp)
        return 1;
    }
  }

  // set stone next to enemy stone on bottom row (prevent 4-move-win)
  for(x = 0; x < 7; x++)
  {
    if(field[x][5] == 0 &&
       ((x > 0 && field[x-1][5] == (curplayer ^ 3)) ||
       (x < 6 && field[x+1][5] == (curplayer ^ 3))))
    {
#ifdef DEBUG
      if(debug >= 2)
        fprintf(logfile, "placing %d, next to enemy\n", x);
#endif
      if(ai_set_stone(x))
        return 0;
    }
  }

  // no definite field could be found, randomly choose a column ;)
  srand((unsigned) time(NULL));
  for(i = 0; 1; i++)
  {
#ifdef DEBUG
    if(debug >= 3)
      fprintf(logfile, "placing randomly...\n");
#endif
    if(i >= 10)
      temp = client_set_stone(rand() % 7);
    else
      temp = ai_set_stone(rand() % 7);
    if(temp == -1)
      return -1;
    else if(temp)
      return 1;
  }
  return 0;
}

/*
 * Print usage information for ai call and exit
 * name: usage
 * @param   program   executable name
 * @return  none, exits
 */
void usage(const char *program)
{
  fprintf(stderr,
          "Usage: %s [options]\n\n"
#ifdef DEBUG
          "  -v, --verbose, --debug         increase output verbosity\n"
          "                                 (can be given more than once)\n"
#endif
          "  -s, --server, --host <server>  specify server to connect to\n"
          "                                 (default: localhost)\n"
          "  -p, --port <port>              specify port to connect to\n"
          "                                 (default: 6666)\n"
          "  -o, --output                   print game grid after game\n"
          "                                 (useful in AI vs AI games)\n"
          "  -h, --help                     show this help\n",
          basename((char *)program));
  exit(0);
}

/*
 * Is called when program is executed.
 * This function includes argument parsing, initialization and main action loop
 * name: main
 * @param   argc    number of arguments
 * @param   argv    array of arguments
 * @return  -1 on error
 *           0 on win
 *           1 on draw
 *           2 on loose
 */
int main(int argc, char *argv[])
{
  unsigned short int running = 1, port = 0, oag, x, y;
  char *server = malloc(10), c;
  extern char *optarg;
  extern int optind, optopt;
  memset(server, 0, sizeof(char)*10);
#ifdef DEBUG
  char *logfilename;

  while((c = getopt_long(argc, argv, ":vs:p:oh", longopts, NULL)) != -1)
#else
  while((c = getopt_long(argc, argv, ":s:p:oh", longopts, NULL)) != -1)
#endif
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
          free(logfilename);
        }
        break;
#endif
      case 's':
        if(strlen(optarg) > 9)
          server = realloc(server, strlen(optarg)+1);
        strcpy(server, optarg);
        break;
      case 'p':
        port = atoi(optarg);
        break;
      case 'o':
        oag = 1;
        break;
      case 'h':
        usage(argv[0]);
        break;
    }

  if(server == NULL || strlen(server) == 0)
    strcpy(server, "localhost");

  if(port == 0)
    port = 6666;

  memset(field, 0, sizeof(field));

  if(!client_initialize(server, port))
    return 2;

  while(running == 1)
  {
    if(curplayer == ownplayer)
    {
      if(ai_turn() == -1)
        running = 0;
    }
    else
    {
      if(!client_exec_cmd())
        running = 0;
    }
    if(get_winner())
      running = 0;
  }

  if(oag == 1)
  {
    for(y = 0; y < 6; y++)
    {
      for(x = 0; x < 7; x++)
        printf("%d ", field[x][y]);
      printf("\n");
    }
  }

  if(get_winner() == 0)
    return -1;
  if(get_winner() == ownplayer)
    return 0;
  if(get_winner() == -1)
    return 1;
  return 2;
}
