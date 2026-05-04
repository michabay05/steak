#ifndef _BOARD_H_
#define _BOARD_H_

#include "bitboard.h"
#include "fen.h"

typedef struct {
    Bitboard piece[12];
    Bitboard units[2];
    Bitboard all_units;

    Color side;
    Sq enpassant;
    u16 half_moves;
    u16 full_moves;
    u8 castling;
} Board;

extern const int castling_rights[64];

void board_add_piece(Board *board, Piece piece, Sq sq);
void board_remove_piece(Board *board, Piece piece, Sq sq);
Piece board_get_piece(Board *board, Sq sq);
void board_update_units(Board *board);
void board_change_side(Board *board);
void board_set_from_fen(Board *board, FENInfo fen);
void board_print(Board *b);
bool board_is_sq_attacked(Board *b, Sq sq, Color side);
bool board_is_in_check(Board *b);

#endif //  _BOARD_H_
