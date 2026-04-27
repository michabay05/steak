#include "move.h"

Move move_encode(Sq source, Sq target, Piece piece, Piece promoted,
    MoveFlags flag) {
    return (Move) {
        .source = source,
        .target = target,
        .piece = piece,
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
    if (move.promoted != P_NONE) {
        move_str[4] = piece_char[move.promoted];
        move_str[5] = 0;
    } else {
        move_str[4] = 0;
    }
}

Move move_parse(char *move_str, Piece piece, MoveFlags flag) {
    int source = SQ(move_str[1] - '0', move_str[0] - 'a');
    int target = SQ(move_str[3] - '0', move_str[2] - 'a');
    Piece promoted = P_NONE;
    if (move_str && (move_str[4] >= 'a' && move_str[4] <= 'z')) {
        switch (move_str[4]) {
        case 'Q':
            promoted = P_LQ;
            break;
        case 'R':
            promoted = P_LR;
            break;
        case 'B':
            promoted = P_LB;
            break;
        case 'N':
            promoted = P_LN;
            break;
        case 'q':
            promoted = P_DQ;
            break;
        case 'r':
            promoted = P_DR;
            break;
        case 'b':
            promoted = P_DB;
            break;
        case 'n':
            promoted = P_DN;
            break;
        }
    }
    Move ouptut = move_encode(source, target, piece, promoted, flag);

    char move_temp[5];
    move_to_str(ouptut, move_temp);

    return ouptut;
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

    // Decode all of the information from the move
    // Sq source = move_get_source(move);
    // Sq target = move_get_target(move);
    // Piece piece = move_get_piece(move);
    // bool is_capture = move_is_capture(move);
    // bool is_two_square_push = move_is_two_square_push(move);
    // bool is_enpassant = move_is_enpassant(move);
    // bool is_castling = move_is_castling(move);
    // Move piece from source sq to target sq
    pop_bit(main->pos.piece[move.piece], move.source);
    set_bit(main->pos.piece[move.piece], move.target);

    // If move is capture, remove the piece from the opponent's bitboard
    if (move.flag == MVF_Capture) {
        for (Piece p = (main->state.side == C_WHITE ? P_DP : P_LP);
                p <= (main->state.side == C_WHITE ? P_DK : P_LK); p++) {
            if (get_bit(main->pos.piece[p], move.target)) {
                pop_bit(main->pos.piece[p], move.target);
                // There's no need to keep looking for another piece because
                // only one piece can be captured
                break;
            }
        }
    }

    // If move is promotion, change the pawn to the desired piece
    if (move.promoted != P_NONE) {
        pop_bit(main->pos.piece[move.piece], move.target);
        set_bit(main->pos.piece[move.promoted], move.target);
    }

    // Unlike other captures, make sure to remove the "enpassant'd" pawn from the enemy bitboard
    if (move.flag == MVF_Enpassant) {
        Piece pawn = P_DP;
        Direction dir = DIR_SOUTH;
        if (main->state.side == C_BLACK) {
            pawn = P_LP;
            dir = DIR_NORTH;
        }
        pop_bit(main->pos.piece[pawn], move.target + dir);
    }
    // Reset enpassant square, even if the current move was enpassant or not
    // because enpassant can only be played on the move after the two square pawn push
    main->state.enpassant = SQ_NONE;
    if (move.flag == MVF_TwoSquarePush) {
        main->state.enpassant = move.target
            + ((main->state.side == C_WHITE) ? DIR_SOUTH : DIR_NORTH);
    }

    // If move is castling, place the rook on the correct square
    // FYI, the king is already on the correct square (as specified in the move generation)
    if (move.flag == MVF_Castling) {
        // Target = king's target square
        switch (move.target) {
        case SQ_G1:
            pop_bit(main->pos.piece[P_LR], SQ_H1);
            set_bit(main->pos.piece[P_LR], SQ_F1);
            break;
        case SQ_C1:
            pop_bit(main->pos.piece[P_LR], SQ_A1);
            set_bit(main->pos.piece[P_LR], SQ_D1);
            break;
        case SQ_G8:
            pop_bit(main->pos.piece[P_DR], SQ_H8);
            set_bit(main->pos.piece[P_DR], SQ_F8);
            break;
        case SQ_C8:
            pop_bit(main->pos.piece[P_DR], SQ_A8);
            set_bit(main->pos.piece[P_DR], SQ_D8);
            break;
        default:
            break;
        }
    }
    main->state.castling &= castling_rights[move.source];
    main->state.castling &= castling_rights[move.target];
    // Manually update the units bitboard because of the manual
    // manipulations of the piece bitboards
    pos_update_units(&main->pos);

    state_change_side(&main->state);

    // After the move is made, if the current move reveals on the check on the king
    // unmake the move by restoring the current board to the earlier clone
    if (board_is_in_check(main)) {
        *main = copy;
        return false;
    } else {
        if (!main->state.side)
            main->state.full_moves++;
        return true;
    }
}
