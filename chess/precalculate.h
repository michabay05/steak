#ifndef _PRECALCULATE_H_
#define _PRECALCULATE_H_

#include "defs.h"

// LEAPER PIECES
extern uint64_t pawn_attacks[2][64];
extern uint64_t knight_attacks[64];
extern uint64_t king_attacks[64];

// SLIDING PIECES
extern uint64_t bishop_occ_mask[64];
extern uint64_t bishop_attacks[64][512];
extern uint64_t rook_occ_mask[64];
extern uint64_t rook_attacks[64][4096];

extern const uint64_t bishop_magics[64];
extern const uint64_t rook_magics[64];

void attack_init(void);
void attack_init_leapers(void);
void attack_init_sliding(PieceType pt);

void gen_pawn_attacks(PieceColor color, Sq sq);
void gen_knight_attacks(Sq sq);
void gen_king_attacks(Sq sq);

uint64_t gen_bishop_occupancy(const Sq sq);
uint64_t gen_bishop_attack(const Sq sq, uint64_t blockerBoard);
uint64_t gen_rook_occupancy(const Sq sq);
uint64_t gen_rook_attack(const Sq sq, const uint64_t blockerBoard);
uint64_t set_occupancy(const int index, const int relevantBits, uint64_t occMask);

uint32_t random_u32(void);
uint64_t random_u64(void);
uint64_t pseudo_random_magic(void);
uint64_t find_magics(const Sq sq, const int relevantBits, const PieceType piece);
void magics_init(void);
uint64_t get_bishop_attack(const Sq sq, uint64_t blockerBoard);
uint64_t get_rook_attack(const Sq sq, uint64_t blockerBoard);
uint64_t get_queen_attack(const Sq sq, uint64_t blockerBoard);

#endif // _PRECALCULATE_H_
