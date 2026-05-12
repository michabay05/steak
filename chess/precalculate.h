#ifndef _PRECALCULATE_H_
#define _PRECALCULATE_H_

#include "defs.h"
#include "bitboard.h"

// LEAPER PIECES
extern Bitboard pawn_attacks[2][64];
extern Bitboard knight_attacks[64];
extern Bitboard king_attacks[64];

// SLIDING PIECES
extern Bitboard bishop_occ_mask[64];
extern Bitboard bishop_attacks[64][512];
extern Bitboard rook_occ_mask[64];
extern Bitboard rook_attacks[64][4096];

extern int bishop_relevant_bits[64];
extern int rook_relevant_bits[64];

void attack_init(void);
void attack_init_leapers(void);
void attack_init_sliding(PieceType pt);

void gen_pawn_attacks(Color side, Sq sq);
void gen_knight_attacks(Sq sq);
void gen_king_attacks(Sq sq);
Bitboard gen_bishop_occupancy(Sq sq);
Bitboard gen_bishop_attack(Sq sq, Bitboard blocker_board);
Bitboard gen_rook_occupancy(Sq sq);
Bitboard gen_rook_attack(Sq sq, Bitboard blocker_board);
Bitboard set_occupancy(int index, int relevantBits, Bitboard occMask);

void magics_init(void);
Bitboard get_bishop_attack(Sq sq, Bitboard blocker_board);
Bitboard get_rook_attack(Sq sq, Bitboard blocker_board);
Bitboard get_queen_attack(Sq sq, Bitboard blocker_board);

#endif // _PRECALCULATE_H_
