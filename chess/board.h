#ifndef _BOARD_H_
#define _BOARD_H_

#include "bitboard.h"
#include "fen.h"

typedef struct {
    Bitboard piece[12];
    Bitboard units[2];
    Bitboard all_units;
} Position;

typedef struct {
    Color side;
    Sq enpassant;
    u16 half_moves;
    u16 full_moves;
    u8 castling;
} State;

typedef struct {
    Position pos;
    State state;
} Board;

extern const int castling_rights[64];

void pos_add_piece(Position *pos, Piece piece, Sq sq);
void pos_remove_piece(Position *pos, Piece piece, Sq sq);
Piece pos_get_piece(Position pos, Sq sq);
void pos_update_units(Position *pos);
void state_change_side(State *state);
void board_set_from_fen(Board *board, FENInfo fen);
void board_print(Board *b);
bool board_is_sq_attacked(Board *b, Sq sq, Color side);
bool board_is_in_check(Board *b);

#endif //  _BOARD_H_
