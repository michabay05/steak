#include <stdlib.h>
#define NOB_IMPLEMENTATION
#include "engine.h"

#define INPUT_BUFSZ 8*1024

typedef struct {
    bool quit;
    Board board;
} UCI_Info;

#define DEFAULT_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
#define ENGINE_NAME "steak"
#define ENGINE_VERSION "1.0"

static int sv_index(String_View haystack, String_View needle) {
    if (haystack.count < needle.count) return -1;

    int n = haystack.count - needle.count;
    for (int i = 0; i < n; i++) {
        String_View temp = sv_chop_left(&haystack, 1);

        bool found = true;
        for (int k = 0; found && k < needle.count; k++) {
            if (temp.data[k] != needle.data[k]) found = false;
        }
        if (found) return i;
    }
    return -1;
}

void _parse_position(UCI_Info *info, String_View args) {
    args = sv_trim(args);
    String_View cmd = sv_chop_by_delim(&args, ' ');
    cmd = sv_trim(cmd);

    String_View fen_sv = {0};
    int moves_ind = sv_index(args, sv_from_cstr("moves"));

    if (sv_eq(cmd, sv_from_cstr("startpos"))) {
        fen_sv = sv_from_cstr(DEFAULT_FEN);
    } else if (sv_eq(cmd, sv_from_cstr("fen"))) {
        int fen_r_end = moves_ind >= 0 ? moves_ind : args.count;
        fen_sv = sv_chop_left(&args, fen_r_end);
    } else {
        fprintf(stderr, "Unknown `position` command: "SV_Fmt, SV_Arg(cmd));
        return;
    }

    if (fen_sv.count == 0) return;

    FENInfo fen_info = parse_fen_sv(fen_sv);
    board_set_from_fen(&info->board, fen_info);

    if (moves_ind < 0) return;

    String_View moves_sv = sv_chop_by_delim(&args, ' ');
    MoveList ml = {0};
    while (args.count > 0) {
        String_View msv = sv_chop_by_delim(&args, ' ');
        Move parsed_move = move_parse_sv(msv);

        ml.count = 0;
        movelist_legal(&ml, &info->board);

        int ind = movelist_search(ml,
            parsed_move.source, parsed_move.target, parsed_move.promoted);
        if (ind < 0) {
            fprintf(stderr, "Illegal move: '"SV_Fmt"'\n", SV_Arg(msv));
            fprintf(stderr, "Stopping move sequence parsing\n");
            break;
        }
        move_make(&info->board, ml.list[ind], AllMoves);
    }
}

void _parse_go(UCI_Info *info, String_View args) {
    args = sv_trim(args);
    String_View cmd = sv_chop_by_delim(&args, ' ');
    cmd = sv_trim(cmd);

    if (sv_eq(cmd, sv_from_cstr("depth"))) {
        String_View val = sv_chop_by_delim(&args, ' ');
        val = sv_trim(val);
        search_position(&info->board, atoi(val.data));
    }
}

void uci_parse(UCI_Info *info, String_View args) {
    args = sv_trim(args);
    String_View first = sv_chop_by_delim(&args, ' ');
    first = sv_trim(first);

    if (sv_eq(first, sv_from_cstr("quit"))) {
        info->quit = true;
    } else if (sv_eq(first, sv_from_cstr("isready"))) {
        printf("readyok\n");
    } else if (sv_eq(first, sv_from_cstr("eval"))) {
        printf("Eval: %+d cp\n", evaluate(&info->board));
    } else if (sv_eq(first, sv_from_cstr("uci"))) {
        printf("id name %s v%s\n", ENGINE_NAME, ENGINE_VERSION);
        printf("id author michabay05\n");
        printf("uciok\n");
    } else if (sv_eq(first, sv_from_cstr("debug"))) {
        board_print(&info->board);
    } else if (sv_eq(first, sv_from_cstr("position"))) {
        _parse_position(info, args);
    } else if (sv_eq(first, sv_from_cstr("go"))) {
        _parse_go(info, args);
    }
}

int main(void) {
    attack_init();
    char buffer[INPUT_BUFSZ] = {0};
    UCI_Info info = {
        .quit = false,
    };

    while (!info.quit) {
        memset(buffer, 0, INPUT_BUFSZ);
        fflush(stdout);

        if (!fgets(buffer, INPUT_BUFSZ - 1, stdin)) {
            fprintf(stderr, "Unable to get read input from stdin.\n");
            break;
        }
        uci_parse(&info, sv_from_parts(buffer, strlen(buffer)));
    }

    return 0;
}
