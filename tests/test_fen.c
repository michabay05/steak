#include "../chess/chess_unity.h"
#include "test.h"

#define FEN_EMPTY_BOARD "8/8/8/8/8/8/8/8 w - - 0 1"
#define FEN_STARTING_POS "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

static void test_compare_boards(Board *board, Board *expected) {
    // Check the board's piece arrangement
    for (Color c = C_WHITE; c <= C_BLACK; c++) {
        for (Color pt = PT_PAWN; pt <= PT_KING; pt++) {
            ENSURE(board->piece[c][pt] == expected->piece[c][pt]);
        }
    }
    ENSURE(board->units[C_WHITE] == expected->units[C_WHITE]);
    ENSURE(board->units[C_BLACK] == expected->units[C_BLACK]);
    ENSURE(board->all_units == expected->all_units);

    // Check the board's state
    ENSURE(board->side == expected->side);
    ENSURE(board->castling == expected->castling);
    ENSURE(board->enpassant == expected->enpassant);
    ENSURE(board->half_moves == expected->half_moves);
    ENSURE(board->full_moves == expected->full_moves);
}

static void test_fen_empty(void) {
    Board expected = {
        .piece = {0},
        .units = {0},
        .all_units = 0ULL,

        .side = C_WHITE,
        .enpassant = SQ_NONE,
        .half_moves = 0,
        .full_moves = 1,
        .castling = 0,
    };

    Board board = {0};
    FENInfo info = parse_fen_cstr(FEN_EMPTY_BOARD);
    board_set_from_fen(&board, info);

    test_compare_boards(&board, &expected);
}

static void test_fen_starting(void) {
    Board expected = {
        .piece = {
            {
                make_bb(SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2),
                make_bb(SQ_B1, SQ_G1),
                make_bb(SQ_C1, SQ_F1),
                make_bb(SQ_A1, SQ_H1),
                make_bb(SQ_D1),
                make_bb(SQ_E1),
            }, {
                make_bb(SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7),
                make_bb(SQ_B8, SQ_G8),
                make_bb(SQ_C8, SQ_F8),
                make_bb(SQ_A8, SQ_H8),
                make_bb(SQ_D8),
                make_bb(SQ_E8),
            }
        },
        .units = {
            make_bb(
                SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
                SQ_B1, SQ_G1, SQ_C1, SQ_F1, SQ_A1, SQ_H1, SQ_D1, SQ_E1),
            make_bb(
                SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
                SQ_B8, SQ_G8, SQ_C8, SQ_F8, SQ_A8, SQ_H8, SQ_D8, SQ_E8),
        },
        .all_units = make_bb(
            SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
            SQ_B1, SQ_G1, SQ_C1, SQ_F1, SQ_A1, SQ_H1, SQ_D1, SQ_E1,
            SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
            SQ_B8, SQ_G8, SQ_C8, SQ_F8, SQ_A8, SQ_H8, SQ_D8, SQ_E8),

        .side = C_WHITE,
        .enpassant = SQ_NONE,
        .half_moves = 0,
        .full_moves = 1,
        .castling = (1<<CR_LK)|(1<<CR_LQ)|(1<<CR_DK)|(1<<CR_DQ),
    };

    Board board = {0};
    FENInfo info = parse_fen_cstr(FEN_STARTING_POS);
    board_set_from_fen(&board, info);

    test_compare_boards(&board, &expected);
}

void test_fen_main(void) {
    test_fen_empty();
    test_fen_starting();
}
