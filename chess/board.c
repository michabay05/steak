#include "defs.h"
#include "precalculate.h"
#include <stdio.h>
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

const char *piece_char[2] = { "PNBRQK", "pnbrqk? " };

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

Piece board_get_piece(Board *board, Sq sq) {
    for (Color c = C_WHITE; c <= C_BLACK; c++) {
        for (PieceType pt = PT_PAWN; pt <= PT_KING; pt++) {
            if (get_bit(board->piece[c][pt], sq))
                return (Piece){.color = c, .type = pt};
        }
    }
    return P_NONE;
}

void board_update_units(Board *board) {
    // Reset all the units bitboard to 0
    board->units[C_WHITE] = board->piece[C_WHITE][PT_PAWN]
        | board->piece[C_WHITE][PT_KNIGHT]
        | board->piece[C_WHITE][PT_BISHOP]
        | board->piece[C_WHITE][PT_ROOK]
        | board->piece[C_WHITE][PT_QUEEN]
        | board->piece[C_WHITE][PT_KING];

    board->units[C_BLACK] = board->piece[C_BLACK][PT_PAWN]
        | board->piece[C_BLACK][PT_KNIGHT]
        | board->piece[C_BLACK][PT_BISHOP]
        | board->piece[C_BLACK][PT_ROOK]
        | board->piece[C_BLACK][PT_QUEEN]
        | board->piece[C_BLACK][PT_KING];

    board->all_units = board->units[C_WHITE] | board->units[C_BLACK];
}

void board_change_side(Board *board) { board->side ^= 1; }

// TODO: ensure that the buffer's size is at least 5.
static void castling_to_str(u8 castling, char *buf) {
    int i = 0;
    if (castling == 0) {
        sprintf(buf, "-");
        return;
    }

    if (castling & (1 << CR_LK)) buf[i++] = 'K';
    if (castling & (1 << CR_LQ)) buf[i++] = 'Q';
    if (castling & (1 << CR_DK)) buf[i++] = 'k';
    if (castling & (1 << CR_DQ)) buf[i++] = 'q';

    buf[i++] = 0;
}

void board_print(Board *b) {
    printf("\n    +---+---+---+---+---+---+---+---+\n");
    for (int r = 0; r < 8; r++) {
        printf("  %d |", 8 - r);
        for (int f = 0; f < 8; f++) {
            Piece p = board_get_piece(b, SQ(7 - r, f));
            printf(" %c |", piece_char[p.color][p.type]);
        }
        printf("\n    +---+---+---+---+---+---+---+---+\n");
    }

    char castling_buf[5] = {0};
    castling_to_str(b->castling, castling_buf);

    printf("      a   b   c   d   e   f   g   h\n\n");
    printf("        Side: %s\n", !b->side ? "white" : "black");
    printf("   Enpassant: %s\n", str_coords[b->enpassant]);
    printf("    Castling: %s\n", castling_buf);
    printf("       Moves: %d\n", b->full_moves);
}

inline bool board_is_sq_attacked(Board *b, Sq sq, Color side) {
    // Attacked by white pawns
    if ((side == C_WHITE) && (pawn_attacks[C_BLACK][sq] & b->piece[C_WHITE][PT_PAWN]))
        return true;
    // Attacked by black pawns
    if ((side == C_BLACK) && (pawn_attacks[C_WHITE][sq] & b->piece[C_BLACK][PT_PAWN]))
        return true;
    // Attacked by knights
    if (knight_attacks[sq] & b->piece[side][PT_KNIGHT])
        return true;
    // Attacked by bishops
    if (get_bishop_attack(sq, b->all_units) & b->piece[side][PT_BISHOP])
        return true;
    // Attacked by rooks
    if (get_rook_attack(sq, b->all_units) & b->piece[side][PT_ROOK])
        return true;
    // Attacked by queens
    if (get_queen_attack(sq, b->all_units) & b->piece[side][PT_QUEEN])
        return true;
    // Attacked by kings
    if (king_attacks[sq] & b->piece[side][PT_KING])
        return true;

    // If all of the above cases fail, return false
    return false;
}

inline bool board_is_in_check(Board *b) {
    return board_is_sq_attacked(
        b,
        bb_lsb_index(b->piece[b->side ^ 1][PT_KING]),
        b->side
    );
}

inline void board_parse_fen_cstr(Board *board, const char *fen) {
    board_parse_fen_sv(board, sv_from_cstr(fen));
}

