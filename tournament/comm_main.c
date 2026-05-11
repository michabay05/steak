#include <stdio.h>
#include <unistd.h>

#include "../nob.h"
#include "board.h"
#include "move.h"
#include "move_gen.h"
#include "precalculate.h"

#define BUF_SIZE 64 * 1024

int is_space(int c) { return isspace(c); }
// is alphabetic or numeric
int is_alnum(int c) { return isalnum(c); }
// is not end-of-line
int is_not_eol(int c) { return c != '\r' && c != '\n'; }

static void eo__parse_info_line(String_View *args) {
    while (args->count > 0) {
        String_View arg = sv_chop_by_delim(args, ' ');
        arg = sv_trim(arg);
        if (sv_eq(arg, sv_from_cstr("string"))) {
            sv_chop_while(args, &is_not_eol);
        } else if (sv_eq(arg, sv_from_cstr("pv"))) {
            sv_chop_while(args, &is_not_eol);
            // String_View pv_line = sv_chop_while(args, &is_not_eol);
            // printf("\tPV: '"SV_Fmt"'\n", SV_Arg(pv_line));
        } else {
            // Unknown things must be ignored according to the UCI protocol
            sv_chop_by_delim(args, ' ');
            // printf(">>> '" SV_Fmt "'\n", SV_Arg(arg));
            // TODO("not done here!");
        }
    }
}

static void eo_parse_go(String_View sv, Move *best_move) {
    while (sv.count > 0) {
        String_View arg = sv_chop_by_delim(&sv, ' ');
        arg = sv_trim(arg);
        if (sv_eq(arg, sv_from_cstr("info"))) {
            String_View info_line = sv_chop_by_delim(&sv, '\n');
            eo__parse_info_line(&info_line);
        } else if (sv_eq(arg, sv_from_cstr("bestmove"))) {
            String_View bm_sv = sv_chop_by_delim(&sv, ' ');
            bm_sv = sv_trim(bm_sv);
            *best_move = move_parse_sv(bm_sv);
        } else if (sv_eq(arg, sv_from_cstr("ponder"))) {
            sv_chop_by_delim(&sv, ' ');
        } else {
            // Unknown things must be ignored silently according to the UCI protocol
            sv_chop_by_delim(&sv, ' ');
        }
    }
}

typedef struct {
    // Input and output file descriptors
    int read_fd;
    int write_fd;
} Engine;

typedef enum {
    GS_ONGOING,
    GS_WHITE_WINS,
    GS_BLACK_WINS,
    GS_DRAW,
} GameState;

GameState check_game_state(MoveList *ml, Board *board) {
    ml->count = 0;
    movelist_legal(ml, board);
    if (ml->count > 0) return GS_ONGOING;

    movelist_print_list(*ml);

    if (board_is_sq_attacked(board,
        board->piece[board->side][PT_KING],
        board->side ^ 1
    )) return board->side == C_WHITE ? GS_BLACK_WINS : GS_WHITE_WINS;
    else return GS_DRAW;
}

int engine_read(Engine engine, char *buf, size_t size);
int engine_write(Engine engine, char *command);
bool engine_load(const char *filepath, Engine *engine);
void engine_unload(Engine engine);

int main2(void) {
    const char *fen = "r2qk2r/pppbp3/2n3NB/3pP3/3P4/2P5/PP4PP/RN1QK2R b KQkq - 0 12";
    attack_init();

    Board b = {0};
    board_parse_fen_cstr(&b, fen);

    MoveList ml = {0};
    movelist_generate_all(&ml, &b);
    movelist_print_list(ml);

    movelist_legal(&ml, &b);
    movelist_print_list(ml);
    return 0;
}

int main(int argc, char **argv) {
    const char *program_name = shift_args(&argc, &argv);
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <ENGINE_A_PATH> <ENGINE_B_PATH>\n", program_name);
        return 1;
    }
    const char *engine_a_path = shift_args(&argc, &argv);
    const char *engine_b_path = shift_args(&argc, &argv);
    printf("Engine A path: '%s'\n", engine_a_path);
    printf("Engine B path: '%s'\n", engine_b_path);

    Engine engine_a = {0}, engine_b = {0};
    if (!engine_load(engine_a_path, &engine_a)) {
        fprintf(stderr, "Failed to load engine a!\n");
        return 1;
    }

    if (!engine_load(engine_b_path, &engine_b)) {
        fprintf(stderr, "Failed to load engine b!\n");
        return 1;
    }

    attack_init();

    char temp_buf[BUF_SIZE] = {0};
    const char *start_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    u32 time_per_move = 1000;
    Board board = {0};
    board_parse_fen_cstr(&board, start_fen);
    String_Builder input = {0};
    String_Builder history = {0};
    MoveList ml = {0};
    GameState state = GS_ONGOING;

    Engine *current = &engine_a;
    bool is_a_current = true;

    for (size_t i = 0; state == GS_ONGOING; i++) {
        memset(temp_buf, 0, BUF_SIZE);

        input.count = 0;
        ml.count = 0;

        sb_appendf(&input, "position fen %s", start_fen);
        if (history.count > 0) {
            sb_append_cstr(&input, " moves ");
            sb_append_buf(&input, history.items, history.count);
        }
        sb_append(&input, '\n');
        sb_appendf(&input, "go movetime %d\n", time_per_move);

        printf("%zu: [%s] << %s\n", i + 1, is_a_current ? engine_a_path : engine_b_path, input.items);
        engine_write(*current, input.items);

        usleep((u32)(1.1 * 1e6));

        if (engine_read(*current, temp_buf, BUF_SIZE) == 0) {
            fprintf(stderr, "[ERROR] Failed to communicate properly.\n");
            break;
        }

        // printf(">>############################################################>>\n");
        // printf("%s\n", temp_buf);
        // printf("<<############################################################<<\n");

        Move best_move = {0};
        eo_parse_go(sv_from_cstr(temp_buf), &best_move);

        // Validate move
        char move_buf[6] = {0};
        move_to_str(best_move, move_buf);
        printf("[%s] >> '%s'\n", is_a_current ? engine_a_path : engine_b_path, move_buf);

        movelist_generate_all(&ml, &board);
        int ind = movelist_search(ml, best_move.source, best_move.target, best_move.promoted);
        if (ind < 0) {
            fprintf(stderr, "[ERROR] Did not find move among legal moves: '%s'\n", move_buf);
            movelist_print_list(ml);
            break;
        }

        for (int i = 0; i < ml.count; i++) {
            Move move = ml.list[i];
            if (move_eq(move, best_move) && move_make(&board, move, AllMoves)) {
                break;
            }
        }

        if (i > 0) sb_append(&history, ' ');
        sb_appendf(&history, "%s", move_buf);

        current = is_a_current ? &engine_b : &engine_a;
        is_a_current = !is_a_current;

        state = check_game_state(&ml, &board);
    }

    printf("Game state = %d\n", state);
    board_print(&board);

    sb_free(input);
    sb_free(history);
    return 0;
}

