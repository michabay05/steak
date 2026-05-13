#include <stdlib.h>
#include "engine.h"
#include "move.h"
#include "move_gen.h"
#include "precalculate.h"
#include "tt.h"

#define INPUT_BUFSZ 8*1024

UCI_Info U_INFO = {
    .quit = false,
    .movestogo = 30,
    .movetime = -1,
    .time = -1,
    .inc = 0,
    .starttime = 0,
    .stoptime = 0,
    .timeset = false,
    .stopped = false,
};

#define DEFAULT_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
#define ENGINE_NAME "steak"
#define ENGINE_VERSION "0.1"

static int sv_index(String_View haystack, String_View needle) {
    if (haystack.count < needle.count) return -1;

    int n = haystack.count - needle.count;
    for (int i = 0; i < n; i++) {
        String_View temp = sv_chop_left(&haystack, 1);

        bool found = true;
        for (u32 k = 0; found && k < needle.count; k++) {
            if (temp.data[k] != needle.data[k]) found = false;
        }
        if (found) return i;
    }
    return -1;
}

static void _uci_parse_position(String_View args) {
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
    board_parse_fen_sv(&U_INFO.board, fen_sv);

    if (moves_ind < 0) return;

    sv_chop_by_delim(&args, ' ');
    MoveList ml = {0};
    while (args.count > 0) {
        String_View msv = sv_chop_by_delim(&args, ' ');
        Move parsed_move = move_parse_sv(msv);

        ml.count = 0;
        movelist_legal(&ml, &U_INFO.board);

        int ind = movelist_search(ml,
            parsed_move.source, parsed_move.target, parsed_move.promoted);
        if (ind < 0) {
            movelist_print_list(ml);
            fprintf(stderr, "Illegal move: '"SV_Fmt"'\n", SV_Arg(msv));
            fprintf(stderr, "Stopping move sequence parsing\n");
            break;
        }
        move_make(&U_INFO.board, ml.list[ind], AllMoves);
    }
}


// Example command:
// - go depth 6 wtime 60000 btime 60000 winc 1000 binc 1000 movtime 1000 movestogo 40
static void _uci_parse_go(String_View args) {
    args = sv_trim(args);
    U_INFO.depth = -1;

    while (args.count > 0) {
        String_View key = sv_chop_by_delim(&args, ' ');
        String_View val = sv_chop_by_delim(&args, ' ');

        if (key.count == 0 || val.count == 0) break;

        int num = atoi(val.data);

        if (sv_eq(key, sv_from_cstr("depth"))) {
            U_INFO.depth = num;
        } else if (sv_eq(key, sv_from_cstr("wtime")) && U_INFO.board.side == C_WHITE) {
            U_INFO.time = num;
        } else if (sv_eq(key, sv_from_cstr("btime")) && U_INFO.board.side == C_BLACK) {
            U_INFO.time = num;
        } else if (sv_eq(key, sv_from_cstr("winc")) && U_INFO.board.side == C_WHITE) {
            U_INFO.inc = num;
        } else if (sv_eq(key, sv_from_cstr("binc")) && U_INFO.board.side == C_BLACK) {
            U_INFO.inc = num;
        } else if (sv_eq(key, sv_from_cstr("movetime"))) {
            U_INFO.movetime = num;
        } else if (sv_eq(key, sv_from_cstr("movestogo"))) {
            U_INFO.movestogo = num;
        }
    }

    if (U_INFO.movetime != -1) {
        U_INFO.time = U_INFO.movetime;
        U_INFO.movestogo = 1;
    }

    U_INFO.starttime = nanos_since_unspecified_epoch() / (1000 * 1000);

    if (U_INFO.time != -1) {
        U_INFO.timeset = true;
        U_INFO.time /= U_INFO.movestogo;
        U_INFO.time -= 50;
        U_INFO.stoptime = U_INFO.starttime + U_INFO.time + U_INFO.inc;
    }

    if (U_INFO.depth == -1) U_INFO.depth = MAX_PLY;

    search_position(&U_INFO.board, U_INFO.depth);
}

void uci_parse(String_View args) {
    args = sv_trim(args);
    String_View first = sv_chop_by_delim(&args, ' ');
    first = sv_trim(first);

    if (sv_eq(first, sv_from_cstr("quit"))) {
        U_INFO.quit = true;
    } else if (sv_eq(first, sv_from_cstr("isready"))) {
        printf("readyok\n");
    } else if (sv_eq(first, sv_from_cstr("eval"))) {
        printf("Eval: %+d cp\n", evaluate(&U_INFO.board));
    } else if (sv_eq(first, sv_from_cstr("uci"))) {
        printf("id name %s v%s\n", ENGINE_NAME, ENGINE_VERSION);
        printf("id author michabay05\n");
        printf("uciok\n");
    } else if (sv_eq(first, sv_from_cstr("d"))) {
        board_print(&U_INFO.board);
    } else if (sv_eq(first, sv_from_cstr("ucinewgame"))) {
        // TODO: ucinewgame indicates the start of a new game. Find out what could be done upon this
        // message being received. Maybe clearing the transposition table, in the future.
        _uci_parse_position(sv_from_cstr("startpos"));
        tt_clear();
    } else if (sv_eq(first, sv_from_cstr("position"))) {
        _uci_parse_position(args);
    } else if (sv_eq(first, sv_from_cstr("go"))) {
        _uci_parse_go(args);
    }
}

int main(void) {
    attack_init();
    zobrist_init();
    tt_clear();

    char buffer[INPUT_BUFSZ] = {0};
    uci_parse(sv_from_cstr("uci"));

    while (!U_INFO.quit) {
        memset(buffer, 0, INPUT_BUFSZ);
        fflush(stdout);

        if (!fgets(buffer, INPUT_BUFSZ - 1, stdin)) break;
        uci_parse(sv_from_cstr(buffer));
    }

    return 0;
}
