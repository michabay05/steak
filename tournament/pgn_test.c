#define NOB_IMPLEMENTATION
#include "pgn.c"

int main(void) {
    PGN pgn = {0};
    if (pgn_read("test.pgn", &pgn)) {
        pgn_print(pgn);
    }

    pgn_deinit(&pgn);
    return 0;
}