int engine_read(Engine engine, char *buf, size_t size) {
    return read(engine.read_fd, buf, size);
}

int engine_write(Engine engine, char *command) {
    return write(engine.write_fd, command, strlen(command));
}

// This function is defined at the end of this file
// Its prototype is here because it's used in the function below
static int bi_popen(const char *const command, int *const in, int *const out);

bool engine_load(const char *filepath, Engine *engine) {
    *engine = (Engine){0};
    const int pid = bi_popen(filepath, &engine->read_fd, &engine->write_fd);
    if (pid < 0) {
        perror("bi_popen");
        return false;
    }

    if (engine->read_fd == INVALID_FD || engine->write_fd == INVALID_FD) {
        fprintf(stderr, "Invalid fd: (read = %d, write = %d)\n", engine->read_fd, engine->write_fd);
        return false;
    }

    char buf[BUF_SIZE] = {0};

    // Read whatever the engine prints out at the beginning; discard it
    engine_read(*engine, buf, BUF_SIZE);

    engine_write(*engine, "uci\n");
    memset(buf, 0, BUF_SIZE);
    engine_read(*engine, buf, BUF_SIZE);

    // Nob_String_View sv_buf = nob_sv_from_parts(buf, strlen(buf));
    // parse_config(engine, &sv_buf);

    engine_write(*engine, "isready\n");

    memset(buf, 0, BUF_SIZE);
    engine_read(*engine, buf, BUF_SIZE);

    // If engine doesn't respond with 'readyok', then
    // the engine is pressumed to be unprepared
    return !strncmp(buf, "readyok", 7);
}

void engine_unload(Engine engine) {
    close(engine.read_fd);
    close(engine.write_fd);
}

// ========================================================================================================================
// Source of the 'bi_popen()' function:
//  - https://unix.stackexchange.com/questions/606861/programming-communicating-with-chess-engine-stockfish-fifos-bash-redirecti
static int bi_popen(const char *const command, int *const in, int *const out) {
    const int READ_END = 0;
    const int WRITE_END = 1;

    int to_child[2] = {INVALID_FD, INVALID_FD};
    int to_parent[2] = {INVALID_FD, INVALID_FD};

    *in = INVALID_FD;
    *out = INVALID_FD;

    if (command == NULL || in == NULL || out == NULL) {
        errno = EINVAL;
        goto bail;
    }

    if (pipe(to_child) < 0) {
        goto bail;
    }

    if (pipe(to_parent) < 0) {
        goto bail;
    }

    const int pid = fork();
    if (pid < 0) {
        goto bail;
    }

    if (pid == 0) { // Child
        if (dup2(to_child[READ_END], STDIN_FILENO) < 0) {
            perror("dup2");
            exit(1);
        }
        close(to_child[READ_END]);
        close(to_child[WRITE_END]);

        if (dup2(to_parent[WRITE_END], STDOUT_FILENO) < 0) {
            perror("dup2");
            exit(1);
        }
        close(to_parent[READ_END]);
        close(to_parent[WRITE_END]);

        execlp(command, command, NULL);
        perror("execlp");
        exit(1);
    }

    // Parent
    close(to_child[READ_END]);
    to_child[READ_END] = INVALID_FD;

    close(to_parent[WRITE_END]);
    to_parent[WRITE_END] = INVALID_FD;

    *in = to_parent[READ_END];
    *out = to_child[WRITE_END];

    return pid;

bail:; // Goto label must be a statement, this is an empty statement
    const int old_errno = errno;

    for (int i = 0; i < 2; ++i) {
        if (to_child[i] != INVALID_FD) {
            close(to_child[i]);
        }
        if (to_parent[i] != INVALID_FD) {
            close(to_parent[i]);
        }
    }

    errno = old_errno;
    return -1;
}
