#include "precalculate.h"
#include "bitboard.h"
#include "magic_constants.h"
#include <string.h>

// LEAPER PIECES
uint64_t pawn_attacks[2][64];
uint64_t knight_attacks[64];
uint64_t king_attacks[64];

// SLIDING PIECES
uint64_t bishop_occ_mask[64];
uint64_t bishop_attacks[64][512];
uint64_t rook_occ_mask[64];
uint64_t rook_attacks[64][4096];

// Total number of square a bishop can go to from a certain square
const int bishop_relevant_bits[64] = {
    6, 5, 5, 5, 5, 5, 5, 6, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 7, 7, 7, 7, 5, 5, 5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5, 5, 5, 7, 7, 7, 7, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 5, 5, 5, 5, 5, 5, 6,
};

// Total number of square a rook can go to from a certain square
const int rook_relevant_bits[64] = {
    12, 11, 11, 11, 11, 11, 11, 12, 11, 10, 10, 10, 10, 10, 10, 11, 11, 10, 10, 10, 10, 10,
    10, 11, 11, 10, 10, 10, 10, 10, 10, 11, 11, 10, 10, 10, 10, 10, 10, 11, 11, 10, 10, 10,
    10, 10, 10, 11, 11, 10, 10, 10, 10, 10, 10, 11, 12, 11, 11, 11, 11, 11, 11, 12,
};
void attack_init() {
    attack_init_leapers();
    attack_init_sliding(BISHOP);
    attack_init_sliding(ROOK);
}

void attack_init_leapers() {
    for (int sq = 0; sq < 64; sq++) {
        gen_pawn_attacks(LIGHT, sq);
        gen_pawn_attacks(DARK, sq);
        gen_knight_attacks(sq);
        gen_king_attacks(sq);
    }
}

void attack_init_sliding(PieceType pt) {
    for (int sq = 0; sq < 64; sq++) {
        // Generate all possible variations which can obstruct the path of the
        // bishop or rook
        bishop_occ_mask[sq] = gen_bishop_occupancy(sq);
        rook_occ_mask[sq] = gen_rook_occupancy(sq);

        uint64_t currentMask = (pt == BISHOP) ? bishop_occ_mask[sq] : rook_occ_mask[sq];
        int bitCount = bb_count(currentMask);
        for (int count = 0; count < (1 << bitCount); count++) {
            // Generate a 'blocking' variation based on the current 'blocking' mask
            uint64_t occupancy = set_occupancy(count, bitCount, currentMask);
            int magicInd;
            // Generate a magic index that can be used to store the attack's sliding
            // pieces
            if (pt == BISHOP) {
                magicInd = (int)((occupancy * bishop_magics[sq]) >> (64 - bitCount));
                bishop_attacks[sq][magicInd] = gen_bishop_attack(sq, occupancy);
            } else {
                magicInd = (int)((occupancy * rook_magics[sq]) >> (64 - bitCount));
                rook_attacks[sq][magicInd] = gen_rook_attack(sq, occupancy);
            }
        }
    }
}

void gen_pawn_attacks(const PieceColor side, const Sq sq) {
    /* Since the board is set up where a8 is 0 and h1 is 63,
       the white pieces attack towards 0 while the black pieces
       attack towards 63.
    */
    if (side == LIGHT) {
        if (ROW(sq) > 0 && COL(sq) > 0)
            set_bit(pawn_attacks[LIGHT][sq], sq + SW);
        if (ROW(sq) > 0 && COL(sq) < 7)
            set_bit(pawn_attacks[LIGHT][sq], sq + SE);
    } else {
        if (ROW(sq) < 7 && COL(sq) > 0)
            set_bit(pawn_attacks[DARK][sq], sq + NW);
        if (ROW(sq) < 7 && COL(sq) < 7)
            set_bit(pawn_attacks[DARK][sq], sq + NE);
    }
}

void gen_knight_attacks(const Sq sq) {
    /* Knight attacks are generated regardless of the
       side to move because knights can go in all directions.
       Both sides use this attack table for knights.
    */
    if (ROW(sq) <= 5 && COL(sq) >= 1)
        set_bit(knight_attacks[sq], sq + NW_N);

    if (ROW(sq) <= 6 && COL(sq) >= 2)
        set_bit(knight_attacks[sq], sq + NW_W);

    if (ROW(sq) <= 6 && COL(sq) <= 5)
        set_bit(knight_attacks[sq], sq + NE_E);

    if (ROW(sq) <= 5 && COL(sq) <= 6)
        set_bit(knight_attacks[sq], sq + NE_N);

    if (ROW(sq) >= 2 && COL(sq) <= 6)
        set_bit(knight_attacks[sq], sq + SE_S);

    if (ROW(sq) >= 1 && COL(sq) <= 5)
        set_bit(knight_attacks[sq], sq + SE_E);

    if (ROW(sq) >= 1 && COL(sq) >= 2)
        set_bit(knight_attacks[sq], sq + SW_W);

    if (ROW(sq) >= 2 && COL(sq) >= 1)
        set_bit(knight_attacks[sq], sq + SW_S);
}

