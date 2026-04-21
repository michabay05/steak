#ifndef _DEFS_H_
#define _DEFS_H_

// Common standard header files
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// clang-format off
typedef enum {
    SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
	SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
    SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
	SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
    SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
	SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
    SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
	SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8,
	SQ_COUNT, SQ_NONE,
} Sq;
// clang-format on
static_assert(SQ_COUNT == 64, "There needs to be 64 squares.");

extern const char *str_coords[66];
extern const char piece_char[14];

#define ROW(sq) (((Sq)sq) >> 3)
#define COL(sq) (((Sq)sq) & 7)
#define SQ(r, f) (((int)r) * 8 + ((int)f))
#define FLIP(sq) ((Sq)sq ^ 56)
#define COLORLESS(piece) (((Piece)piece) % 6)
#define SQCLR(r, f) (((int)(r + f + 1)) & 1)

#define set_bit(bitboard, square) ((bitboard) |= (1ULL << (square)))
#define get_bit(bitboard, square) (((bitboard) & (1ULL << (square))) ? 1 : 0)
#define pop_bit(bitboard, square) ((bitboard) &= ~(1ULL << (square)))

/* Pieces */
typedef enum {
    P_LP, P_LN, P_LB, P_LR, P_LQ, P_LK,
    P_DP, P_DN, P_DB, P_DR, P_DQ, P_DK,
    P_COUNT, P_NONE
} Piece;
static_assert(P_COUNT == 12, "There needs to be 12 pieces.");

typedef enum {
    PT_PAWN, PT_KNIGHT, PT_BISHOP, PT_ROOK, PT_QUEEN, PT_KING,
    PT_COUNT
} PieceType;
static_assert(PT_COUNT == 6, "There needs to be 6 piece types.");

typedef enum { C_WHITE, C_BLACK, C_COUNT } Color;
static_assert(C_COUNT == 2, "There needs to be 2 colors.");

typedef enum { CR_LK, CR_LQ, CR_DK, CR_DQ, CR_COUNT } CastlingRight;
static_assert(CR_COUNT == 4, "There needs to be 2 colors.");

/* Direction offsets */
typedef enum {
    DIR_NORTH = 8,
    DIR_SOUTH = -8,
    DIR_WEST = -1,
    DIR_EAST = 1,
    DIR_NE = 8 + 1,
    DIR_NW = 8 - 1,
    DIR_SE = -8 + 1,
    DIR_SW = -8 - 1,

    // NOTE: for knights only
    DIR_NEN = (8 + 1) + 8,  // 17
    DIR_NEE = (8 + 1) + 1,  // 11
    DIR_NWN = (8 - 1) + 8,  // 15
    DIR_NWW = (8 - 1) - 1,  // 6
    DIR_SES = (-8 + 1) - 8, // -15
    DIR_SEE = (-8 + 1) + 1, // -6
    DIR_SWS = (-8 - 1) - 8, // -17
    DIR_SWW = (-8 - 1) - 1, // -10
} Direction;

#endif // _DEFS_H_
