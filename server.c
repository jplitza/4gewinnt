#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#ifdef _WIN32
#include <windows.h>
#include <winsock.h>
#include <winsock2.h>
#include "basename.h"
#else
#include <libgen.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#endif
#include "net.h"
#include "gamelogic.h"
#include "server.h"

/*
 * Sends the RESYNC command to the client, including argument string giving
 * complete game state
 * name: server_resync
 * @param   n   number identifying client (0 or 1)
 * @return  1 on error (connection closed), 0 otherwise
 */
int server_resync(unsigned short int n)
{
  short unsigned int x, y;
  char *buf = malloc(52);;
  memset(buf, 0, 52);
  strcpy(buf, "RESYNC ");
  for(x = 0; x < 7; x++)
    for(y = 0; y < 6; y++)
      *(buf + strlen(buf)) = field[x][y] + '0';
  sprintf(buf, "%s%d%d", buf, curplayer, n+1);
  if(sendline(clientsock[n], buf))
  {
    free(buf);
    return 0;
  }
  free(buf);
  return 1;
}

/*
 * Parse the argument string sent by a client along with a SET command,
 * validate it, respond accordingly and notify other client of change if
 * neccessary.
 * name: server_set_stone
 * @param   arg   argument string
 * @param   n     number identifying client (0 or 1)
 * @return  -1 if it's not the client's turn
 *          -2 if the argument string is invalid
 *          -3 if the client's desired placement is invalid
 *           0 otherwise
 */
int server_set_stone(const char *arg, unsigned short int n)
{
  unsigned short int col, row, player;
  char sender[8];

  if(curplayer != n + 1)
  {
    // it isn't the player's turn
    sendline(clientsock[n], "FAIL Not your turn");
    server_resync(n);
    return -1;
  }

  if(!isdigit(*arg) || !isdigit(arg[1]) || !isdigit(arg[2]))
  {
    // got invalid argument from client
    sendline(clientsock[n], "FAIL Invalid argument");
    return -2;
  }

  col = arg[0] - '0';
  row = arg[1] - '0';
  player = arg[2] - '0';
  if(get_stone(col) != row || player != n+1)
  {
    // got invalid placement from client
    sendline(clientsock[n], "FAIL Invalid placement");
    server_resync(n);
    return -3;
  }

  sprintf(sender, "OK %d%d%d", col, get_stone(col), n+1);
  sendline(clientsock[n], sender);

  sprintf(sender, "SET %d%d%d", col, get_stone(col), n+1);
  sendline(clientsock[n^1], sender);
  local_set_stone(col);
  return 0;
}

/*
 * Is called when program is executed.
 * This function includes argument parsing, initialization and main action loop
 * name: main
 * @param   argc    number of arguments
 * @param   argv    array of arguments
 * @return  0 on error, 1 otherwise
 */
int main(int argc, char *argv[])
{
  struct sockaddr_in address;
  unsigned short int i, port = 0;
  unsigned int addrlen;
  fd_set rfds;
  char *command, *arg;

  if(argc > 1)
  {
    for(i = 1; i < argc; i++)
    {
#ifdef DEBUG
      if(!strcmp(argv[i], "-v"))
      {
        debug += 1;
        continue;
      }
#endif
      if(!isdigit(*argv[i]))
      {
#ifdef DEBUG
        fprintf(stderr, "Usage: %s [-v] [port]\n",
#else
        fprintf(stderr, "Usage: %s [port]\n",
#endif
            basename(argv[0]));
        return 0;
      }
      port = (unsigned short int)atoi(argv[i]);
    }
  }

  if(port == 0)
    port = 6666;

#ifdef DEBUG
  char *logfilename;
  if(debug > 0)
  {
    logfilename = malloc(strlen(basename(argv[0]))+5);
    sprintf(logfilename, "%s.log", basename(argv[0]));
    logfile = fopen(logfilename, "a");
    free(logfilename);
  }
#endif

  bufsize = 64;
  buffer = malloc(bufsize*sizeof(char));

  clientsock[0] = -1;
  clientsock[1] = -1;

#ifdef _WIN32
  WSADATA wsaData;
  if (WSAStartup (MAKEWORD(1, 1), &wsaData) != 0) {
    fprintf (stderr, "WSAStartup(): Couldn't initialize WinSock.\n");
      exit (EXIT_FAILURE);
  }
#endif
  servsock = socket(AF_INET, SOCK_STREAM, 0);
  if(servsock == -1)
  {
    perror("socket");
    return 0;
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);

  if(bind(servsock, (struct sockaddr *)&address, sizeof(address)))
  {
    perror("bind");
    return 0;
  }

  if(listen(servsock, 2))
  {
    perror("listen");
    return 0;
  }

  curplayer = 1;

  command = malloc(bufsize*sizeof(char));
  arg = malloc(bufsize*sizeof(char));
  addrlen = sizeof(struct sockaddr_in);

  FD_ZERO(&rfds);
  FD_SET(servsock, &rfds);
  
  while(select(((clientsock[0] == -1) ? servsock + 1 : ((clientsock[1] == -1) ? clientsock[0] + 1 : clientsock[1] + 1)), &rfds, NULL, NULL, NULL))
  {
    if(FD_ISSET(servsock, &rfds))
    {
      // accept new connect
      i = (clientsock[0] == -1) ? 0 : 1;
      clientsock[i] = accept(servsock, (struct sockaddr *)&address, &addrlen);
      server_resync(i);
      FD_ZERO(&rfds);
      if(i == 1)
      {
        close(servsock);
        FD_SET(clientsock[0], &rfds);
      }
      else
        FD_SET(servsock, &rfds);
      FD_SET(clientsock[i], &rfds);
      continue;
    }
    for(i = 0; i < 2; i++)
      if(FD_ISSET(clientsock[i], &rfds))
        break;

    if(!recvline(clientsock[i], command, arg))
    {
      server_quit();
      break;
    }
#ifdef DEBUG
    if(debug >= 1)
      fprintf(logfile, "Got command from client #%d: %s %s\n", i, command, arg);
#endif

    if(!strcmp(command, "RESYNC"))
      server_resync(i);
    else if(!strcmp(command, "SET"))
      server_set_stone(arg, i);
    else if(!strcmp(command, "QUIT"))
    {
      server_quit();
      break;
    }
    FD_ZERO(&rfds);
    FD_SET(clientsock[0], &rfds);
    if(clientsock[1] == -1)
      FD_SET(servsock, &rfds);
    else
      FD_SET(clientsock[1], &rfds);
  }
  return 1;
}

/*
 * Send both clients QUIT command and close all connections afterwards
 * name: server_quit
 * @param   none
 * @return  none
 */
void server_quit( void )
{
  sendline(clientsock[0], "QUIT");
  sendline(clientsock[1], "QUIT");
  close(clientsock[0]);
  close(clientsock[1]);
  close(servsock);
}
