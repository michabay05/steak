#define NOB_IMPLEMENTATION
#include "../nob.h"

#define INPUT_BUFSZ 8*1024

typedef struct {
    bool quit;
} UCI_Info;

void uci_parse(String_View args, UCI_Info *info) {
    args = sv_trim(args);
    String_View first = sv_chop_by_delim(&args, ' ');

    if (sv_eq(first, sv_from_cstr("quit"))) {
        info->quit = true;
    } else if (sv_eq(first, sv_from_cstr("isready"))) {
        printf("readyok\n");
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
