#ifndef GUI_H
#define GUI_H

#ifdef DEBUG
unsigned short int debug;
FILE *logfile;
#endif

short unsigned int colpos, curplayer;
short int mode;
char *msg;

enum mode { m_local = 0, m_client, m_server, m_ai };

WINDOW *gamewin, *msgwin;

void draw_field( void );
void draw_msgwin( void );

#ifndef WIN32
void resize_handler( int );
#endif

int fork_process( const char*, const char*, const char* );
void usage( int, const char* );

int main( int, char *[] );

#endif
