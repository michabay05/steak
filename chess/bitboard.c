#include "bitboard.h"
#include "defs.h"

const Bitboard RANK_MASK[8] = {
    0x00000000000000ff, 0x000000000000ff00, 0x0000000000ff0000,
    0x00000000ff000000, 0x000000ff00000000, 0x0000ff0000000000,
    0x00ff000000000000, 0xff00000000000000
};

const Bitboard FILE_MASK[8] = {
    0x0101010101010101, 0x0202020202020202, 0x0404040404040404,
    0x0808080808080808, 0x1010101010101010, 0x2020202020202020,
    0x4040404040404040, 0x8080808080808080
};

void bb_print(Bitboard bb) {
    printf("\n");
    for (int r = 0; r < 8; r++) {
        printf(" %d |", 8 - r);
        for (int f = 0; f < 8; f++)
            printf(" %c", get_bit(bb, (7 - r) * 8 + f) ? '1' : '.');

        printf("\n");
    }
    printf("     - - - - - - - -\n     a b c d e f g h\n");
    printf("\n\n      Decimal: %ld\n      Hexadecimal: 0x%lx\n", bb, bb);
}

#ifndef _WIN32
int bb_count(Bitboard bb) {
    return __builtin_popcountll(bb);
}

int bb_lsb_index(Bitboard bb) {
    return bb > 0 ? __builtin_ctzll(bb) : 0;
}
#else
int bb_count(Bitboard bb) {
    int count = 0;
    for (count = 0; bb; count++, bb &= bb - 1)
        ;
    return count;
}

int bb_lsb_index(Bitboard bb) {
    return bb > 0 ? bb_count(bb ^ (bb - 1)) - 1 : 0;
}
#endif

int init_rank_mask(void) {
    Bitboard bbs[8] = {0};
    for (int r = 0; r < RANK_COUNT; r++) {
        bbs[r] = 0;
        for (int f = 0; f < FILE_COUNT; f++) {
            set_bit(bbs[r], SQ(r, f));
        }
        bb_print(bbs[r]);
    }

    printf("{");
    for (int i = 0; i < 8; i++) {
        printf("0x%016lx, ", bbs[i]);
    }
    printf("}\n");

    return 0;
}

int init_file_masks(void) {
    Bitboard bbs[8] = {0};
    for (int f = 0; f < FILE_COUNT; f++) {
        bbs[f] = 0;
        for (int r = 0; r < RANK_COUNT; r++) {
            set_bit(bbs[f], SQ(r, f));
        }
        bb_print(bbs[f]);
    }

    printf("{");
    for (int i = 0; i < 8; i++) {
        printf("0x%016lx, ", bbs[i]);
    }
    printf("}\n");

    return 0;
}

