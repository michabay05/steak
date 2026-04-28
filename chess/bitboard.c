#include "bitboard.h"

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
