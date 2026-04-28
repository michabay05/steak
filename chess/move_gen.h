#ifndef _MOVE_GEN_H_
#define _MOVE_GEN_H_

#include "move.h"

#define MOVE_GEN_MAX 100
typedef struct {
    Move list[MOVE_GEN_MAX];
    int count;
} MoveList;

void movelist_add(MoveList *ml, Move move);
int movelist_search(MoveList ml, Sq source, Sq target, Piece promoted);
void movelist_print_list(MoveList ml);

void movelist_generate_all(MoveList *ml, Board *b);
void movelist_generate(MoveList *ml, Board *b, Piece p);
void movelist_legal(MoveList *ml, Board *b);

#endif // _MOVE_GEN_H_
