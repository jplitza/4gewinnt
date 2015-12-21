#ifndef AI_H
#define AI_H

#ifdef DEBUG
unsigned short int debug;
FILE *logfile;
#endif

int ai_check_array( short unsigned int[] );
int ai_check( short unsigned int, short unsigned int );
int ai_set_stone( unsigned short int );
int ai_turn( void );
void usage( const char * );
int main( int, char *[] );
#endif
