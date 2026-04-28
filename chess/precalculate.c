#include "precalculate.h"

#include "magic_constants.h"

// LEAPER PIECES
Bitboard pawn_attacks[2][64];
Bitboard knight_attacks[64];
Bitboard king_attacks[64];

// SLIDING PIECES
Bitboard bishop_occ_mask[64];
Bitboard bishop_attacks[64][512];
Bitboard rook_occ_mask[64];
Bitboard rook_attacks[64][4096];

// clang-format off
// Total number of square a bishop can go to from a certain square
int bishop_relevant_bits[64] = {
    6, 5, 5, 5, 5, 5, 5, 6,
    5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5,
    6, 5, 5, 5, 5, 5, 5, 6,
};

// Total number of square a rook can go to from a certain square
int rook_relevant_bits[64] = {
    12, 11, 11, 11, 11, 11, 11, 12,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    12, 11, 11, 11, 11, 11, 11, 12,
};
// clang-format on

void attack_init(void) {
    attack_init_leapers();
    attack_init_sliding(PT_BISHOP);
    attack_init_sliding(PT_ROOK);
}

void attack_init_leapers(void) {
    for (int sq = 0; sq < 64; sq++) {
        gen_pawn_attacks(C_WHITE, sq);
        gen_pawn_attacks(C_BLACK, sq);
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

        Bitboard currentMask = (pt == PT_BISHOP)
            ? bishop_occ_mask[sq] : rook_occ_mask[sq];
        int bitCount = bb_count(currentMask);
        for (int count = 0; count < (1 << bitCount); count++) {
            // Generate a 'blocking' variation based on the current 'blocking' mask
            Bitboard occupancy = set_occupancy(count, bitCount, currentMask);
            int magicInd;
            // Generate a magic index that can be used to store the attack's sliding
            // pieces
            if (pt == PT_BISHOP) {
                magicInd = (int)((occupancy * bishop_magics[sq]) >> (64 - bitCount));
                bishop_attacks[sq][magicInd] = gen_bishop_attack(sq, occupancy);
            } else {
                magicInd = (int)((occupancy * rook_magics[sq]) >> (64 - bitCount));
                rook_attacks[sq][magicInd] = gen_rook_attack(sq, occupancy);
            }
        }
    }
}

void gen_pawn_attacks(Color side, Sq sq) {
    /* Since the board is set up where a8 is 0 and h1 is 63,
       the white pieces attack towards 0 while the black pieces
       attack towards 63.
    */
    u8 r = ROW(sq), f = COL(sq);

    if (side == C_WHITE) {
        if (r < 7 && f > 0) set_bit(pawn_attacks[C_WHITE][sq], sq + DIR_NW);
        if (r < 7 && f < 7) set_bit(pawn_attacks[C_WHITE][sq], sq + DIR_NE);
    } else {
        if (r > 0 && f > 0) set_bit(pawn_attacks[C_BLACK][sq], sq + DIR_SW);
        if (r > 0 && f < 7) set_bit(pawn_attacks[C_BLACK][sq], sq + DIR_SE);
    }
}

void gen_knight_attacks(Sq sq) {
    u8 r = ROW(sq), f = COL(sq);
    if (r <= 5 && f >= 1)
        set_bit(knight_attacks[sq], sq + DIR_NWN);

    if (r <= 6 && f >= 2)
        set_bit(knight_attacks[sq], sq + DIR_NWW);

    if (r <= 6 && f <= 5)
        set_bit(knight_attacks[sq], sq + DIR_NEE);

    if (r <= 5 && f <= 6)
        set_bit(knight_attacks[sq], sq + DIR_NEN);

    if (r >= 2 && f <= 6)
        set_bit(knight_attacks[sq], sq + DIR_SES);

    if (r >= 1 && f <= 5)
        set_bit(knight_attacks[sq], sq + DIR_SEE);

    if (r >= 1 && f >= 2)
        set_bit(knight_attacks[sq], sq + DIR_SWW);

    if (r >= 2 && f >= 1)
        set_bit(knight_attacks[sq], sq + DIR_SWS);
}

void gen_king_attacks(Sq sq) {
    u8 r = ROW(sq), f = COL(sq);

    if (r > 0) set_bit(king_attacks[sq], sq + DIR_SOUTH);
    if (r < 7) set_bit(king_attacks[sq], sq + DIR_NORTH);
    if (f > 0) set_bit(king_attacks[sq], sq + DIR_WEST);
    if (f < 7) set_bit(king_attacks[sq], sq + DIR_EAST);

    if (r > 0 && f > 0) set_bit(king_attacks[sq], sq + DIR_SW);
    if (r > 0 && f < 7) set_bit(king_attacks[sq], sq + DIR_SE);
    if (r < 7 && f > 0) set_bit(king_attacks[sq], sq + DIR_NW);
    if (r < 7 && f < 7) set_bit(king_attacks[sq], sq + DIR_NE);
}

Bitboard gen_bishop_occupancy(Sq sq) {
    Bitboard output = 0ULL;
    int r, f;
    int sr = ROW(sq), sf = COL(sq);

    for (r = sr + 1, f = sf + 1; r < 7 && f < 7; r++, f++)
        set_bit(output, SQ(r, f));

    for (r = sr + 1, f = sf - 1; r < 7 && f > 0; r++, f--)
        set_bit(output, SQ(r, f));

    for (r = sr - 1, f = sf + 1; r > 0 && f < 7; r--, f++)
        set_bit(output, SQ(r, f));

    for (r = sr - 1, f = sf - 1; r > 0 && f > 0; r--, f--)
        set_bit(output, SQ(r, f));

    return output;
}

