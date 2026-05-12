#ifndef _PGN_H_
#define _PGN_H_

#include "../nob.h"
#include "move.h"

typedef enum {
    PGN_GR_ONGOING,
    PGN_GR_WHITE_WINS,
    PGN_GR_BLACK_WINS,
    PGN_GR_DRAW,
    __count_pgn_gr
} PGN_GameResult;

typedef struct {
    String_View event;
    String_View site;
    String_View date;
    int round;
    String_View white_player;
    String_View black_player;
    PGN_GameResult result;
} PGN;

typedef struct {
    Move *items;
    int count;
    int capacity;
} MoveHistory;

typedef struct {
    f32 time_per_move_s;
    char *start_fen;
    Board board;
    MoveHistory history;
    PGN_GameResult state;
    String_View white_name;
    String_View black_name;
} Game;

void pgn_export(Game *game, String_Builder *sb);

#endif // _PGN_H_
