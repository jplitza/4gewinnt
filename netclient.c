#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <string.h>
#include <ctype.h>
#ifdef _WIN32
#include <windows.h>
#include <winsock.h>
#else
#include <sys/socket.h>
#include <netdb.h>
#endif

#include "net.h"
#include "gamelogic.h"
#include "netclient.h"

/*
 * Send RESYNC command to server
 * name: client_resync
 * @param   none
 * @return  none
 */
void client_resync()
{
  sendline(sock, "RESYNC");
}

/*
 * Parse the argument string sent by the server along with a RESYNC command
 * name: client_resync_handler
 * @param   arg   argument string sent by server
 * @return  0 on error in parsing, 1 otherwise
 */
int client_resync_handler(const char *arg)
{
  short unsigned int x, y, tfield[10][9];
  if(strlen(arg) < 44)
  {
#ifdef DEBUG
    if(debug >= 1)
      fprintf(logfile, "Got unexpected response from server after RESYNC (too few chars): %s", arg);
#endif
    client_resync();
    return 0;
  }
  for(x = 0; x < 7; x++)
    for(y = 0; y < 6; y++)
    {
      if(isdigit(arg[x*6+y]))
        tfield[x][y] = arg[x*6+y] - '0';
      else
      {
#ifdef DEBUG
        if(debug >= 1)
          fprintf(logfile, "Got unexpected response from server after RESYNC (not a number): %s", arg);
#endif
        client_resync();
        return 0;
      }
    }
  if(isdigit(arg[6*7]) && arg[6*7] != '0' && isdigit(arg[6*7+1]) && arg[6*7+1] != '0')
  {
    curplayer = arg[6*7]-'0';
    ownplayer = arg[6*7+1]-'0';
    memcpy(*field, *tfield, sizeof(tfield));
    return 1;
  }
  else
  {
#ifdef DEBUG
    if(debug >= 1)
      fprintf(logfile, "Got unexpected response from server after RESYNC phase 2: %s\n", arg);
#endif
    return 0;
  }
}

/*
 * Parse the argument string sent by the server along with a SET command and
 * modify the local field array accordingly. If irregularities are detected
 * (differing local field array and server field array), a RESYNC request is
 * sent automatically.
 * name: client_get_move
 * @param   arg   argument string sent by server
 * @return  0 on error in parsing, 1 otherwise
 */
int client_get_move(const char *arg)
{
  short unsigned int x, y, player;

  if(isdigit(arg[0]) && isdigit(arg[1]) && isdigit(arg[2]) && arg[2] != '0')
  {
    x = arg[0]-'0';
    y = arg[1]-'0';
    player = arg[2]-'0';
    if(get_stone(x) != y || player != curplayer)
    {
#ifdef DEBUG
      if(debug >= 1)
        fprintf(logfile, "Somethings wrong: x=%d, y=%d, get_stone(x)=%d, player=%d, curplayer=%d\n", x, y, get_stone(x), player, curplayer);
#endif
      client_resync();
    }
    else
      local_set_stone(x);
    return 1;
  }
  else
  {
#ifdef DEBUG
    if(debug >= 1)
      fprintf(logfile, "Got unexpected response from server after GET: %s\n", arg);
#endif
    client_resync();
    return 0;
  }
}

/*
 * Send placment request to server and modify local field array accordingly
 * on acknowledgement. Also requests resync if server denies or sends an
 * unexpected row number
 * name: client_set_stone
 * @param   col   column number of desired placement
 * @return  1 on acknowledgement of server, 0 otherwise
 */
int client_set_stone(unsigned short int col)
{
  if(get_stone(col) == -1)
    return 0;
  char *msg = malloc(6), command[bufsize], arg[bufsize];
  sprintf(msg, "SET %d%d%d", col, get_stone(col), ownplayer);
  if(!sendline(sock, msg))
    return -1;
  free(msg);
  recvline(sock, command, arg);
  if(!strcmp(command, "OK") && isdigit(arg[0]))
  {
    // it is more or less redundant to check this, as the server should do all
    // necessary checks. But... make assurence double sure :-)
    if(arg[0] - '0' != col || arg[1] - '0' != get_stone(col) || arg[2] - '0' != ownplayer)
      client_resync();
    else
      local_set_stone(col);
    return 1;
  }
  else if(!strcmp(command, "FAIL"))
  {
#ifdef DEBUG
    if(debug >= 1)
      fprintf(logfile, "Server rejected my placement: %s\n", arg);
#endif
  }
  else
  {
#ifdef DEBUG
    if(debug >= 1)
      fprintf(logfile, "Got unexpected response from server after SET: %s %s\n", command, arg);
#endif
  }
  // we don't need to RESYNC here because server automaticalle resyncs if
  // invalid placement is sent.
  return 0;
}

/*
 * read a line from server socket and execute received command if known
 * name: client_exec_cmd
 * @param   none
 * @return  0 on read error or QUIT command, 1 otherwise
 */
int client_exec_cmd()
{
  char command[1024], arg[1024];
  if(!recvline(sock, command, arg))
    return 0;
#ifdef DEBUG
  if(debug >= 1)
    fprintf(logfile, "Got command from server: %s %s\n", command, arg);
#endif

  if(!strcmp(command, "SET"))
    client_get_move(arg);
  else if(!strcmp(command, "RESYNC"))
    client_resync_handler(arg);
  else if(!strcmp(command, "QUIT"))
    return 0;
  return 1;
}

/*
 * Check if data can be read from server socket
 * name: client_available
 * @param   none
 * @return  1 if data is available, 0 otherwise
 */
int client_available(void)
{
  fd_set rfds;
  struct timeval tv;
  FD_ZERO(&rfds);
  FD_SET(sock, &rfds);
  tv.tv_sec = 0;
  tv.tv_usec = 1;
  if(select(sock+1, &rfds, NULL, NULL, &tv))
    return 1;
  return 0;
}

/*
 * establish connection to desired server and set game variables to a sane
 * default
 * name: client_initialize
 * @param   server    server address (either as IP address or as hostname)
 * @param   port      server port
 * @return  0 on error, 1 otherwise
 */
int client_initialize(const char *server, unsigned short int port)
{
  struct hostent *he;
  struct sockaddr_in address;

  bufsize = 64;
  buffer = malloc(bufsize*sizeof(char));

#ifdef _WIN32
  WSADATA wsaData;
  if (WSAStartup (MAKEWORD(1, 1), &wsaData) != 0) {
    fprintf (stderr, "WSAStartup(): Couldn't initialize WinSock.\n");
      exit (EXIT_FAILURE);
  }
#endif
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if(sock == -1)
  {
    perror("socket");
    return 0;
  }

  if((he = gethostbyname(server)) == NULL)
  {
    perror("Error resolving hostname");
    return 0;
  }

  memcpy(&address.sin_addr, he->h_addr_list[0], he->h_length);
  address.sin_family = AF_INET;
  address.sin_port = htons(port);

  if(connect(sock, (struct sockaddr *) &address, sizeof(address)))
  {
    perror("Error connecting");
    return 0;
  }

  curplayer = 0;
  ownplayer = 1;

  return 1;
}

/*
 * Close server socket
 * name: client_quit
 * @param   none
 * @return  none
 */
void client_quit( void )
{
  sendline(sock, "QUIT");
  close(sock);
}