/* Generates a bishop's attack given its sq and a 'blocking' pieces on its
   path */
Bitboard gen_bishop_attack(Sq sq, Bitboard blocker_board) {
    Bitboard output = 0ULL;
    int r, f;
    int sr = ROW(sq), sf = COL(sq);

    // NE direction
    for (r = sr + 1, f = sf + 1; r <= 7 && f <= 7; r++, f++) {
        set_bit(output, SQ(r, f));
        if (get_bit(blocker_board, SQ(r, f))) break;
    }
    // NW direction
    for (r = sr + 1, f = sf - 1; r <= 7 && f >= 0; r++, f--) {
        set_bit(output, SQ(r, f));
        if (get_bit(blocker_board, SQ(r, f))) break;
    }
    // SE direction
    for (r = sr - 1, f = sf + 1; r >= 0 && f <= 7; r--, f++) {
        set_bit(output, SQ(r, f));
        if (get_bit(blocker_board, SQ(r, f))) break;
    }
    // SW direction
    for (r = sr - 1, f = sf - 1; r >= 0 && f >= 0; r--, f--) {
        set_bit(output, SQ(r, f));
        if (get_bit(blocker_board, SQ(r, f))) break;
    }

    return output;
}

/* Generates all the maximum occupancy on a rook's path on its given square */
Bitboard gen_rook_occupancy(Sq sq) {
    Bitboard output = 0ULL;
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
Bitboard gen_rook_attack(Sq sq, Bitboard blocker_board) {
    Bitboard output = 0ULL;
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
Bitboard set_occupancy(int index, int relevantBits, Bitboard occMask) {
    Bitboard occupancy = 0ULL;
    for (int count = 0; count < relevantBits; count++) {
        int ls1bIndex = bb_lsb_index(occMask);
        pop_bit(occMask, ls1bIndex);
        if ((index & (1 << count)) > 0)
            set_bit(occupancy, ls1bIndex);
    }
    return occupancy;
}

static u32 randomState = 1804289383;
u32 random_u32(void) {
    u32 number = randomState;

    // XOR shift algorithm
    number ^= number << 13;
    number ^= number >> 17;
    number ^= number << 5;

    // Update random number state
    randomState = number;

    // Return random number
    return number;
}

u64 random_u64(void) {
    u64 rand1, rand2, rand3, rand4;
    rand1 = (u64)(random_u32() & 0xFFFF);
    rand2 = (u64)(random_u32() & 0xFFFF);
    rand3 = (u64)(random_u32() & 0xFFFF);
    rand4 = (u64)(random_u32() & 0xFFFF);
    return rand1 | (rand2 << 16) | (rand3 << 32) | (rand4 << 48);
}

// Used to generate sparse, random 64-bit numbers
// -> Sparse: the number of 1 (on-bits) are minimal
u64 pseudo_random_magic(void) {
    return random_u64() & random_u64() & random_u64();
}

Bitboard find_magics(Sq sq, int relevant_bits, PieceType piece) {
    // 4096(1 << 12) - because it's maximum possible occupancy variations
    Bitboard used_attacks[4096], occupancies[4096], attacks[4096], magic_number;
    Bitboard possible_occ = (piece == PT_BISHOP) ? gen_bishop_occupancy(sq) : gen_rook_occupancy(sq);
    int occupancy_indices = 1 << relevant_bits;
    for (int count = 0; count < occupancy_indices; count++) {
        occupancies[count] = set_occupancy(
            count, relevant_bits, possible_occ);
        attacks[count] = (piece == PT_BISHOP)
            ? gen_bishop_attack(sq, occupancies[count])
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
    printf("Failed to find magic number for %s on %s\n",
        (piece == PT_BISHOP) ? "bishop" : "rook", str_coords[sq]);
    return 0;
}

void magics_init(void) {
    int sq;
    printf("u64 rook_magics[64] = {\n");
    for (sq = 0; sq < 64; sq++)
        printf("0x%016lxULL,\n", find_magics(sq, rook_relevant_bits[sq], PT_ROOK));

    printf("\n};\n\n");
    printf("u64 bishop_magics[64] = {\n");
    for (sq = 0; sq < 64; sq++)
        printf("0x%016lxULL,\n", find_magics(sq, bishop_relevant_bits[sq], PT_BISHOP));
    printf("};\n");
}

Bitboard get_bishop_attack(Sq sq, Bitboard blocker_board) {
    blocker_board &= bishop_occ_mask[sq];
    blocker_board *= bishop_magics[sq];
    blocker_board >>= (64 - bishop_relevant_bits[sq]);
    return bishop_attacks[sq][blocker_board];
}

Bitboard get_rook_attack(Sq sq, Bitboard blocker_board) {
    blocker_board &= rook_occ_mask[sq];
    blocker_board *= rook_magics[sq];
    blocker_board >>= (64 - rook_relevant_bits[sq]);
    return rook_attacks[sq][blocker_board];
}

Bitboard get_queen_attack(Sq sq, Bitboard blocker_board) {
    return get_bishop_attack(sq, blocker_board) | get_rook_attack(sq, blocker_board);
}

