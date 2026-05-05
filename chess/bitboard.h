#ifndef _BITBOARD_H_
#define _BITBOARD_H_

#include "defs.h"

#define make_bb(...) make_bb_opt(sizeof((Sq[]){__VA_ARGS__}) / sizeof(Sq), __VA_ARGS__)
Bitboard make_bb_opt(size_t n, ...);

void bb_print(Bitboard bb);
int bb_count(Bitboard bb);
int bb_lsb_index(Bitboard bb);

#endif // _BITBOARD_H_
