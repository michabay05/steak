#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../chess/chess_unity.h"
#include "util.c"

typedef enum {
    EST_CP = 1,
    EST_MATE,
} EngineScoreType;

typedef struct {
    int depth;
    EngineScoreType score_type;
    int score;
    int nodes;
    int time;
    String_View pv;
    String_View best_move;
} EngineOutput;

typedef struct {
    // Input and output file descriptors
    int read_fd;
    int write_fd;

    // Parse engine lines
    EngineOutput *output;
    size_t output_size;
    Move best_move;

    // UCI configuration
    String_View name;
    int threads;
    int min_threads;
    int max_threads;
    int hash_size;
    int min_hash_size;
    int max_hash_size;
    bool uci_ok;
} Engine;

typedef enum {
    IVT_DEPTH = 1,
    IVT_SCORE,
    IVT_NODES,
    IVT_TIME,
    IVT_PV_LINE,
    IVT_BEST_MOVE
} InfoValueType;

static void engine_add_output(Engine *engine, EngineOutput eo) {
    engine->output_size++;
    engine->output =
        (EngineOutput *)realloc(engine->output, engine->output_size * sizeof(EngineOutput));

    memcpy(&engine->output[engine->output_size - 1], &eo, sizeof(EngineOutput));
}


#if 0
static void parse_config(Engine *engine, String_View *sv) {
    char c;
    String_View temp = {0};
    String_View key = {0};
    while (sv->count > 0) {
        c = peek(*sv);
        if (isalnum(c)) {
            // id name Stockfish
            // id
            consume_while(&temp, sv, &is_alnum);
            if (sv_eq(temp, sv_from_cstr("uciok"))) {
                engine->uci_ok = true;
            } else if (sv_eq(temp, sv_from_cstr("id"))) {
                consume_while(NULL, sv, &is_space);
                // name
                consume_while(&temp, sv, &is_alnum);
                if (sv_eq(temp, sv_from_cstr("name"))) {
                    consume_while(NULL, sv, &is_space);
                    // <ENGINE_NAME>
                    consume_while(&engine->name, sv, &is_not_eol);
                } else {
                    consume_while(NULL, sv, &is_space);
                    consume_while(NULL, sv, &is_not_eol);
                    consume_while(NULL, sv, &is_space);
                }
            } else if (sv_eq(temp, sv_from_cstr("option"))) {
                // 'option name Threads type spin default 1 min 1 max 1024'
                // name
                consume_while(NULL, sv, &is_space);
                consume_while(&temp, sv, &is_alnum);
                if (!sv_eq(temp, sv_from_cstr("name"))) {
                    // if the word after `option` isn't `name`, then skip
                    // the entire line and move on
                    consume_while(NULL, sv, &is_not_eol);
                    continue;
                }
                consume_while(NULL, sv, &is_space);
                // Threads
                consume_while(&key, sv, &is_alnum);

                consume_while(NULL, sv, &is_space);
                // type
                consume_while(&temp, sv, &is_alnum);
                if (!sv_eq(temp, sv_from_cstr("type"))) {
                    consume_while(NULL, sv, &is_not_eol);
                    continue;
                }

                consume_while(NULL, sv, &is_space);
                // spin
                consume_while(&temp, sv, &is_alnum);
                if (!sv_eq(temp, sv_from_cstr("spin"))) {
                    consume_while(NULL, sv, &is_not_eol);
                    continue;
                }

                consume_while(NULL, sv, &is_space);
                // default
                consume_while(&temp, sv, &is_alnum);
                if (!sv_eq(temp, sv_from_cstr("default"))) {
                    consume_while(NULL, sv, &is_not_eol);
                    continue;
                }
                consume_while(NULL, sv, &is_space);
                // 1
                consume_while(&temp, sv, &is_alnum);
                int default_size = atoi(temp.data);

                consume_while(NULL, sv, &is_space);
                // min
                consume_while(&temp, sv, &is_alnum);
                if (!sv_eq(temp, sv_from_cstr("min"))) {
                    consume_while(NULL, sv, &is_not_eol);
                    continue;
                }
                consume_while(NULL, sv, &is_space);
                // 1
                consume_while(&temp, sv, &is_alnum);
                int min_size = atoi(temp.data);

                consume_while(NULL, sv, &is_space);
                // max
                consume_while(&temp, sv, &is_alnum);
                if (!sv_eq(temp, sv_from_cstr("max"))) {
                    consume_while(NULL, sv, &is_not_eol);
                    continue;
                }
                consume_while(NULL, sv, &is_space);
                // 1024
                consume_while(&temp, sv, &is_alnum);
                int max_size = atoi(temp.data);

                if (sv_eq(key, sv_from_cstr("Threads"))) {
                    engine->threads = default_size;
                    engine->min_threads = min_size;
                    engine->max_threads = max_size;
                } else if (sv_eq(key, sv_from_cstr("Hash"))) {
                    engine->hash_size = default_size;
                    engine->min_hash_size = min_size;
                    engine->max_hash_size = max_size;
                }
            }
        } else if (isspace(c)) {
            consume_while(NULL, sv, &is_space);
        }
    }

    printf("[INFO]          name = '"SV_Fmt"'\n", SV_Arg(engine->name));
    printf("[INFO]       threads = '%d'\n", engine->threads);
    printf("[INFO]   min_threads = '%d'\n", engine->min_threads);
    printf("[INFO]   max_threads = '%d'\n", engine->max_threads);
    printf("[INFO]     hash_size = '%d'\n", engine->hash_size);
    printf("[INFO] min_hash_size = '%d'\n", engine->min_hash_size);
    printf("[INFO] max_hash_size = '%d'\n", engine->max_hash_size);
}
#endif

