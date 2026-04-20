#include "bitboard.h"
#include <stdio.h>
#include <sys/time.h>

#define NOB_IMPLEMENTATION
#include "../nob.h"
#include "precalculate.h"
#include "move_gen.h"

// leaf nodes (number of positions reached during the test of the move generator at a given depth)
uint64_t nodes;

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
    uint64_t start = nanos_since_unspecified_epoch();

    Board clone;
    // loop over generated moves
    for (int i = 0; i < ml.count; i++) {
        // preserve board state
        clone = *board;

        // make move
        if (!move_make(board, ml.list[i], AllMoves))
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
        printf("     move: %s%s%c  nodes: %ld\n",
            str_coords[ml.list[i].source],
            str_coords[ml.list[i].target],
            move_get_promoted(ml.list[i]) ? piece_char[move_get_promoted(ml.list[i])] : ' ',
            old_nodes);
    }

    // print results
    printf("\n    Depth: %d\n", depth);
    printf("    Nodes: %ld\n", nodes);
    printf("     Time: %ld ms\n\n",
        (uint64_t)((nanos_since_unspecified_epoch() - start) * 1e-6));
}

int main(int argc, char **argv) {
    #define TESTING(s) s * 2 + s

    char *program = nob_shift_args(&argc, &argv);
    if (argc != 2) {
        fprintf(stderr, "[ERROR] Expected 2 cmdline args\n");
        fprintf(stderr, "[NOTE] Usage: %s <FEN> <depth>\n", program);
        fprintf(stderr, "[WARN] Make sure that the fen string is single or double quoted\n");
        return 1;
    }

    attack_init();

    Board board;
    FENInfo fen_info = parse_fen(argv[0]);
    board_set_from_fen(&board, fen_info);
    board_print(&board);

    perft_test(&board, atoi(argv[1]));
}
