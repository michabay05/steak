#include <stdio.h>

#define NOB_IMPLEMENTATION
#include "chess_unity.h"
#include "chess_unity.c"

// leaf nodes (number of positions reached during the test of the move generator at a given depth)
static u64 nodes = 0;

// perft driver
static inline void perft_driver(Board *board, int depth) {
    // recursion escape condition
    if (depth == 0) {
        // increment nodes count (count reached positions)
        nodes++;
        return;
    }

    // create move list instance
    MoveList ml = {0};

    // generate moves
    movelist_generate_all(&ml, board);

    // loop over generated moves
    Board clone;
    for (int move_count = 0; move_count < ml.count; move_count++) {
        // preserve board state
        clone = *board;

        // make move
        if (!move_make(board, ml.list[move_count], AllMoves))
            // skip to the next move
            continue;

        // call perft driver recursively
        perft_driver(board, depth - 1);

        // take back
        *board = clone;
    }
}

// perft test
void perft_test(Board *board, int depth) {
    printf("\n     Performance test\n\n");

    // create move list instance
    MoveList ml = {0};

    // generate moves
    movelist_generate_all(&ml, board);

    // init start time
    u64 start = nanos_since_unspecified_epoch();

    Board clone;
    // loop over generated moves
    for (int i = 0; i < ml.count; i++) {
        Move mv = ml.list[i];
        // preserve board state
        clone = *board;

        // make move
        if (!move_make(board, mv, AllMoves))
            // skip to the next move
            continue;

        // cummulative nodes
        long cummulative_nodes = nodes;

        // call perft driver recursively
        perft_driver(board, depth - 1);

        // old nodes
        long old_nodes = nodes - cummulative_nodes;

        // take back
        *board = clone;

        // print move
        char buf[6] = {0};
        move_to_str(mv, buf);
        printf("     move: %s  nodes: %ld\n", buf, old_nodes);
    }

    // print results
    printf("\n    Depth: %d\n", depth);
    printf("    Nodes: %ld\n", nodes);
    printf("     Time: %ld ms\n\n",
        (u64)((nanos_since_unspecified_epoch() - start) * 1e-6));
}

int main(int argc, char **argv) {
    char *program = nob_shift_args(&argc, &argv);
    if (argc != 2) {
        fprintf(stderr, "[ERROR] Expected 2 cmdline args\n");
        fprintf(stderr, "[NOTE] Usage: %s <FEN> <depth>\n", program);
        fprintf(stderr, "[WARN] Make sure that the fen string is single or double-quoted\n");
        return 1;
    }

    attack_init();

    Board board = {0};
    // FENInfo fen_info = parse_fen(argv[0]);
    FENInfo fen_info = parse_fen_cstr("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    board_set_from_fen(&board, fen_info);

    // perft_test(&board, atoi(argv[1]));
    perft_test(&board, 5);
}