void read_from_engine(Engine engine, char *buf, size_t size) { read(engine.read_fd, buf, size); }

void send_to_engine(Engine engine, char *command) {
    write(engine.write_fd, command, strlen(command));
}

// This function is defined at the end of this file
// Its prototype is here because it's used in the function below
static int bi_popen(const char *const command, int *const in, int *const out);

bool load_engine(const char *filepath, Engine *engine) {
    *engine = (Engine){0};
    const int pid = bi_popen(filepath, &engine->read_fd, &engine->write_fd);
    if (pid < 0) {
        perror("bi_popen");
        return false;
    }

    if (engine->read_fd == INVALID_FD || engine->write_fd == INVALID_FD)
        return false;

    const int BUF_SIZE = 8 * 1024;
    char buf[8 * 1024] = {0};

    // Read whatever the engine prints out at the beginning
    // Don't need it
    read_from_engine(*engine, buf, BUF_SIZE);

    send_to_engine(*engine, "uci\n");
    // TODO: come back here and remove this with actual parsing
    // from the engine's output
    engine->uci_ok = true;

    memset(buf, 0, BUF_SIZE);
    read_from_engine(*engine, buf, BUF_SIZE);

    // Nob_String_View sv_buf = nob_sv_from_parts(buf, strlen(buf));
    // parse_config(engine, &sv_buf);

    send_to_engine(*engine, "isready\n");

    memset(buf, 0, BUF_SIZE);
    read_from_engine(*engine, buf, BUF_SIZE);

    // If engine doesn't respond with 'readyok', then
    // the engine is pressumed to be unprepared
    return engine->uci_ok && (!strncmp(buf, "readyok", 7));
}

void unload_engine(Engine engine) {
    close(engine.read_fd);
    close(engine.write_fd);
    free(engine.output);
}

static bool identify_info(String_View info_key, InfoValueType *ivt) {
    struct {
        const char *key_str;
        InfoValueType key_ivt;
    } matches[] = {
        {"depth", IVT_DEPTH},
        {"score", IVT_SCORE},
        {"nodes", IVT_NODES},
        {"time", IVT_TIME},
        {"pv", IVT_PV_LINE},
        {"bestmove", IVT_BEST_MOVE}
    };

    int n = (sizeof(matches) / sizeof(matches[0]));
    for (int i = 0; i < n; i++) {
        const char *key = matches[i].key_str;
        if (sv_eq(info_key, sv_from_cstr(key))) {
            *ivt = matches[i].key_ivt;
            return true;
        }
    }

    return false;
}

void parse_engine_output0(String_View *sv, Engine *engine) {
    printf("[INFO] Initial string: '"SV_Fmt"'\n", SV_Arg(*sv));

    String_View temp = {0};
    String_View key = {0};
    String_View value = {0};
    int is_mate_score = 0;
    InfoValueType ivt;
    EngineOutput output = {0};
    while (sv->count > 0) {
        char c = peek(*sv);
        if (isalnum(c)) {
            if (sv_eq(key, sv_from_cstr("pv"))) {
                consume_while(&temp, sv, &is_not_eol);
            } else {
                consume_while(&temp, sv, &is_alnum);
                if (sv_eq(temp, sv_from_cstr("info"))) {
                    continue;
                }
                if (sv_eq(temp, sv_from_cstr("string"))) {
                    consume_while(&temp, sv, &is_not_eol);
                    continue;
                }
                if (sv_eq(temp, sv_from_cstr("mate"))
                    || sv_eq(temp, sv_from_cstr("cp"))
                ) {
                    is_mate_score = sv_eq(temp, sv_from_cstr("mate"))
                        ? 1 : 0;
                    continue;
                }
            }
            if (key.count == 0) {
                key = temp;
                continue;
            } else {
                value = temp;
            }
            if (identify_info(key, &ivt)) {
                switch (ivt) {
                case IVT_DEPTH:
                    output.depth = atoi(value.data);
                    break;
                case IVT_NODES:
                    output.nodes = atoi(value.data);
                    break;
                case IVT_SCORE:
                    output.score_type = is_mate_score ? EST_MATE : EST_CP;
                    output.score = atoi(value.data);
                    break;
                case IVT_TIME:
                    output.time = atoi(value.data);
                    break;
                case IVT_PV_LINE:
                    output.pv = value;
                    // Append parsed output to save in a list
                    engine_add_output(engine, output);
                    // Reset output for next line
                    output = (EngineOutput){0};
                    break;

                case IVT_BEST_MOVE:
                    TODO("Unimplemented; implement ivt best move");
                }
                key.count = 0;
                value.count = 0;
            } else {
                // Unknown key; ignored
                key.count = 0;
            }
        } else if (isspace(c)) {
            consume_while(&temp, sv, &is_space);
        }
    }

    temp.count = 0;
}

// ========================================================================================================================
// Source of the 'bi_popen()' function:
//      https://unix.stackexchange.com/questions/606861/programming-communicating-with-chess-engine-stockfish-fifos-bash-redirecti
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