void gen_king_attacks(const Sq sq) {
    /* king attacks are generated regardless of the
       side to move because kings can go in all directions.
       Both sides use this attack table for kings.
    */
    if (ROW(sq) > 0)
        set_bit(king_attacks[sq], sq + SOUTH);
    if (ROW(sq) < 7)
        set_bit(king_attacks[sq], sq + NORTH);
    if (COL(sq) > 0)
        set_bit(king_attacks[sq], sq + WEST);
    if (COL(sq) < 7)
        set_bit(king_attacks[sq], sq + EAST);
    if (ROW(sq) > 0 && COL(sq) > 0)
        set_bit(king_attacks[sq], sq + SW);
    if (ROW(sq) > 0 && COL(sq) < 7)
        set_bit(king_attacks[sq], sq + SE);
    if (ROW(sq) < 7 && COL(sq) > 0)
        set_bit(king_attacks[sq], sq + NW);
    if (ROW(sq) < 7 && COL(sq) < 7)
        set_bit(king_attacks[sq], sq + NE);
}

uint64_t gen_bishop_occupancy(const Sq sq) {
    uint64_t output = 0ULL;
    int r, f;
    int sr = ROW(sq), sf = COL(sq);

    // NE direction
    for (r = sr + 1, f = sf + 1; r < 7 && f < 7; r++, f++)
        set_bit(output, SQ(r, f));
    // NW direction
    for (r = sr + 1, f = sf - 1; r < 7 && f > 0; r++, f--)
        set_bit(output, SQ(r, f));
    // SE direction
    for (r = sr - 1, f = sf + 1; r > 0 && f < 7; r--, f++)
        set_bit(output, SQ(r, f));
    // SW direction
    for (r = sr - 1, f = sf - 1; r > 0 && f > 0; r--, f--)
        set_bit(output, SQ(r, f));

    return output;
}

/* Generates a bishop's attack given its sq and a 'blocking' pieces on its
   path */
uint64_t gen_bishop_attack(const Sq sq, uint64_t blocker_board) {
    uint64_t output = 0ULL;
    int r, f;
    int sr = ROW(sq), sf = COL(sq);

    // NE direction
    for (r = sr + 1, f = sf + 1; r <= 7 && f <= 7; r++, f++) {
        set_bit(output, SQ(r, f));
        if (get_bit(blocker_board, SQ(r, f)))
            break;
    }
    // NW direction
    for (r = sr + 1, f = sf - 1; r <= 7 && f >= 0; r++, f--) {
        set_bit(output, SQ(r, f));
        if (get_bit(blocker_board, SQ(r, f)))
            break;
    }
    // SE direction
    for (r = sr - 1, f = sf + 1; r >= 0 && f <= 7; r--, f++) {
        set_bit(output, SQ(r, f));
        if (get_bit(blocker_board, SQ(r, f)))
            break;
    }
    // SW direction
    for (r = sr - 1, f = sf - 1; r >= 0 && f >= 0; r--, f--) {
        set_bit(output, SQ(r, f));
        if (get_bit(blocker_board, SQ(r, f)))
            break;
    }

    return output;
}

/* Generates all the maximum occupancy on a rook's path on its given square */
uint64_t gen_rook_occupancy(const Sq sq) {
    uint64_t output = 0ULL;
    int r, f;
    int sr = ROW(sq), sf = COL(sq);

    // N direction
    for (r = sr + 1; r < 7; r++)
        set_bit(output, SQ(r, sf));
    // S direction
    for (r = sr - 1; r > 0; r--)
        set_bit(output, SQ(r, sf));
    // E direction
    for (f = sf + 1; f < 7; f++)
        set_bit(output, SQ(sr, f));
    // W direction
    for (f = sf - 1; f > 0; f--)
        set_bit(output, SQ(sr, f));

    return output;
}

/* Generates a rook's attack given its sq and a 'blocking' pieces on its
   path */
uint64_t gen_rook_attack(const Sq sq, const uint64_t blocker_board) {
    uint64_t output = 0ULL;
    int r, f;
    int sr = ROW(sq), sf = COL(sq);

    // N direction
    for (r = sr + 1; r <= 7; r++) {
        set_bit(output, SQ(r, sf));
        if (get_bit(blocker_board, SQ(r, sf)))
            break;
    }
    // S direction
    for (r = sr - 1; r >= 0; r--) {
        set_bit(output, SQ(r, sf));
        if (get_bit(blocker_board, SQ(r, sf)))
            break;
    }
    // E direction
    for (f = sf + 1; f <= 7; f++) {
        set_bit(output, SQ(sr, f));
        if (get_bit(blocker_board, SQ(sr, f)))
            break;
    }
    // W direction
    for (f = sf - 1; f >= 0; f--) {
        set_bit(output, SQ(sr, f));
        if (get_bit(blocker_board, SQ(sr, f)))
            break;
    }

    return output;
}

