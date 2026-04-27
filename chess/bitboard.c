#include "bitboard.h"

void bb_print(const uint64_t bitboard) {
    printf("\n");
    for (int r = 0; r < 8; r++) {
        printf(" %d |", 8 - r);
        for (int f = 0; f < 8; f++)
            printf(" %c", get_bit(bitboard, (7 - r) * 8 + f) ? '1' : '.');

        printf("\n");
    }
    printf("     - - - - - - - -\n     a b c d e f g h\n");
    printf("\n\n      Decimal: %ld\n      Hexadecimal: 0x%lx\n", bitboard, bitboard);
}

#ifndef _WIN32
int bb_count(uint64_t bitboard) {
    return __builtin_popcountll(bitboard);
}

int bb_lsb_index(const uint64_t bitboard) {
    return bitboard > 0 ? __builtin_ctzll(bitboard) : 0;
}
#else
int bb_count(uint64_t bitboard) {
    int count = 0;
    for (count = 0; bitboard; count++, bitboard &= bitboard - 1)
        ;
    return count;
}

int bb_lsb_index(const uint64_t bitboard) {
    return bitboard > 0 ? bb_count(bitboard ^ (bitboard - 1)) - 1 : 0;
}
#endif
