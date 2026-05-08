#include <stdio.h>
#include <unistd.h>

#define NOB_IMPLEMENTATION
#include "comm.c"

#include "../chess/chess_unity.c"

#define BUF_SIZE 64*1024

static void eo__parse_info_line(String_View *args) {
    while (args->count > 0) {
        String_View arg = sv_chop_by_delim(args, ' ');
        arg = sv_trim(arg);
        if (sv_eq(arg, sv_from_cstr("string"))) {
            sv_chop_while(args, &is_not_eol);
            return;
        } else if (sv_eq(arg, sv_from_cstr("pv"))) {
            sv_chop_while(args, &is_not_eol);
            // printf("\tPV: '"SV_Fmt"'\n", SV_Arg(pv_line));
            return;
        } else if (sv_eq(arg, sv_from_cstr("score"))) {
            sv_chop_by_delim(args, ' ');
            sv_chop_by_delim(args, ' ');
            // printf("\ttype: '"SV_Fmt"', value: '"SV_Fmt"'\n", SV_Arg(type), SV_Arg(value));
        } else if (sv_eq(arg, sv_from_cstr("depth"))) {
            sv_chop_by_delim(args, ' ');
            // printf("\tDepth: %d\n", atoi(value.data));
        } else if (sv_eq(arg, sv_from_cstr("nodes"))) {
            sv_chop_by_delim(args, ' ');
            // printf("\tNodes: %d\n", atoi(value.data));
        } else if (sv_eq(arg, sv_from_cstr("nps"))) {
            sv_chop_by_delim(args, ' ');
            // printf("\tnps: %d\n", atoi(value.data));
        } else if (sv_eq(arg, sv_from_cstr("time"))) {
            sv_chop_by_delim(args, ' ');
            // printf("\ttime: %d\n", atoi(value.data));
        } else if (
            sv_eq(arg, sv_from_cstr("seldepth"))
            || sv_eq(arg, sv_from_cstr("multipv"))
            || sv_eq(arg, sv_from_cstr("hashfull"))
            || sv_eq(arg, sv_from_cstr("tbhits"))
        ) {
            sv_chop_by_delim(args, ' ');
            // Ignored for now
        } else {
            printf(">>> '"SV_Fmt"'\n", SV_Arg(arg));
            TODO("not done here!");
        }
    }
}

static void eo_parse_go(String_View sv) {
    while (sv.count > 0) {
        String_View arg = sv_chop_by_delim(&sv, ' ');
        arg = sv_trim(arg);
        if (sv_eq(arg, sv_from_cstr("info"))) {
            eo__parse_info_line(&sv);
        } else if (sv_eq(arg, sv_from_cstr("bestmove"))) {
            String_View bm_sv = sv_chop_by_delim(&sv, ' ');
            bm_sv = sv_trim(bm_sv);

            Move best_move = move_parse_sv(bm_sv);
            char buf[6] = {0};
            move_to_str(best_move, buf);
            printf("\tBestmove (sv): '"SV_Fmt"'\n", SV_Arg(bm_sv));
            printf("\tBestmove (mv): '%s'\n", buf);
        } else if (sv_eq(arg, sv_from_cstr("ponder"))) {
            String_View ponder_sv = sv_chop_by_delim(&sv, ' ');
            ponder_sv = sv_trim(ponder_sv);

            Move best_move = move_parse_sv(ponder_sv);
            char buf[6] = {0};
            move_to_str(best_move, buf);
            printf("\tPonder (sv): '"SV_Fmt"'\n", SV_Arg(ponder_sv));
            printf("\tPonder (mv): '%s'\n", buf);
        } else {
            printf(">>> '"SV_Fmt"'\n", SV_Arg(arg));
            UNREACHABLE("unknown arg");
        }
    }

}


int main(int argc, char **argv) {
    const char *program_name = shift_args(&argc, &argv);
    if (argc == 0) {
        fprintf(stderr, "Usage: %s <ENGINE_PATH>\n", program_name);
        return 1;
    }
    const char *engine_path = shift_args(&argc, &argv);

    Engine engine = {0};
    // char *engine_path = "../stockfish-ubuntu-x86-64-avx2";
    if (!load_engine(engine_path, &engine)) {
        fprintf(stderr, "Failed to load engine!\n");
        return 1;
    }

    char buf[BUF_SIZE] = {0};
    const char *fen_strs[] = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
    };

    for (size_t i = 0; i < ARRAY_LEN(fen_strs); i++) {
        memset(buf, 0, BUF_SIZE);

        send_to_engine(engine,
            temp_sprintf("position fen %s\n", fen_strs[i])
        );
        send_to_engine(engine, "go depth 6\n");
        printf("Started waiting!\n");
        sleep(3);
        printf("Done waiting!\n");

        read_from_engine(engine, buf, BUF_SIZE);
        printf("%s\n", buf);
        String_View sv = sv_from_cstr(buf);
        eo_parse_go(sv);
    }

    return 0;
}
