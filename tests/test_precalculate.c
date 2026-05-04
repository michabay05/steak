#include <stdarg.h>

#include "../chess/chess_unity.h"
#include "test.h"

#define make_bb(...) make_bb_opt(sizeof((Sq[]){__VA_ARGS__}) / sizeof(Sq), __VA_ARGS__)
Bitboard make_bb_opt(size_t n, ...) {
    va_list args;
    va_start(args, n);

    Bitboard out = 0ULL;
    for (u32 i = 0; i < n; i++) set_bit(out, va_arg(args, int));

    va_end(args);
    return out;
}

static void test_attack_pawn(void) {
    struct {
        Color color;
        Sq sq;
        Bitboard expected;
    } items[] = {
        {C_WHITE, SQ_A2, make_bb(SQ_B3)},
        {C_WHITE, SQ_H8, 0ULL},
        {C_WHITE, SQ_E4, make_bb(SQ_D5, SQ_F5)},
        {C_WHITE, SQ_C8, 0ULL},
        {C_WHITE, SQ_H7, make_bb(SQ_G8)},

        {C_BLACK, SQ_A7, make_bb(SQ_B6)},
        {C_BLACK, SQ_H1, 0ULL},
        {C_BLACK, SQ_D5, make_bb(SQ_C4, SQ_E4)},
        {C_BLACK, SQ_F1, 0ULL},
        {C_BLACK, SQ_A2, make_bb(SQ_B1)},
    };

    for (u32 i = 0; i < ARRAY_LEN(items); i++) {
        Bitboard bb = pawn_attacks[items[i].color][items[i].sq];
        ENSURE(bb == items[i].expected);
    }
}

static void test_attack_knight(void) {
    struct {
        Sq sq; Bitboard expected;
    } items[] = {
        {SQ_A1, make_bb(SQ_B3, SQ_C2)},
        {SQ_H8, make_bb(SQ_G6, SQ_F7)},
        {SQ_D4, make_bb(
            SQ_C2, SQ_E2, SQ_B3, SQ_F3, SQ_B5, SQ_F5, SQ_C6, SQ_E6)},
        {SQ_F6, make_bb(
            SQ_E4, SQ_G4, SQ_D5, SQ_H5, SQ_D7, SQ_H7, SQ_E8, SQ_G8)},
        {SQ_H4, make_bb(SQ_G2, SQ_F3, SQ_F5, SQ_G6)},
        {SQ_B4, make_bb(SQ_A2, SQ_C2, SQ_D3, SQ_D5, SQ_A6, SQ_C6)},
        {SQ_A7, make_bb(SQ_B5, SQ_C6, SQ_C8)},
        {SQ_G2, make_bb(SQ_E1, SQ_E3, SQ_F4, SQ_H4)},
    };

    for (u32 i = 0; i < ARRAY_LEN(items); i++) {
        Bitboard bb = knight_attacks[items[i].sq];
        ENSURE(bb == items[i].expected);
    }
}

static void test_attack_king(void) {
    struct {
        Sq sq; Bitboard expected;
    } items[] = {
        {SQ_A1, make_bb(SQ_B1, SQ_A2, SQ_B2)},
        {SQ_H8, make_bb(SQ_G7, SQ_G8, SQ_H7)},
        {SQ_D4, make_bb(
            SQ_C3, SQ_D3, SQ_E3, SQ_C4, SQ_E4, SQ_C5, SQ_D5, SQ_E5)},
        {SQ_E8, make_bb(SQ_D7, SQ_E7, SQ_F7, SQ_D8, SQ_F8)},
        {SQ_H5, make_bb(SQ_G4, SQ_H4, SQ_G5, SQ_G6, SQ_H6)},
        {SQ_B4, make_bb(
            SQ_A3, SQ_B3, SQ_C3, SQ_A4, SQ_C4, SQ_A5, SQ_B5, SQ_C5)},
        {SQ_A3, make_bb(SQ_A2, SQ_B2, SQ_B3, SQ_A4, SQ_B4)},
        {SQ_F1, make_bb(SQ_E1, SQ_G1, SQ_E2, SQ_F2, SQ_G2)},
    };

    for (u32 i = 0; i < ARRAY_LEN(items); i++) {
        Bitboard bb = king_attacks[items[i].sq];
        ENSURE(bb == items[i].expected);
    }
}

