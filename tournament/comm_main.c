#include <stdio.h>
#include <unistd.h>

#include "../nob.h"
#include "board.h"
#include "move.h"
#include "move_gen.h"
#include "precalculate.h"

#include "pgn.h"

#define BUF_SIZE 64 * 1024

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

typedef Pipe Engine;

PGN_GameResult game_check_state(MoveList *ml, Board *board) {
    ml->count = 0;
    movelist_legal(ml, board);
    if (ml->count > 0) return PGN_GR_ONGOING;

    if (board_is_sq_attacked(board,
        board->piece[board->side][PT_KING],
        board->side ^ 1
    )) return board->side == C_WHITE ? PGN_GR_BLACK_WINS : PGN_GR_WHITE_WINS;
    else return PGN_GR_DRAW;
}

int engine_read(Engine engine, char *buf, size_t size);
int engine_write(Engine engine, char *command);
bool engine_load(const char *filepath, Engine *engine);
void engine_unload(Engine engine);

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
    char *start_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    MoveList temp_ml = {0};
    String_Builder temp = {0};
    String_Builder history = {0};
    Game game = {
        .time_per_move_s = 1.f,
        .white_name = sv_from_cstr("??"),
        .black_name = sv_from_cstr("??"),
        .state = PGN_GR_ONGOING,
        .start_fen = start_fen
    };
    board_parse_fen_cstr(&game.board, start_fen);
    Engine *current = &engine_a;
    bool is_a_current = true;

    for (size_t move_counter = 0; game.state == PGN_GR_ONGOING; move_counter++) {
        memset(temp_buf, 0, BUF_SIZE);
        temp.count = 0;
        temp_ml.count = 0;

        sb_appendf(&temp, "position fen %s", start_fen);
        if (history.count > 0) {
            sb_append_cstr(&temp, " moves ");
            sb_append_buf(&temp, history.items, history.count);
        }
        sb_append(&temp, '\n');
        sb_appendf(&temp, "go movetime %d\n", (u32)(game.time_per_move_s * 1000.f));

        printf("%zu: [%s] << %s\n", move_counter + 1, is_a_current ? engine_a_path : engine_b_path, temp.items);
        engine_write(*current, temp.items);

        usleep((u32)((game.time_per_move_s + 0.1) * 1e6));

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
        char move_buf[7] = {0};
        move_to_str(best_move, move_buf);
        printf("[%s] >> '%s'\n", is_a_current ? engine_a_path : engine_b_path, move_buf);

        movelist_generate_all(&temp_ml, &game.board);
        int ind = movelist_search(temp_ml, best_move.source, best_move.target, best_move.promoted);
        if (ind < 0) {
            fprintf(stderr, "[ERROR] Did not find move among legal moves: '%s'\n", move_buf);
            movelist_print_list(temp_ml);
            break;
        }

        if (!move_make(&game.board, temp_ml.list[ind], AllMoves)) {
            UNREACHABLE("[ERROR] Illegal move.");
        }

        da_append(&game.history, temp_ml.list[ind]);
        if (move_counter > 0) sb_append(&history, ' ');
        sb_appendf(&history, "%s", move_buf);

        current = is_a_current ? &engine_b : &engine_a;
        is_a_current = !is_a_current;

        game.state = game_check_state(&temp_ml, &game.board);
    }

    printf("Game state = %d\n", game.state);
    board_print(&game.board);

    temp.count = 0;
    pgn_export(&game, &temp);
    write_entire_file("sf_vs_steak.pgn", temp.items, temp.count);

    sb_free(temp);
    sb_free(history);
    return 0;
}

int engine_read(Engine engine, char *buf, size_t size) {
    return read(engine.read, buf, size);
}

int engine_write(Engine engine, char *command) {
    return write(engine.write, command, strlen(command));
}

// This function is defined at the end of this file
// Its prototype is here because it's used in the function below
static int bi_popen(const char *const command, int *const in, int *const out);

bool engine_load(const char *filepath, Engine *engine) {
    *engine = (Engine){0};
    const int pid = bi_popen(filepath, &engine->read, &engine->write);
    if (pid < 0) {
        perror("bi_popen");
        return false;
    }

    if (engine->read == INVALID_FD || engine->write == INVALID_FD) {
        fprintf(stderr, "Invalid fd: (read = %d, write = %d)\n", engine->read, engine->write);
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
    close(engine.read);
    close(engine.write);
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
