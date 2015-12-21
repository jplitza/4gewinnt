#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#ifdef _WIN32
#include <windows.h>
#include <winsock.h>
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#endif
#include "net.h"

/*
 * Read a full line (terminated by \n) from socket sock, divide it into
 * command and argument and store it accordingly in pointers command and arg
 * name: recvline
 * @param   sock      socket from which the line shall be read
 * @param   command   pointer to memory area where command shall be stored
 * @param   arg       pointer to memory area where argument shall be stored
 *                    May be NULL, in which case no split in command and
 *                    argument is perfomed.
 * @return  0 on error (connection closed), 1 otherwise
 */
int recvline(int sock, char *command, char *arg)
{
  int readlength;

  memset(buffer, 0, bufsize);

  while((readlength = recv(sock, buffer+strlen(buffer), 1, 0)) == 1 && buffer[strlen(buffer) - 1] != '\n' && strlen(buffer) < bufsize);

  if(readlength == 0 || readlength == -1)
  {
    // Connection closed
    return 0;
  }

  // strip newline
  *(buffer + strlen(buffer) - 1) = '\0';

  // split command and argument
  if(strchr(buffer, ' ') != NULL && arg != NULL)
  {
    strcpy(arg, strchr(buffer, ' ')+1);
    *(strchr(buffer, ' ')) = '\0';
  }
  else
    strcpy(arg, "");
  strcpy(command, buffer);
#ifdef DEBUG
  if(debug >= 3)
    fprintf(logfile, "Received: %s %s\n", command, arg);
#endif
  return 1;
}

/*
 * Send a full line (terminated by \n, which is added automatically) to the
 * socket sock
 * name: sendline
 * @param   sock    socket to which shall be written
 * @param   str     string that shall be written WITHOUT terminating \n
 * @return  0 on error (connection closed), 1 otherwise
 */
int sendline(int sock, const char *str)
{
  char *buf;
  buf = malloc(strlen(str)+2);
  memset(buf, 0, strlen(str)+2);
  strcpy(buf, str);
  *(buf + strlen(buf)) = '\n';
#ifdef DEBUG
  if(debug >= 3)
    fprintf(logfile, "Sending: %s", buf);
#endif
  if(send(sock, buf, strlen(buf), 0) != strlen(buf))
  {
    // Connection closed
    free(buf);
    return 0;
  }
  free(buf);
  return 1;
}
