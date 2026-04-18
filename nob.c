#define NOB_IMPLEMENTATION
#include "nob.h"

int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    Cmd cmd = {0};

    // Build perft
    cmd_append(&cmd, "clang");
    cmd_append(&cmd, "-o", "perft");
    cmd_append(&cmd, "-Wall", "-Wextra", "-pedantic");
    cmd_append(&cmd, "chess/chess.c");
    cmd_append(&cmd, "chess/perft.c");
    if (!cmd_run(&cmd)) return 1;

    return 0;
}
