#ifndef _FEN_H_
#define _FEN_H_

#include "defs.h"

typedef struct
{
    Piece board[64];
    int side;
    int castling;
    int full_moves;
    int enpassant;
    int half_moves;
} FENInfo;

FENInfo parse_fen(const char *fen);
void fen_info_print(FENInfo *fen);

#endif // _FEN_H_
