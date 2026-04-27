#define NOB_IMPLEMENTATION
#include "../nob.h"
#include "engine.h"

#define INPUT_BUFSZ 8*1024

typedef struct {
    bool quit;
    Board board;
} UCI_Info;

#define DEFAULT_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

void uci_parse(String_View args, UCI_Info *info) {
    args = sv_trim(args);
    String_View first = sv_chop_by_delim(&args, ' ');

    if (sv_eq(first, sv_from_cstr("quit"))) {
        info->quit = true;
    } else if (sv_eq(first, sv_from_cstr("isready"))) {
        printf("readyok\n");
    } else if (sv_eq(first, sv_from_cstr("eval"))) {
        FENInfo f_info = parse_fen(
            // DEFAULT_FEN
            "rnbqkbnr/pppppppp/8/8/8/8/P1PPPPPP/RNBQKBNR w KQkq - 0 1"
        );
        board_set_from_fen(&info->board, f_info);
        printf("Eval: %d\n", evaluate(&info->board));
    } else if (sv_eq(first, sv_from_cstr("uci"))) {
        printf("id name <placeholder>\n");
        printf("id author michabay05\n");
        printf("uciok\n");
    }
}

int main(void) {
    char buffer[INPUT_BUFSZ] = {0};
    UCI_Info info = {
        .quit = false,
    };

    while (!info.quit) {
        memset(buffer, 0, INPUT_BUFSZ);
        fflush(stdout);

        if (!fgets(buffer, INPUT_BUFSZ - 1, stdin)) continue;
        uci_parse(sv_from_parts(buffer, strlen(buffer)), &info);
    }

    return 0;
}
