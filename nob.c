#define NOB_IMPLEMENTATION
#include "nob.h"

#define C_COMP "clang"
#define OUT_DIR "build"
#define CHESS_UNITY "chess_unity"

enum BuildMode {
    BM_DEBUG,
    BM_RELEASE,
} BUILD = BM_DEBUG;
const char *MODE_STR[] = { "DEBUG", "RELEASE" };

typedef struct {
    const char *root;
    File_Paths srcs, headers;
} SourceHeaders;

bool walk_chess_func(Nob_Walk_Entry entry) {
    if (entry.type != FILE_REGULAR) return true;

    SourceHeaders *sh = (SourceHeaders*)entry.data;
    String_View sv = sv_from_cstr(entry.path);
    sv_chop_prefix(&sv, sv_from_cstr(temp_sprintf("%s/", sh->root)));

    // mabay: this is the auto-gen'd file; it should not be included in itself.
    if (sv_starts_with(sv, sv_from_cstr(CHESS_UNITY))) return true;

    // mabay: skip this because this is the file that will be compiled
    if (sv_starts_with(sv, sv_from_cstr("perft"))) return true;

    if (sv_ends_with_cstr(sv, ".c")) {
        da_append(&sh->srcs, temp_sv_to_cstr(sv));
    } else if (sv_ends_with_cstr(sv, ".h")) {
        da_append(&sh->headers, temp_sv_to_cstr(sv));
    }

    return true;
}

static void prep_unity(Cmd *cmd, const char *src_dir) {
    // Update unity source and header files
    SourceHeaders sh = { .root = src_dir };
    walk_dir(sh.root, &walk_chess_func, .data = &sh);

    String_Builder sb = {0};

    // Generated the unity source file
    sb_append_cstr(&sb,
        "// mabay: Do not modify manually; this is auto-generated.\n\n");
    for (int i = 0; i < sh.srcs.count; i++) {
        sb_appendf(&sb, "#include \"%s\" // %s:%d\n",
            sh.srcs.items[i], __FILE__, __LINE__);
    }
    write_entire_file(
        temp_sprintf("%s/%s.c", sh.root, CHESS_UNITY),
        sb.items, sb.count
    );

    // Generated the unity header file
    sb.count = 0;
    sb_append_cstr(&sb,
        "// mabay: Do not modify manually; this is auto-generated.\n");
    for (int i = 0; i < sh.headers.count; i++) {
        sb_appendf(&sb, "#include \"%s\" // %s:%d\n",
            sh.headers.items[i], __FILE__, __LINE__);
    }
    write_entire_file(
        temp_sprintf("%s/%s.h", sh.root, CHESS_UNITY),
        sb.items, sb.count
    );

    sb_free(sb);
}

void build_exe(Cmd *cmd, const char *input, const char *output) {
    // Rewind cmd
    cmd->count = 0;

    cmd_append(cmd, C_COMP);
    cmd_append(cmd, "-Wall", "-Wextra", "-pedantic");

    switch (BUILD) {
        case BM_DEBUG  : cmd_append(cmd, "-ggdb"); break;
        case BM_RELEASE: cmd_append(cmd, "-O3"); break;
    }

    temp_reset();
    cmd_append(cmd, "-o", temp_sprintf(OUT_DIR"/%s", output));
    cmd_append(cmd, input);
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
    nob_log(INFO, "Build output dir: "OUT_DIR"/");
    mkdir_if_not_exists(OUT_DIR);
    Cmd cmd = {0};
    prep_unity(&cmd, "chess");

    build_exe(&cmd, "chess/perft.c", "perft");
    if (!cmd_run(&cmd)) return 1;

    build_exe(&cmd, "tests/run_tests.c", "run_tests");
    if (!cmd_run(&cmd)) return 1;

    // build_exe(&cmd, "engine/uci.c", "steak-engine");
    // if (!cmd_run(&cmd)) return 1;

    cmd_free(cmd);
    return 0;
}
