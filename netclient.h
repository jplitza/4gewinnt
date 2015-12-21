#ifndef NETCLIENT_H
#define NETCLIENT_H

#ifdef DEBUG
unsigned short int debug;
FILE *logfile;
#endif

short unsigned int ownplayer;
int sock, bufsize;
char *buffer;

void client_resync( void );
int client_resync_handler( const char * );
int client_get_move( const char * );
int client_set_stone( unsigned short int );
int client_exec_cmd( void );
int client_available( void );
int client_initialize( const char *, unsigned short int );
void client_quit( void );
#endif