void board_parse_fen_sv(Board *board, String_View fen) {
    // Piece placements
    u8 rank = 7, file = 0;

    // Set every square to be empty before setting values
    Board temp = {0};
    String_View pieces = sv_chop_by_delim(&fen, ' ');
    while (pieces.count > 0) {
        char c = sv_chop_left(&pieces, 1).data[0];
        if (c == '/') {
            rank--;
            file = 0;
            continue;
        } else if (c >= '0' && c <= '9') {
            file += c - '0';
            continue;
        }

        Sq sq = SQ(rank, file);
        switch (c) {
            case 'K': set_bit(temp.piece[C_WHITE][PT_KING]  , sq); break;
            case 'Q': set_bit(temp.piece[C_WHITE][PT_QUEEN] , sq); break;
            case 'R': set_bit(temp.piece[C_WHITE][PT_ROOK]  , sq); break;
            case 'B': set_bit(temp.piece[C_WHITE][PT_BISHOP], sq); break;
            case 'N': set_bit(temp.piece[C_WHITE][PT_KNIGHT], sq); break;
            case 'P': set_bit(temp.piece[C_WHITE][PT_PAWN]  , sq); break;
            case 'k': set_bit(temp.piece[C_BLACK][PT_KING]  , sq); break;
            case 'q': set_bit(temp.piece[C_BLACK][PT_QUEEN] , sq); break;
            case 'r': set_bit(temp.piece[C_BLACK][PT_ROOK]  , sq); break;
            case 'b': set_bit(temp.piece[C_BLACK][PT_BISHOP], sq); break;
            case 'n': set_bit(temp.piece[C_BLACK][PT_KNIGHT], sq); break;
            case 'p': set_bit(temp.piece[C_BLACK][PT_PAWN]  , sq); break;
        }
        file++;
    }
    board_update_units(&temp);

    // Push pointer one more to account for space
    String_View side_to_move = sv_chop_by_delim(&fen, ' ');
    temp.side = sv_eq(side_to_move, sv_from_cstr("w")) ? C_WHITE : C_BLACK;

    String_View castling = sv_chop_by_delim(&fen, ' ');
    while (castling.count > 0) {
        char c = sv_chop_left(&castling, 1).data[0];
        switch (c) {
            case 'K': set_bit(temp.castling, CR_LK); break;
            case 'Q': set_bit(temp.castling, CR_LQ); break;
            case 'k': set_bit(temp.castling, CR_DK); break;
            case 'q': set_bit(temp.castling, CR_DQ); break;
        }

    }

    temp.enpassant = SQ_NONE;
    String_View enpassant = sv_chop_by_delim(&fen, ' ');
    if (!sv_eq(enpassant, sv_from_cstr("-")) && enpassant.count == 2) {
        int file = enpassant.data[0] - 'a';

        int rank = 8 - (enpassant.data[1] - '0');
        temp.enpassant = SQ(rank, file);
    }


    String_View half_moves = sv_chop_by_delim(&fen, ' ');
    temp.half_moves = atoi(half_moves.data);

    String_View full_moves = sv_chop_by_delim(&fen, ' ');
    temp.full_moves = atoi(full_moves.data);

    *board = temp;
}

void board_fen_generate(Board *board, String_Builder *sb) {
    // Piece arrangement
    for (Rank r = RANK_1; r <= RANK_8; r++) {
        int empty = 0;
        for (File f = FILE_A; f <= FILE_H; f++) {
            Piece p = board_get_piece(board, SQ(7 - r, f));
            if (p.type == PT_NONE) empty++;
            else {
                if (empty > 0) sb_appendf(sb, "%d", empty);
                sb_append(sb, piece_char[p.color][p.type]);
                empty = 0;
            }
        }

        if (empty != 0) sb_appendf(sb, "%d", empty);
        if (r < RANK_8) sb_append(sb, '/');
    }
    sb_append(sb, ' ');

    // Side to move
    sb_append(sb, board->side == C_WHITE ? 'w' : 'b');
    sb_append(sb, ' ');

    // Castling right
    char castling_buf[5] = {0};
    castling_to_str(board->castling, castling_buf);
    sb_append_buf(sb, castling_buf, strlen(castling_buf));
    sb_append(sb, ' ');

    // Enpassant square
    if (board->enpassant == SQ_NONE) sb_append(sb, '-');
    else sb_append_cstr(sb, str_coords[board->enpassant]);
    sb_append(sb, ' ');

    // Half move
    sb_appendf(sb, "%d ", board->half_moves);
    // Full move
    sb_appendf(sb, "%d", board->full_moves);
}
