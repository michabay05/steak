#pragma once

#include "board.h"
#include "defs.h"

typedef enum {
    AllMoves,
    CapturesOnly,
} MoveType;

typedef int32_t Move;

int move_encode(Sq source, Sq target, Piece piece, Piece promoted, bool isCapture,
                bool isTwoSquarePush, bool isEnpassant, bool isCastling);
Sq move_get_source(const int move);
Sq move_get_target(const int move);
Piece move_get_piece(const int move);
Piece move_get_promoted(const int move);
bool move_is_capture(const int move);
bool move_is_two_square_push(const int move);
bool move_is_enpassant(const int move);
bool move_is_castling(const int move);

void move_to_str(int move, char *move_str);
int move_parse(char *move_str, Piece piece, bool isCapture, bool isTwoSquarePush, bool isEnpassant,
               bool isCastling);
bool move_make(Board *main, Move move, MoveType move_flag);
