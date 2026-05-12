#ifndef _DEFS_H_
#define _DEFS_H_

// Common standard header files
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../nob.h"

typedef int8_t i8;
typedef int16_t i16;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float f32;

typedef u64 Bitboard;

#define ENUM_DEF(e_type, e_name) e_type e_name; enum

// clang-format off
typedef ENUM_DEF(u16, Sq) {
    SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
	SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
    SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
	SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
    SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
	SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
    SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
	SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8,
	SQ_COUNT, SQ_NONE,
};
// clang-format on
static_assert(SQ_COUNT == 64, "There needs to be 64 squares.");

typedef ENUM_DEF(u8, Rank) {
    RANK_1, RANK_2, RANK_3, RANK_4,
    RANK_5, RANK_6, RANK_7, RANK_8, RANK_COUNT,
};
static_assert(RANK_COUNT == 8, "There needs to be 8 ranks (rows).");

typedef ENUM_DEF(u8, File) {
    FILE_A, FILE_B, FILE_C, FILE_D,
    FILE_E, FILE_F, FILE_G, FILE_H, FILE_COUNT,
};
static_assert(FILE_COUNT == 8, "There needs to be 8 files (columns).");

extern const char *str_coords[66];
extern const char *piece_char[2];
extern const Bitboard RANK_MASK[8];
extern const Bitboard FILE_MASK[8];

#define ROW(sq) (((Sq)sq) >> 3)
#define COL(sq) (((Sq)sq) & 7)
#define SQ(r, f) ((Sq)((r) * 8 + (f)))
#define FLIP(sq) ((Sq)sq ^ 56)
#define COLORLESS(piece) (((Piece)piece) % 6)
#define SQCLR(r, f) (((int)(r + f + 1)) & 1)
// Takes an integer value and converts it to 0 or 1
#define TO_BOOL(x) ((x) != 0)

#define set_bit(bitboard, square) ((bitboard) |= (1ULL << (square)))
#define get_bit(bitboard, square) (((bitboard) & (1ULL << (square))) ? 1 : 0)
#define pop_bit(bitboard, square) ((bitboard) &= ~(1ULL << (square)))

typedef ENUM_DEF(u8, PieceType) {
    PT_PAWN, PT_KNIGHT, PT_BISHOP, PT_ROOK, PT_QUEEN, PT_KING,
    PT_COUNT, PT_NONE
};
static_assert(PT_COUNT == 6, "There needs to be 6 piece types.");

typedef ENUM_DEF(u8, Color) { C_WHITE, C_BLACK, C_COUNT };
static_assert(C_COUNT == 2, "There needs to be 2 colors.");

typedef struct {
    Color color : 1;
    PieceType type : 3;
} Piece;

#define P_LP ((Piece){.color = C_WHITE, .type = PT_PAWN})
#define P_LN ((Piece){.color = C_WHITE, .type = PT_KNIGHT})
#define P_LB ((Piece){.color = C_WHITE, .type = PT_BISHOP})
#define P_LR ((Piece){.color = C_WHITE, .type = PT_ROOK})
#define P_LQ ((Piece){.color = C_WHITE, .type = PT_QUEEN})
#define P_LK ((Piece){.color = C_WHITE, .type = PT_KING})

#define P_DP ((Piece){.color = C_BLACK, .type = PT_PAWN})
#define P_DN ((Piece){.color = C_BLACK, .type = PT_KNIGHT})
#define P_DB ((Piece){.color = C_BLACK, .type = PT_BISHOP})
#define P_DR ((Piece){.color = C_BLACK, .type = PT_ROOK})
#define P_DQ ((Piece){.color = C_BLACK, .type = PT_QUEEN})
#define P_DK ((Piece){.color = C_BLACK, .type = PT_KING})
#define P_NONE ((Piece){.color = C_BLACK, .type = PT_NONE})

typedef ENUM_DEF(u8, CastlingRight) {
    CR_LK, CR_LQ, CR_DK, CR_DQ, CR_COUNT
};
static_assert(CR_COUNT == 4, "There needs to be 2 colors.");

/* Direction offsets */
typedef ENUM_DEF(i8, Direction) {
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
};

typedef struct board Board;

#endif // _DEFS_H_
