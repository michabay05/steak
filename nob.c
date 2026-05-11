#define NOB_IMPLEMENTATION
#include "nob.h"

#define C_COMP "clang"
#define OUT_DIR "build/"
#define CHESS_UNITY "chess_unity"

enum BuildMode {
    BM_DEBUG,
    BM_RELEASE,
} BUILD = BM_DEBUG;
const char *MODE_STR[] = { "DEBUG", "RELEASE" };

static void __add_comp_n_flags(Cmd *cmd) {
    cmd_append(cmd, C_COMP);
    cmd_append(cmd, "-Wall", "-Wextra", "-pedantic");

    switch (BUILD) {
        case BM_DEBUG  : cmd_append(cmd, "-g"); break;
        case BM_RELEASE: cmd_append(cmd, "-O3"); break;
    }
}

static void __add_libchess(Cmd *cmd) {
    const char *dir = "chess";
    cmd_append(cmd, temp_sprintf("-I%s/", dir));
    const char *modules[] = {
        "bitboard", "board", "move", "move_gen", "precalculate",
    };

    for (int i = 0; i < ARRAY_LEN(modules); i++)
        cmd_append(cmd, temp_sprintf("chess/%s.c", modules[i]));

    cmd_append(cmd, "-L"OUT_DIR, "-lnob");
}

static bool build_nob_static(Cmd *cmd) {
    cmd->count = 0;

    __add_comp_n_flags(cmd);
    cmd_append(cmd, "-x", "c");
    cmd_append(cmd, "-D", "NOB_IMPLEMENTATION");
    cmd_append(cmd, "-c", "nob.h");
    cmd_append(cmd, "-o", OUT_DIR"nob.h.o");
    if (!cmd_run(cmd)) return false;

    cmd_append(cmd, "ar", "rcs");
    cmd_append(cmd, OUT_DIR"libnob.a");
    cmd_append(cmd, OUT_DIR"nob.h.o");
    if (!cmd_run(cmd)) return false;

    return true;
}

static bool build_perft(Cmd *cmd) {
    temp_reset();
    cmd->count = 0;

    __add_comp_n_flags(cmd);
    __add_libchess(cmd);
    cmd_append(cmd, "chess/perft.c");
    cmd_append(cmd, "-o", OUT_DIR"perft");

    return cmd_run(cmd);
}

static bool build_tests(Cmd *cmd) {
    cmd->count = 0;

    __add_comp_n_flags(cmd);
    __add_libchess(cmd);
    cmd_append(cmd, "tests/test_main.c");
    cmd_append(cmd, "-o", OUT_DIR"test_main");

    return cmd_run(cmd);
}

static bool build_tournament(Cmd *cmd) {
    cmd->count = 0;

    __add_comp_n_flags(cmd);
    __add_libchess(cmd);
    cmd_append(cmd, "tournament/comm_main.c");
    cmd_append(cmd, "-o", OUT_DIR"comm_main");

    return cmd_run(cmd);
}

static bool build_engine(Cmd *cmd) {
    cmd->count = 0;

    __add_comp_n_flags(cmd);
    __add_libchess(cmd);
    cmd_append(cmd, "./engine/eval.c");
    cmd_append(cmd, "./engine/search.c");
    cmd_append(cmd, "./engine/uci.c");
    cmd_append(cmd, "-o", OUT_DIR"steak");

    return cmd_run(cmd);
}

int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    const char *program_name = shift_args(&argc, &argv);
    while (argc > 0) {
        const char *arg = shift_args(&argc, &argv);
        if (!strcmp(arg, "release")) {
            BUILD = BM_RELEASE;
        } else if (!strcmp(arg, "debug")) {
            BUILD = BM_DEBUG;
        }
    }

    nob_log(INFO, "Build mode: %s", MODE_STR[BUILD]);
    nob_log(INFO, "Build output dir: "OUT_DIR);
    mkdir_if_not_exists(OUT_DIR);
    Cmd cmd = {0};

    if (nob_needs_rebuild1(OUT_DIR"libnob.a", "nob.h")) build_nob_static(&cmd);

    if (!build_perft(&cmd)) return 1;
    if (!build_tests(&cmd)) return 1;
    if (!build_tournament(&cmd)) return 1;
    if (!build_engine(&cmd)) return 1;

    cmd_free(cmd);
    return 0;
}