/* Generates a variation of 'blocking' pieces given an index, relevant bits, and
   occupancy mask */
uint64_t set_occupancy(const int index, const int relevantBits, uint64_t occMask) {
    uint64_t occupancy = 0ULL;
    for (int count = 0; count < relevantBits; count++) {
        int ls1bIndex = bb_lsb_index(occMask);
        pop_bit(occMask, ls1bIndex);
        if ((index & (1 << count)) > 0)
            set_bit(occupancy, ls1bIndex);
    }
    return occupancy;
}

uint32_t randomState = 1804289383;

uint32_t random_u32() {
    uint32_t number = randomState;

    // XOR shift algorithm
    number ^= number << 13;
    number ^= number >> 17;
    number ^= number << 5;

    // Update random number state
    randomState = number;

    // Return random number
    return number;
}

uint64_t random_u64() {
    uint64_t rand1, rand2, rand3, rand4;
    rand1 = (uint64_t)(random_u32() & 0xFFFF);
    rand2 = (uint64_t)(random_u32() & 0xFFFF);
    rand3 = (uint64_t)(random_u32() & 0xFFFF);
    rand4 = (uint64_t)(random_u32() & 0xFFFF);
    return rand1 | (rand2 << 16) | (rand3 << 32) | (rand4 << 48);
}

uint64_t pseudo_random_magic() { return random_u64() & random_u64() & random_u64(); }

uint64_t find_magics(const Sq sq, const int relevant_bits, const PieceType piece) {
    // 4096(1 << 12) - because it's maximum possible occupancy variations
    uint64_t used_attacks[4096], occupancies[4096], attacks[4096], magic_number;
    uint64_t possible_occ = (piece == BISHOP) ? gen_bishop_occupancy(sq) : gen_rook_occupancy(sq);
    int occupancy_indices = 1 << relevant_bits;
    for (int count = 0; count < occupancy_indices; count++) {
        occupancies[count] = set_occupancy(count, relevant_bits, possible_occ);
        attacks[count] = (piece == BISHOP) ? gen_bishop_attack(sq, occupancies[count])
                                           : gen_rook_attack(sq, occupancies[count]);
    }

    for (int rand_count = 0; rand_count < 100000000; rand_count++) {
        magic_number = pseudo_random_magic();
        if (bb_count((possible_occ * magic_number) & 0xFF00000000000000) < 6)
            continue;
        memset(used_attacks, 0, sizeof(used_attacks));
        int count, fail_flag;
        for (count = 0, fail_flag = 0; !fail_flag && count < occupancy_indices; count++) {
            int magic_ind = (int)((occupancies[count] * magic_number) >> (64 - relevant_bits));
            if (used_attacks[magic_ind] == 0)
                used_attacks[magic_ind] = attacks[count];
            else if (used_attacks[magic_ind] != attacks[count])
                fail_flag = 1;
        }
        if (!fail_flag)
            return magic_number;
    }
    printf("Failed to find magic number for %s on %s\n", (piece == BISHOP) ? "bishop" : "rook",
           str_coords[sq]);
    return 0;
}

void magics_init() {
    int sq;
    printf("ROOK: {\n");
    for (sq = 0; sq < 64; sq++)
        printf("0x%lxULL,\n", find_magics(sq, rook_relevant_bits[sq], ROOK));

    printf("\n}\n\n");
    printf("BISHOP: {\n");
    for (sq = 0; sq < 64; sq++)
        printf("0x%lxULL,\n", find_magics(sq, bishop_relevant_bits[sq], BISHOP));
    printf("};\n");
}

uint64_t get_bishop_attack(const Sq sq, uint64_t blocker_board) {
    blocker_board &= bishop_occ_mask[sq];
    blocker_board *= bishop_magics[sq];
    blocker_board >>= (64 - bishop_relevant_bits[sq]);
    return bishop_attacks[sq][blocker_board];
}

uint64_t get_rook_attack(const Sq sq, uint64_t blocker_board) {
    blocker_board &= rook_occ_mask[sq];
    blocker_board *= rook_magics[sq];
    blocker_board >>= (64 - rook_relevant_bits[sq]);
    return rook_attacks[sq][blocker_board];
}

uint64_t get_queen_attack(const Sq sq, uint64_t blocker_board) {
    return get_bishop_attack(sq, blocker_board) | get_rook_attack(sq, blocker_board);
}
