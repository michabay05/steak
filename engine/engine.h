#ifndef _ENGINE_H_
#define _ENGINE_H_

#include "board.h"

typedef struct {
    Board board;

    bool quit;
    int depth, time, inc, movetime, movestogo;
    int starttime, stoptime;
    bool timeset, stopped;
} UCI_Info;

extern UCI_Info U_INFO;

#define MAX_PLY 64

int evaluate(Board *board);
void search_position(Board *board, int depth);

#endif // _ENGINE_H_
