#ifndef _FEN_H_
#define _FEN_H_

#include "defs.h"

typedef struct {
    u16 full_moves;
    u16 half_moves;
    Piece board[64];
    Sq enpassant;
    Color side;
    u8 castling;
} FENInfo;

FENInfo parse_fen_cstr(const char *fen);
FENInfo parse_fen_sv(String_View fen);
void fen_info_print(FENInfo *fen);

#endif // _FEN_H_
