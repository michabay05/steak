#include "defs.h"
#include "move_gen.h"
#include "test.h"
#include "zobrist.h"
#include "board.h"

/*

*/

static void test_zobrist_from_scratch(void) {
    struct {
        const char *fen;
        ZB_Key expected;
    } items[] = {
        { "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
            0xe0ac430339c6fb3e },

        { "rnbqkb1r/pp1p1pPp/8/2p1pP2/1P1P4/3P3P/P1P1P3/RNBQKBNR w KQkq e3 0 1",
            0x6347f026aedce7c },

        { "r2q1rk1/ppp2ppp/2n1bn2/2b1p3/3pP3/3P1NPP/PPP1NPB1/R1BQ1RK1 b - - 0 9",
            0x611538e703381dac },

        { "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
            0xed1ef6942fb0f13e },

        { "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
            0x20daf29de066feac },

        { "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
            0x5348db2c42aa4d3d },

        { "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
            0x19b60eb75135f186 },
    };

    for (u32 i = 0; i < ARRAY_LEN(items); i++) {
        Board b = {0};
        board_parse_fen_cstr(&b, items[i].fen);
        ZB_Key key = zobrist_gen_key(&b);
        ENSURE(key == items[i].expected);
    }
}

#define ZOBIRST_INCREMENTAL_TEST
static void test_zobrist_incremental(void) {
    const char *fens[] = {
        "rnbqkb1r/pp1p1pPp/8/2p1pP2/1P1P4/3P3P/P1P1P3/RNBQKBNR w KQkq e3 0 1",
        "r2q1rk1/ppp2ppp/2n1bn2/2b1p3/3pP3/3P1NPP/PPP1NPB1/R1BQ1RK1 b - - 0 9",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
    };

    for (u32 i = 0; i < ARRAY_LEN(fens); i++) {
        Board b = {0};
        board_parse_fen_cstr(&b, fens[i]);
        perft_driver(NULL, &b, 4);
    }
}
#undef ZOBIRST_INCREMENTAL_TEST

void test_zobrist_main(void) {
    test_zobrist_from_scratch();
    test_zobrist_incremental();
}
