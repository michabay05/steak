#pragma once

#include "../chess/move.h"

typedef enum {
    EST_CP = 1,
    EST_MATE,
} EngineScoreType;

typedef struct
{
    int depth;
    EngineScoreType score_type;
    int score;
    int nodes;
    int time;
    char *pv;
} EngineOutput;

typedef struct
{
    // Input and output file descriptors
    int read_fd;
    int write_fd;

    // Parse engine lines
    EngineOutput *output;
    size_t output_size;
    Move best_move;

    // UCI configuration
    char *name;
    int threads;
    int min_threads;
    int max_threads;
    int hash_size;
    int min_hash_size;
    int max_hash_size;
    bool uci_ok;
} Engine;

bool load_engine(const char *filepath, Engine *engine);
void unload_engine(Engine engine);
void read_from_engine(Engine engine, char *buf, size_t size);
// When sending a message, make sure to tack a newline to the end
// of the command. It simulates pressing "ENTER" in the terminal
void send_to_engine(Engine engine, char *command);
void parse_engine_output(char *buf, size_t size, Engine *engine);
