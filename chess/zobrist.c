#include "zobrist.h"
#include "bitboard.h"
#include "board.h"
#include "defs.h"

static ZB_Key PIECE_KEYS[2][6][64] = {0};
static ZB_Key ENPASSANT_KEYS[64] = {0};
static ZB_Key CASTLING_KEYS[16] = {0};
static ZB_Key SIDE_KEY = 0ULL;

#include "prand.c"

void zobrist_init(void) {
    for (Color c = C_WHITE; c <= C_BLACK; c++) {
        for (PieceType pt = PT_PAWN; pt <= PT_KING; pt++) {
            for (Sq sq = SQ_A1; sq <= SQ_H8; sq++)
                PIECE_KEYS[c][pt][sq] = random_u64();
        }
    }

    for (Sq sq = SQ_A1; sq <= SQ_H8; sq++) ENPASSANT_KEYS[sq] = random_u64();
    for (CastlingRight cr = 0; cr < 16; cr++) CASTLING_KEYS[cr] = random_u64();

    SIDE_KEY = random_u64();
}

ZB_Key zobrist_gen_key(Board *board) {
    ZB_Key key = 0ULL;

    Bitboard bb;
    Sq sq;
    for (Color c = C_WHITE; c <= C_BLACK; c++) {
        for (PieceType pt = PT_PAWN; pt <= PT_KING; pt++) {
            bb = board->piece[c][pt];
            while (bb) {
                sq = bb_lsb_index(bb);
                key ^= PIECE_KEYS[c][pt][sq];
                pop_bit(bb, sq);
            }
        }
    }

    if (board->enpassant != SQ_NONE) key ^= ENPASSANT_KEYS[board->enpassant];
    key ^= CASTLING_KEYS[board->castling];
    if (board->side == C_BLACK) board->side ^= SIDE_KEY;

    return key;
}

void zobrist_toggle_piece(Board *board, Piece piece, Sq sq) {
    board->key ^= PIECE_KEYS[piece.color][piece.type][sq];
}

void zobrist_update_enpassant(Board *board, Sq enpass_sq) {
    board->key ^= ENPASSANT_KEYS[enpass_sq];
}

void zobrist_update_castling(Board *board, CastlingRight cright) {
    board->key ^= CASTLING_KEYS[cright];
}

void zobrist_toggle_side(Board *board) {
    board->key ^= SIDE_KEY;
}

