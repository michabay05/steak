#ifndef _ZOBRIST_H_
#define _ZOBRIST_H_

#include "defs.h"
typedef struct board Board;

// "Almost" unique position identifier (could have collisions)
typedef u64 ZB_Key;

void zobrist_init(void);
ZB_Key zobrist_gen_key(Board *board);

void zobrist_toggle_side(Board *board);
void zobrist_toggle_piece(Board *board, Piece piece, Sq sq);
void zobrist_update_enpassant(Board *board, Sq enpass_sq);
void zobrist_update_castling(Board *board, CastlingRight cright);

#endif // _ZOBRIST_H_
