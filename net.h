#ifndef NET_H
#define NET_H

#ifdef DEBUG
unsigned short int debug;
FILE *logfile;
#endif

int bufsize;
char *buffer;

int recvline( int, char *, char * );
int sendline( int, const char * );
#endif