static void test_attack_bishop(void) {
    struct {
        Sq sq; Bitboard blockers, expected;
    } items[] = {
        {SQ_A1, 0ULL, make_bb(
            SQ_B2, SQ_C3, SQ_D4, SQ_E5, SQ_F6, SQ_G7, SQ_H8)},
        {SQ_A1, make_bb(SQ_F6), make_bb(SQ_B2, SQ_C3, SQ_D4, SQ_E5, SQ_F6)},

        {SQ_H1, 0ULL, make_bb(
            SQ_G2, SQ_F3, SQ_E4, SQ_D5, SQ_C6, SQ_B7, SQ_A8)},
        {SQ_H1, make_bb(SQ_E4), make_bb(SQ_G2, SQ_F3, SQ_E4)},

        {SQ_C4, 0ULL, make_bb(
            SQ_B3, SQ_A2, SQ_F1, SQ_E2, SQ_D3, SQ_B5, SQ_A6,
            SQ_D5, SQ_E6, SQ_F7, SQ_G8)},
        {SQ_C4, make_bb(SQ_E2, SQ_D4, SQ_F7), make_bb(
            SQ_B3, SQ_A2, SQ_E2, SQ_D3, SQ_B5, SQ_A6, SQ_D5, SQ_E6, SQ_F7)},

        {SQ_E5, 0ULL, make_bb(
            SQ_A1, SQ_B2, SQ_C3, SQ_D4, SQ_F6, SQ_G7, SQ_H8,
            SQ_H2, SQ_G3, SQ_F4, SQ_D6, SQ_C7, SQ_B8
        )},
        {SQ_E5, make_bb(SQ_C3, SQ_H8, SQ_D6, SQ_B8), make_bb(
            SQ_C3, SQ_D4, SQ_F6, SQ_G7, SQ_H8, SQ_H2, SQ_G3, SQ_F4, SQ_D6)},

        {SQ_A7, 0ULL,
            make_bb(SQ_B8, SQ_G1, SQ_F2, SQ_E3, SQ_D4, SQ_C5, SQ_B6)},
        {SQ_A7, make_bb(SQ_C4, SQ_G6, SQ_E3),
            make_bb(SQ_B8, SQ_E3, SQ_D4, SQ_C5, SQ_B6)},

        {SQ_F1, 0ULL, make_bb(
            SQ_G2, SQ_H3, SQ_A6, SQ_B5, SQ_C4, SQ_D3, SQ_E2)},
        {SQ_F1, make_bb(SQ_G2, SQ_H3, SQ_D3), make_bb(SQ_G2, SQ_D3, SQ_E2)},

        {SQ_D8, 0ULL, make_bb(
            SQ_A5, SQ_B6, SQ_C7, SQ_H4, SQ_G5, SQ_F6, SQ_E7)},
        {SQ_D8, make_bb(SQ_A5, SQ_G4), make_bb(
            SQ_A5, SQ_B6, SQ_C7, SQ_H4, SQ_G5, SQ_F6, SQ_E7)},

        {SQ_H3, 0ULL, make_bb(
            SQ_F1, SQ_G2, SQ_C8, SQ_D7, SQ_E6, SQ_F5, SQ_G4)},
    };

    for (u32 i = 0; i < ARRAY_LEN(items); i++) {
        Bitboard bb = get_bishop_attack(items[i].sq, items[i].blockers);
        ENSURE(bb == items[i].expected);
    }
}

static void test_attack_rook(void) {
    struct {
        Sq sq; Bitboard blockers, expected;
    } items[] = {
        {SQ_A1, 0ULL, make_bb(SQ_A2, SQ_A3, SQ_A4, SQ_A5, SQ_A6, SQ_A7, SQ_A8,
            SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1)},
        {SQ_A1, make_bb(SQ_A4, SQ_B1), make_bb(SQ_A2, SQ_A3, SQ_A4, SQ_B1)},

        {SQ_H8, 0ULL, make_bb(SQ_H7, SQ_H6, SQ_H5, SQ_H4, SQ_H3, SQ_H2, SQ_H1, SQ_G8, SQ_F8, SQ_E8, SQ_D8, SQ_C8, SQ_B8, SQ_A8)},
        {SQ_H8, make_bb(SQ_B5, SQ_F6), make_bb(SQ_H7, SQ_H6, SQ_H5, SQ_H4,
            SQ_H3, SQ_H2, SQ_H1, SQ_G8, SQ_F8, SQ_E8, SQ_D8, SQ_C8, SQ_B8,
            SQ_A8
        )},

        {SQ_E8, 0ULL, make_bb(SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_F8, SQ_G8, SQ_H8,
            SQ_E1, SQ_E2, SQ_E3, SQ_E4, SQ_E5, SQ_E6, SQ_E7)},
        {SQ_E8, make_bb(SQ_C8), make_bb(SQ_C8, SQ_D8, SQ_F8, SQ_G8, SQ_H8,
            SQ_E1, SQ_E2, SQ_E3, SQ_E4, SQ_E5, SQ_E6, SQ_E7)},

        {SQ_H3, 0ULL, make_bb(SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3,
            SQ_H1, SQ_H2, SQ_H4, SQ_H5, SQ_H6, SQ_H7, SQ_H8)},
        {SQ_H3, make_bb(SQ_E4, SQ_D3, SQ_H6), make_bb(SQ_D3, SQ_E3, SQ_F3,
            SQ_G3, SQ_H1, SQ_H2, SQ_H4, SQ_H5, SQ_H6)},

        {SQ_A5, 0ULL, make_bb(SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
            SQ_A1, SQ_A2, SQ_A3, SQ_A4, SQ_A6, SQ_A7, SQ_A8)},
        {SQ_A5, make_bb(SQ_A3, SQ_D5), make_bb(SQ_B5, SQ_C5, SQ_D5, SQ_A3,
            SQ_A4, SQ_A6, SQ_A7, SQ_A8)},

        {SQ_C1, 0ULL, make_bb(SQ_A1, SQ_B1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
            SQ_C2, SQ_C3, SQ_C4, SQ_C5, SQ_C6, SQ_C7, SQ_C8)},

        {SQ_D4, 0ULL, make_bb(SQ_A4, SQ_B4, SQ_C4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
            SQ_D1, SQ_D2, SQ_D3, SQ_D5, SQ_D6, SQ_D7, SQ_D8)},
    };

    for (u32 i = 0; i < ARRAY_LEN(items); i++) {
        Bitboard bb = get_rook_attack(items[i].sq, items[i].blockers);
        ENSURE(bb == items[i].expected);
    }
}

void test_precalculate_main(void) {
    test_attack_pawn();
    test_attack_knight();
    test_attack_king();
    test_attack_bishop();
    test_attack_rook();
}
