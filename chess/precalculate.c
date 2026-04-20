#include "precalculate.h"
#include "bitboard.h"

// LEAPER PIECES
uint64_t pawn_attacks[2][64];
uint64_t knight_attacks[64];
uint64_t king_attacks[64];

// SLIDING PIECES
uint64_t bishop_occ_mask[64];
uint64_t bishop_attacks[64][512];
uint64_t rook_occ_mask[64];
uint64_t rook_attacks[64][4096];

// clang-format off
// Total number of square a bishop can go to from a certain square
const int bishop_relevant_bits[64] = {
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
const int rook_relevant_bits[64] = {
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
    attack_init_sliding(BISHOP);
    attack_init_sliding(ROOK);
}

void attack_init_leapers(void) {
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
    uint8_t r = ROW(sq), f = COL(sq);

    if (side == LIGHT) {
        if (r < 7 && f > 0) set_bit(pawn_attacks[LIGHT][sq], sq + NW);
        if (r < 7 && f < 7) set_bit(pawn_attacks[LIGHT][sq], sq + NE);
    } else {
        if (r > 0 && f > 0) set_bit(pawn_attacks[DARK][sq], sq + SW);
        if (r > 0 && f < 7) set_bit(pawn_attacks[DARK][sq], sq + SE);
    }
}

void gen_knight_attacks(const Sq sq) {
    uint8_t r = ROW(sq), f = COL(sq);
    if (r <= 5 && f >= 1)
        set_bit(knight_attacks[sq], sq + NW_N);

    if (r <= 6 && f >= 2)
        set_bit(knight_attacks[sq], sq + NW_W);

    if (r <= 6 && f <= 5)
        set_bit(knight_attacks[sq], sq + NE_E);

    if (r <= 5 && f <= 6)
        set_bit(knight_attacks[sq], sq + NE_N);

    if (r >= 2 && f <= 6)
        set_bit(knight_attacks[sq], sq + SE_S);

    if (r >= 1 && f <= 5)
        set_bit(knight_attacks[sq], sq + SE_E);

    if (r >= 1 && f >= 2)
        set_bit(knight_attacks[sq], sq + SW_W);

    if (r >= 2 && f >= 1)
        set_bit(knight_attacks[sq], sq + SW_S);
}

void gen_king_attacks(const Sq sq) {
    uint8_t r = ROW(sq), f = COL(sq);

    if (r > 0) set_bit(king_attacks[sq], sq + SOUTH);
    if (r < 7) set_bit(king_attacks[sq], sq + NORTH);
    if (f > 0) set_bit(king_attacks[sq], sq + WEST);
    if (f < 7) set_bit(king_attacks[sq], sq + EAST);

    if (r > 0 && f > 0) set_bit(king_attacks[sq], sq + SW);
    if (r > 0 && f < 7) set_bit(king_attacks[sq], sq + SE);
    if (r < 7 && f > 0) set_bit(king_attacks[sq], sq + NW);
    if (r < 7 && f < 7) set_bit(king_attacks[sq], sq + NE);
}

uint64_t gen_bishop_occupancy(const Sq sq) {
    uint64_t output = 0ULL;
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
uint64_t gen_bishop_attack(const Sq sq, uint64_t blocker_board) {
    uint64_t output = 0ULL;
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

uint32_t random_u32(void) {
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

uint64_t random_u64(void) {
    uint64_t rand1, rand2, rand3, rand4;
    rand1 = (uint64_t)(random_u32() & 0xFFFF);
    rand2 = (uint64_t)(random_u32() & 0xFFFF);
    rand3 = (uint64_t)(random_u32() & 0xFFFF);
    rand4 = (uint64_t)(random_u32() & 0xFFFF);
    return rand1 | (rand2 << 16) | (rand3 << 32) | (rand4 << 48);
}

uint64_t pseudo_random_magic(void) { return random_u64() & random_u64() & random_u64(); }

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

void magics_init(void) {
    int sq;
    printf("const uint64_t rook_magics[64] = {\n");
    for (sq = 0; sq < 64; sq++)
        printf("0x%016lxULL,\n", find_magics(sq, rook_relevant_bits[sq], ROOK));

    printf("\n};\n\n");
    printf("const uint64_t bishop_magics[64] = {\n");
    for (sq = 0; sq < 64; sq++)
        printf("0x%016lxULL,\n", find_magics(sq, bishop_relevant_bits[sq], BISHOP));
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


const uint64_t rook_magics[64] = {
    0x8a80104000800020ULL, 0x0140002000100040ULL, 0x02801880a0017001ULL,
    0x0100081001000420ULL, 0x0200020010080420ULL, 0x03001c0002010008ULL,
    0x8480008002000100ULL, 0x2080088004402900ULL, 0x0000800098204000ULL,
    0x2024401000200040ULL, 0x0100802000801000ULL, 0x0120800800801000ULL,
    0x0208808088000400ULL, 0x0002802200800400ULL, 0x2200800100020080ULL,
    0x0801000060821100ULL, 0x0080044006422000ULL, 0x0100808020004000ULL,
    0x12108a0010204200ULL, 0x0140848010000802ULL, 0x0481828014002800ULL,
    0x8094004002004100ULL, 0x4010040010010802ULL, 0x0000020008806104ULL,
    0x0100400080208000ULL, 0x2040002120081000ULL, 0x0021200680100081ULL,
    0x0020100080080080ULL, 0x0002000a00200410ULL, 0x0000020080800400ULL,
    0x0080088400100102ULL, 0x0080004600042881ULL, 0x4040008040800020ULL,
    0x0440003000200801ULL, 0x0004200011004500ULL, 0x0188020010100100ULL,
    0x0014800401802800ULL, 0x2080040080800200ULL, 0x0124080204001001ULL,
    0x0200046502000484ULL, 0x0480400080088020ULL, 0x1000422010034000ULL,
    0x0030200100110040ULL, 0x0000100021010009ULL, 0x2002080100110004ULL,
    0x0202008004008002ULL, 0x0020020004010100ULL, 0x2048440040820001ULL,
    0x0101002200408200ULL, 0x0040802000401080ULL, 0x4008142004410100ULL,
    0x02060820c0120200ULL, 0x0001001004080100ULL, 0x020c020080040080ULL,
    0x2935610830022400ULL, 0x0044440041009200ULL, 0x0280001040802101ULL,
    0x2100190040002085ULL, 0x80c0084100102001ULL, 0x4024081001000421ULL,
    0x00020030a0244872ULL, 0x0012001008414402ULL, 0x02006104900a0804ULL,
    0x0001004081002402ULL,
};

const uint64_t bishop_magics[64] = {
    0x0040040844404084ULL, 0x002004208a004208ULL, 0x0010190041080202ULL,
    0x0108060845042010ULL, 0x0581104180800210ULL, 0x2112080446200010ULL,
    0x1080820820060210ULL, 0x03c0808410220200ULL, 0x0004050404440404ULL,
    0x0000021001420088ULL, 0x24d0080801082102ULL, 0x0001020a0a020400ULL,
    0x0000040308200402ULL, 0x0004011002100800ULL, 0x0401484104104005ULL,
    0x0801010402020200ULL, 0x00400210c3880100ULL, 0x0404022024108200ULL,
    0x0810018200204102ULL, 0x0004002801a02003ULL, 0x0085040820080400ULL,
    0x810102c808880400ULL, 0x000e900410884800ULL, 0x8002020480840102ULL,
    0x0220200865090201ULL, 0x2010100a02021202ULL, 0x0152048408022401ULL,
    0x0020080002081110ULL, 0x4001001021004000ULL, 0x800040400a011002ULL,
    0x00e4004081011002ULL, 0x001c004001012080ULL, 0x8004200962a00220ULL,
    0x8422100208500202ULL, 0x2000402200300c08ULL, 0x8646020080080080ULL,
    0x80020a0200100808ULL, 0x2010004880111000ULL, 0x623000a080011400ULL,
    0x42008c0340209202ULL, 0x0209188240001000ULL, 0x400408a884001800ULL,
    0x00110400a6080400ULL, 0x1840060a44020800ULL, 0x0090080104000041ULL,
    0x0201011000808101ULL, 0x1a2208080504f080ULL, 0x8012020600211212ULL,
    0x0500861011240000ULL, 0x0180806108200800ULL, 0x4000020e01040044ULL,
    0x300000261044000aULL, 0x0802241102020002ULL, 0x0020906061210001ULL,
    0x5a84841004010310ULL, 0x0004010801011c04ULL, 0x000a010109502200ULL,
    0x0000004a02012000ULL, 0x500201010098b028ULL, 0x8040002811040900ULL,
    0x0028000010020204ULL, 0x06000020202d0240ULL, 0x8918844842082200ULL,
    0x4010011029020020ULL,
};
