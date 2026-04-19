#define NOB_IMPLEMENTATION
#include "nob.h"

int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    Cmd cmd = {0};

    // Build perft
    cmd_append(&cmd, "clang");
    // cmd_append(&cmd, "-ggdb");
    cmd_append(&cmd, "-O3");
    cmd_append(&cmd, "-o", "perft");
    cmd_append(&cmd, "-Wall", "-Wextra", "-pedantic");
    cmd_append(&cmd, "chess/chess.c");
    cmd_append(&cmd, "chess/perft.c");
    if (!cmd_run(&cmd)) return 1;

    cmd_append(&cmd,
        "./perft",
        "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
        "5"
    );
    if (!cmd_run(&cmd)) return 1;

    return 0;
}
