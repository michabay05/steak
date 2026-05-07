#include "../chess/chess_unity.h"
#include "test.h"

#define FEN_STARTING_POS "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

static void test_move_two_square_push(void) {
    Board board = {0};

    FENInfo fen = parse_fen_cstr(FEN_STARTING_POS);
    board_set_from_fen(&board, fen);

    Move move = {
        .source = SQ_E2,
        .target = SQ_E4,
        .promoted = PT_NONE,
        .flag = MVF_TwoSquarePush
    };
    move_make(&board, move, AllMoves);

    ENSURE(board_get_piece(&board, move.source).type == PT_NONE);
    ENSURE(board_get_piece(&board, move.target).color == C_WHITE);
    ENSURE(board_get_piece(&board, move.target).type == PT_PAWN);
    ENSURE(board.castling == 15); // 15 = 0b1111
    ENSURE(board.side == C_BLACK);
    ENSURE(board.enpassant == SQ_E3);
}

static void test_move_capture(void) {
    Board board = {0};
    FENInfo fen = parse_fen_cstr(
        "r1bQ1rk1/p3bppp/5n2/2p5/8/4pN2/PPP1NPPP/R4RK1 b - - 0 13");
    board_set_from_fen(&board, fen);

    Move move = {
        .source = SQ_F8,
        .target = SQ_D8,
        .promoted = PT_NONE,
        .flag = MVF_Capture
    };
    move_make(&board, move, AllMoves);

    ENSURE(board_get_piece(&board, move.source).type == PT_NONE);
    ENSURE(board_get_piece(&board, move.target).type == PT_ROOK);
    ENSURE(board.side == C_WHITE);
    ENSURE(board.enpassant == SQ_NONE);
    ENSURE(board.castling == 0);
}

static void test_move_promotion(void) {
    Board board = {0};
    FENInfo fen = parse_fen_cstr(
        "1r4k1/5pp1/5n1p/2b5/2b5/4P1KP/p5P1/8 b - - 1 31");
    board_set_from_fen(&board, fen);

    Move move = {
        .source = SQ_A2,
        .target = SQ_A1,
        .promoted = PT_QUEEN,
        .flag = MVF_Quiet
    };
    move_make(&board, move, AllMoves);

    ENSURE(board_get_piece(&board, move.source).type == PT_NONE);
    ENSURE(board_get_piece(&board, move.target).type == PT_QUEEN);
    ENSURE(board.side == C_WHITE);
    ENSURE(board.castling == 0);
}

void test_moves_main(void) {
    test_move_two_square_push();
    test_move_capture();
    test_move_promotion();
}
