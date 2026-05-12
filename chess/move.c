#include "move.h"
#include "board.h"
#include "defs.h"

Move move_encode(Sq source, Sq target, PieceType promoted,
    MoveFlags flag) {
    return (Move) {
        .source = source,
        .target = target,
        .promoted = promoted,
        .flag = flag
    };
}

inline bool move_eq(Move a, Move b) {
    return a.source == b.source && a.target == b.target
        && a.promoted == b.promoted;
}

// TODO: add checks to see if move_str has enough space (move_str.length >= 6)
void move_to_str(Move move, char *move_str) {
    // Source square
    move_str[0] = str_coords[move.source][0];
    move_str[1] = str_coords[move.source][1];
    // Target square
    move_str[2] = str_coords[move.target][0];
    move_str[3] = str_coords[move.target][1];
    // Promotion piece
    if (move.promoted != PT_NONE) {
        move_str[4] = piece_char[C_BLACK][move.promoted];
        move_str[5] = 0;
    } else {
        move_str[4] = 0;
    }
}

Move move_parse_cstr(char *move_str) {
    return move_parse_sv(sv_from_cstr(move_str));
}

// NOTE: Since a move flag can not be associated with a move upon parsing, a default move flag of
// QUIET is assigned to the returned move.
Move move_parse_sv(String_View msv) {
    if (!(msv.count == 4 || msv.count == 5)) {
        fprintf(stderr, "Illegal move string: '"SV_Fmt"'\n", SV_Arg(msv));
        UNREACHABLE("illegal move string during parsing");
    }

    int source = SQ(msv.data[1] - '1', msv.data[0] - 'a');
    int target = SQ(msv.data[3] - '1', msv.data[2] - 'a');
    PieceType promoted = PT_NONE;
    if (msv.count == 5 && (msv.data[4] >= 'a' && msv.data[4] <= 'z')) {
        // TODO: this is not correct. Promoted should keep track of the piece type not an actual
        // piece. Fix this.
        switch (msv.data[4]) {
            case 'Q':
            case 'q': promoted = PT_QUEEN; break;
            case 'R':
            case 'r': promoted = PT_ROOK; break;
            case 'B':
            case 'b': promoted = PT_BISHOP; break;
            case 'N':
            case 'n': promoted = PT_KNIGHT; break;
        }
    }

    return move_encode(source, target, promoted, MVF_Quiet);
}

bool move_make(Board *main, Move move, MoveType move_flag) {
    if (move_flag == CapturesOnly) {
        // Before recusively calling this method ensure that this move is a capture
        if (move.flag == MVF_Capture) {
            return move_make(main, move, AllMoves);
        } else {
            // If it's not don't make it
            return false;
        }
    }

    // Clone board and make current move on main board, if current
    // move is illegal restore the board to this clone
    Board copy = *main;
    Piece piece = board_get_piece(main, move.source);

    pop_bit(main->piece[piece.color][piece.type], move.source);
    set_bit(main->piece[piece.color][piece.type], move.target);

    // If move is capture, remove the piece from the opponent's bitboard
    if (move.flag == MVF_Capture) {
        for (PieceType pt = PT_PAWN; pt < PT_KING; pt++) {
            if (get_bit(main->piece[piece.color ^ 1][pt], move.target)) {
                pop_bit(main->piece[piece.color ^ 1][pt], move.target);
                // There's no need to keep looking for another piece because
                // only one piece can be captured
                break;
            }
        }
    }

    // If move is promotion, change the pawn to the desired piece
    if (move.promoted != PT_NONE) {
        Piece prom_piece = {
            .color = TO_BOOL(move.target & RANK_MASK[RANK_1]),
            .type = move.promoted
        };

        pop_bit(main->piece[piece.color][piece.type], move.target);
        set_bit(main->piece[prom_piece.color][prom_piece.type], move.target);
    }

    // Unlike other captures, make sure to remove the "enpassant'd" pawn from the enemy bitboard
    if (move.flag == MVF_Enpassant) {
        Piece pawn = P_DP;
        Direction dir = DIR_SOUTH;
        if (main->side == C_BLACK) {
            pawn = P_LP;
            dir = DIR_NORTH;
        }
        pop_bit(main->piece[pawn.color][pawn.type], move.target + dir);
    }
    // Reset enpassant square, even if the current move was enpassant or not
    // because enpassant can only be played on the move after the two square pawn push
    main->enpassant = SQ_NONE;
    if (move.flag == MVF_TwoSquarePush) {
        main->enpassant = move.target
            + ((main->side == C_WHITE) ? DIR_SOUTH : DIR_NORTH);
    }

    // If move is castling, place the rook on the correct square
    // FYI, the king is already on the correct square (as specified in the move generation)
    if (move.flag == MVF_Castling) {
        // Target = king's target square
        switch (move.target) {
            case SQ_G1:
                pop_bit(main->piece[C_WHITE][PT_ROOK], SQ_H1);
                set_bit(main->piece[C_WHITE][PT_ROOK], SQ_F1);
                break;
            case SQ_C1:
                pop_bit(main->piece[C_WHITE][PT_ROOK], SQ_A1);
                set_bit(main->piece[C_WHITE][PT_ROOK], SQ_D1);
                break;
            case SQ_G8:
                pop_bit(main->piece[C_BLACK][PT_ROOK], SQ_H8);
                set_bit(main->piece[C_BLACK][PT_ROOK], SQ_F8);
                break;
            case SQ_C8:
                pop_bit(main->piece[C_BLACK][PT_ROOK], SQ_A8);
                set_bit(main->piece[C_BLACK][PT_ROOK], SQ_D8);
                break;
            default:
                break;
        }
    }
    main->castling &= castling_rights[move.source];
    main->castling &= castling_rights[move.target];
    // Manually update the units bitboard because of the manual
    // manipulations of the piece bitboards
    board_update_units(main);
    board_change_side(main);

    // After the move is made, if the current move reveals on the check on the king
    // unmake the move by restoring the current board to the earlier clone
    if (board_is_in_check(main)) {
        *main = copy;
        return false;
    } else {
        if (!main->side)
            main->full_moves++;
        return true;
    }
}
