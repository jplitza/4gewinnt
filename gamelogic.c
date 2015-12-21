#include "gamelogic.h"

/*
 * Get y coordinate of the next stone that would be thrown into column col
 * name: get_stone
 * @param   col   column number to use
 * @return  -1 if col is invalid or column number col is already full
 *           y coordinate of next stone otherwise
 */
int get_stone(short unsigned int col)
{
  short unsigned int y;

  if(col > 6 || field[col][0] != 0)
    return -1;

  for(y = 0; y < 6; y++)
    if(field[col][y] != 0)
      return y - 1;
  return 5;
}

/*
 * "Throw" a current player's stone into the column col of the local field
 * Modifies field array directly!
 * name: local_set_stone
 * @param   col   desired column number
 * @return  1 on success, 0 if column is already full or col is invalid
 */
int local_set_stone(short unsigned int col)
{
  if(get_stone(col) == -1)
    return 0;
  field[col][get_stone(col)] = curplayer;
  curplayer ^= 3;
  return 1;
}

/*
 * Determine if there is a winner, i.e. if there are four stones of one value
 * in a row (horizontal, vertical or diagonal)
 * Also, check if field is full with no winner (draw)
 * name: get_winner
 * @param   none
 * @return  player number of winner if available
 *          -1 on draw
 *           0 otherwise (game is still running)
 */
short int get_winner()
{
  short unsigned int x, y, temp;
  for(x = 0; x < 7; x++)
    for(y = 0; y < 6; y++)
    {
      temp = field[x][y];
      if(temp == 0)
        continue;

      // horizontal match
      if(x < 7 - 3 &&
         temp == field[x+1][y] &&
         temp == field[x+2][y] &&
         temp == field[x+3][y])
        return temp;

      // vertical match
      if(y < 6 - 3 &&
         temp == field[x][y+1] &&
         temp == field[x][y+2] &&
         temp == field[x][y+3])
        return temp;

      // bottom-right diagonal match
      if(x < 7 - 3 &&
         y < 6 - 3 &&
         temp == field[x+1][y+1] &&
         temp == field[x+2][y+2] &&
         temp == field[x+3][y+3])
        return temp;

      // bottom-left diagonal match
      if(x > 2 &&
         y < 6 - 3 &&
         temp == field[x-1][y+1] &&
         temp == field[x-2][y+2] &&
         temp == field[x-3][y+3])
        return temp;
    }
  for(x = 0; x < 7; x++)
  {
    if(field[x][0] == 0)
      break;
    else if(x == 6)
      return -1;
  }
  return 0;
}
