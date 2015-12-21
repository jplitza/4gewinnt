#ifndef GAMELOGIC_H
#define GAMELOGIC_H

short unsigned int field[10][9], curplayer;

int get_stone( unsigned short int );
int local_set_stone( unsigned short int );
short int get_winner( void );
#endif
