#include "precalculate.h"
#include "board.h"

// clang-format off
const char *str_coords[66] = {
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
    "??", "none"
};

const char piece_char[14] = {
    'P', 'N', 'B', 'R', 'Q', 'K', 'p', 'n', 'b', 'r', 'q', 'k', '?', '_'
};

const int castling_rights[64] = {
    13, 15, 15, 15, 12, 15, 15, 14,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
     7, 15, 15, 15,  3, 15, 15, 11,
};
// clang-format on

void pos_add_piece(Position *pos, Piece piece, Sq sq) {
    set_bit(pos->piece[piece], sq);
    pos_update_units(pos);
}

void pos_remove_piece(Position *pos, Piece piece, Sq sq) {
    pop_bit(pos->piece[piece], sq);
    pos_update_units(pos);
}

Piece pos_get_piece(const Position pos, Sq sq) {
    for (Piece i = P_LP; i <= P_DK; i++) {
        if (get_bit(pos.piece[i], sq)) return i;
    }
    return P_NONE;
}

void pos_update_units(Position *pos) {
    // Reset all the units bitboard to 0
    pos->units[C_WHITE] = pos->piece[P_LP]
        | pos->piece[P_LN]
        | pos->piece[P_LB]
        | pos->piece[P_LR]
        | pos->piece[P_LQ]
        | pos->piece[P_LK];

    pos->units[C_BLACK]  = pos->piece[P_DP]
        | pos->piece[P_DN]
        | pos->piece[P_DB]
        | pos->piece[P_DR]
        | pos->piece[P_DQ]
        | pos->piece[P_DK];

    pos->all_units = pos->units[C_WHITE] | pos->units[C_BLACK];

    // for (int i = PT_PAWN; i <= PT_KING; i++) {
    //     pos->units[C_WHITE] |= pos->piece[i];
    //     pos->units[C_BLACK] |= pos->piece[6 + i];
    // }
    // pos->all_units = pos->units[C_WHITE] | pos->units[C_BLACK];
}

void state_change_side(State *state) { state->side ^= 1; }

void board_set_from_fen(Board *board, FENInfo fen) {
    *board = (Board){0};
    // Set pieces
    for (int i = 0; i < 64; i++) {
        if (fen.board[i] == P_NONE) continue;
        pos_add_piece(&board->pos, fen.board[i], i);
    }
    board->state.side = fen.side;
    board->state.castling = fen.castling;
    board->state.enpassant = fen.enpassant;
    board->state.half_moves = fen.half_moves;
    board->state.full_moves = fen.full_moves;
}

static void print_castling(u8 castling) {
    if (castling == 0) {
        printf("-\n");
        return;
    }

    if (castling & CR_LK) printf("K");
    if (castling & CR_LQ) printf("Q");
    if (castling & CR_DK) printf("k");
    if (castling & CR_DQ) printf("q");
    printf("\n");
}

void board_print(Board *b) {
    printf("\n    +---+---+---+---+---+---+---+---+\n");
    for (int r = 0; r < 8; r++) {
        printf("  %d |", 8 - r);
        for (int f = 0; f < 8; f++) {
            printf(" %c |", piece_char[pos_get_piece(b->pos, SQ(7 - r, f))]);
        }
        printf("\n    +---+---+---+---+---+---+---+---+\n");
    }
    printf("      a   b   c   d   e   f   g   h\n\n");
    printf("        Side: %s\n", !b->state.side ? "white" : "black");
    printf("   Enpassant: %s\n", str_coords[b->state.enpassant]);
    printf("    Castling: ");
    print_castling(b->state.castling);
    printf("       Moves: %d\n", b->state.full_moves);
}

inline bool board_is_sq_attacked(Board *b, Sq sq, Color side) {
    // Attacked by white pawns
    if ((side == C_WHITE) && (pawn_attacks[C_BLACK][sq] & b->pos.piece[P_LP]))
        return true;
    // Attacked by black pawns
    if ((side == C_BLACK) && (pawn_attacks[C_WHITE][sq] & b->pos.piece[P_DP]))
        return true;
    // Attacked by knights
    if (knight_attacks[sq] & b->pos.piece[side == C_WHITE ? P_LN : P_DN])
        return true;
    // Attacked by bishops
    if (get_bishop_attack(sq, b->pos.all_units) & b->pos.piece[side == C_WHITE ? P_LB : P_DB])
        return true;
    // Attacked by rooks
    if (get_rook_attack(sq, b->pos.all_units) & b->pos.piece[side == C_WHITE ? P_LR : P_DR])
        return true;
    // Attacked by queens
    if (get_queen_attack(sq, b->pos.all_units) & b->pos.piece[side == C_WHITE ? P_LQ : P_DQ])
        return true;
    // Attacked by kings
    if (king_attacks[sq] & b->pos.piece[side == C_WHITE ? P_LK : P_DK])
        return true;

    // If all of the above cases fail, return false
    return false;
}

inline bool board_is_in_check(Board *b) {
    return board_is_sq_attacked(
        b,
        bb_lsb_index(b->pos.piece[b->state.side == C_WHITE ? P_DK : P_LK]),
        b->state.side
    );
}
