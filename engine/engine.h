#ifndef _ENGINE_H_
#define _ENGINE_H_

#include "../chess/chess.h"

int evaluate(Board *board);
void search_position(Board *board, int depth);

#endif // _ENGINE_H_
