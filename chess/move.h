#ifndef _MOVE_H_
#define _MOVE_H_

#include "board.h"
#include "defs.h"

typedef enum {
    AllMoves,
    CapturesOnly,
} MoveType;

typedef ENUM_DEF(u8, MoveFlags) {
    MVF_Quiet,
    MVF_Capture,
    MVF_TwoSquarePush,
    MVF_Enpassant,
    MVF_Castling,
};

typedef struct {
    Sq source : 7;
    Sq target : 7;
    PieceType promoted : 3;
    MoveFlags flag : 3;
} Move;

Move move_encode(Sq source, Sq target, PieceType promoted, MoveFlags flag);
bool move_eq(Move a, Move b);
void move_to_str(Move move, char *move_str);
Move move_parse_cstr(char *move_str);
Move move_parse_sv(String_View msv);
bool move_make(Board *main, Move move, MoveType move_flag);

#endif // _MOVE_H_
