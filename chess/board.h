#ifndef _BOARD_H_
#define _BOARD_H_

#include "bitboard.h"
#include "zobrist.h"

typedef struct board {
    Bitboard piece[2][6];
    Bitboard units[2];
    Bitboard all_units;

    Color side;
    Sq enpassant;
    u16 half_moves;
    u16 full_moves;
    u8 castling;

    ZB_Key key;
} Board;

extern const int castling_rights[64];

Piece board_get_piece(Board *board, Sq sq);
void board_update_units(Board *board);
void board_change_side(Board *board);
void board_print(Board *b);
bool board_is_sq_attacked(Board *b, Sq sq, Color side);
bool board_is_in_check(Board *b);

void board_parse_fen_cstr(Board *board, const char *fen);
void board_parse_fen_sv(Board *board, String_View fen);
void board_fen_generate(Board *board, String_Builder *sb);

#endif //  _BOARD_H_
