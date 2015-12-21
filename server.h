#ifndef SERVER_H
#define SERVER_H

#ifdef DEBUG
#include <stdio.h>
unsigned short int debug;
FILE *logfile;
#endif

int servsock, clientsock[2], bufsize;
char *buffer;

int server_resync( unsigned short int );
int server_set_stone( const char *, unsigned short int );
int main( int, char*[] );
void server_quit( void );
#endif
