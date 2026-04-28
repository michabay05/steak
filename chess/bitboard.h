#ifndef _BITBOARD_H_
#define _BITBOARD_H_

#include "defs.h"

typedef u64 Bitboard;

void bb_print(Bitboard bb);
int bb_count(Bitboard bb);
int bb_lsb_index(Bitboard bb);

#endif // _BITBOARD_H_
