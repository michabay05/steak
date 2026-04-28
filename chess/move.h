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
    Piece piece : 4;
    Piece promoted : 4;
    MoveFlags flag : 3;
} Move;

Move move_encode(Sq source, Sq target, Piece piece, Piece promoted,
    MoveFlags flag);
bool move_eq(Move a, Move b);
void move_to_str(Move move, char *move_str);
Move move_parse(char *move_str, Piece piece, MoveFlags flag);
bool move_make(Board *main, Move move, MoveType move_flag);

#endif // _MOVE_H_
