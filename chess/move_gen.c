#include "../nob.h"

#include "move_gen.h"
#include "bitboard.h"
#include "board.h"
#include "defs.h"
#include "move.h"
#include "precalculate.h"

void movelist_add(MoveList *ml, Move move) {
    assert(ml->count < MOVE_GEN_MAX);
    ml->list[ml->count++] = move;
}

Move movelist_search(MoveList ml, Sq source, Sq target, Piece promoted) {
    for (int i = 0; i < ml.count; i++) {
        // Parse move info
        Sq listMoveSource = ml.list[i].source;
        Sq listMoveTarget = ml.list[i].target;
        Piece listMovePromoted = move_get_promoted(ml.list[i]);
        // Check if source and target match
        if (listMoveSource == source && listMoveTarget == target && listMovePromoted == promoted)
            // Return index of move from movelist, if true
            return ml.list[i];
    }
    return (Move){0};
}

void movelist_print_list(MoveList ml) {
    printf("    Source   |   Target  |  Piece  |  Promoted  |  Capture  |  Two "
           "Square Push  |  Enpassant  |  Castling\n");
    printf("  "
           "---------------------------------------------------------------------"
           "--------------------------------------\n");
    for (int i = 0; i < ml.count; i++) {
        printf("       %s    |    %s     |    %c    |     %c      |     %d     |   "
               "      %d         |      %d      |     %d\n",
               str_coords[ml.list[i].source], str_coords[ml.list[i].target],
               piece_char[ml.list[i].piece], piece_char[ml.list[i].promoted],
               ml.list[i].flag == MVF_Capture,
               ml.list[i].flag == MVF_TwoSquarePush,
               ml.list[i].flag == MVF_Enpassant,
               ml.list[i].flag == MVF_Castling);
    }
    printf("\n    Total number of moves: %d\n", ml.count);
}

static void movelist_gen_pawn(MoveList *ml, Board *b) {
    #define in_enemy_back_rank(p, sq) (p == P_LP ? (SQ_A8 <= sq && sq <= SQ_H8) : (SQ_A1 <= sq && sq <= SQ_H1))

    #define in_starting_rank(p, sq) (p == P_LP ? (SQ_A2 <= sq && sq <= SQ_H2) : (SQ_A7 <= sq && sq <= SQ_H7))

    #define in_promotion_rank(p, sq) (p == P_LP ? (SQ_A7 <= sq && sq <= SQ_H7) : (SQ_A2 <= sq && sq <= SQ_H2))


    uint64_t bitboard_copy, attack_copy;
    Piece pawn;
    Direction direction;
    Sq source, target;
    if (b->state.side == C_WHITE) {
        pawn = P_LP;
        direction = DIR_NORTH;
    }
    else {
        pawn = P_DP;
        direction = DIR_SOUTH;
    }

    bitboard_copy = b->pos.piece[pawn];
    while (bitboard_copy) {
        source = bb_lsb_index(bitboard_copy);
        target = source + direction;
        if (!get_bit(b->pos.all_units, target)) {
            if (in_promotion_rank(pawn, source)
                && in_enemy_back_rank(pawn, target)) {
                // Quiet Promotion moves
                movelist_add(ml, move_encode(source, target, pawn,
                    (b->state.side == C_WHITE ? P_LQ : P_DQ), MVF_Quiet));
                movelist_add(ml, move_encode(source, target, pawn,
                    (b->state.side == C_WHITE ? P_LR : P_DR), MVF_Quiet));
                movelist_add(ml, move_encode(source, target, pawn,
                    (b->state.side == C_WHITE ? P_LB : P_DB), MVF_Quiet));
                movelist_add(ml, move_encode(source, target, pawn,
                    (b->state.side == C_WHITE ? P_LN : P_DN), MVF_Quiet));
            } else {
                // Quiet moves
                movelist_add(ml, move_encode(
                    source, target, pawn, P_NONE, MVF_Quiet));

                if (in_starting_rank(pawn, source)
                    && !get_bit(b->pos.all_units, target + direction))
                    movelist_add(ml, move_encode(source, target + direction,
                        pawn, P_NONE, MVF_TwoSquarePush));
            }
        }

        // Capture moves
        attack_copy = pawn_attacks[b->state.side][source] & b->pos.units[b->state.side ^ 1];
        while (attack_copy) {
            Sq attack_target = bb_lsb_index(attack_copy);
            // Capture move
            if (in_promotion_rank(pawn, source)
                && in_enemy_back_rank(pawn, attack_target)) {
                movelist_add(ml, move_encode(source, attack_target, pawn,
                    (b->state.side == C_WHITE ? P_LQ : P_DQ), MVF_Capture));
                movelist_add(ml, move_encode(source, attack_target, pawn,
                    (b->state.side == C_WHITE ? P_LR : P_DR), MVF_Capture));
                movelist_add(ml, move_encode(source, attack_target, pawn,
                    (b->state.side == C_WHITE ? P_LB : P_DB), MVF_Capture));
                movelist_add(ml, move_encode(source, attack_target, pawn,
                    (b->state.side == C_WHITE ? P_LN : P_DN), MVF_Capture));
            } else
                movelist_add(ml, move_encode(source, attack_target,
                    pawn, P_NONE, MVF_Capture));
            // Remove 'source' bit
            pop_bit(attack_copy, attack_target);
        }
        // Generate enpassant capture
        if (b->state.enpassant != SQ_NONE) {
            uint64_t enpassCapture =
                pawn_attacks[b->state.side][source] & (1ULL << b->state.enpassant);
            if (enpassCapture) {
                int enpassTarget = bb_lsb_index(enpassCapture);
                movelist_add(ml, move_encode(
                    source, enpassTarget, pawn, P_NONE, MVF_Enpassant));
            }
        }
        // Remove bits
        pop_bit(bitboard_copy, source);
    }
}

static void movelist_gen_qrnbk(MoveList *ml, Board *b, PieceType pt) {
    Piece piece;
    switch (pt) {
        case PT_KNIGHT:
            piece = b->state.side == C_WHITE ? P_LN : P_DN;
            break;
        case PT_BISHOP:
            piece = b->state.side == C_WHITE ? P_LB : P_DB;
            break;
        case PT_ROOK:
            piece = b->state.side == C_WHITE ? P_LR : P_DR;
            break;
        case PT_QUEEN:
            piece = b->state.side == C_WHITE ? P_LQ : P_DQ;
            break;
        case PT_KING:
            piece = b->state.side == C_WHITE ? P_LK : P_DK;
            break;

        case PT_PAWN:
        default: UNREACHABLE("Pawn or unknown piece type");
    }

    Sq source, target;
    uint64_t bitboard_copy = b->pos.piece[piece], attack_copy;
    while (bitboard_copy) {
        source = bb_lsb_index(bitboard_copy);

        switch (pt) {
            case PT_KNIGHT:
                attack_copy = knight_attacks[source];
                break;
            case PT_BISHOP:
                attack_copy = get_bishop_attack(source, b->pos.all_units);
                break;
            case PT_ROOK:
                attack_copy = get_rook_attack(source, b->pos.all_units);
                break;
            case PT_QUEEN:
                attack_copy = get_queen_attack(source, b->pos.all_units);
                break;
            case PT_KING:
                attack_copy = king_attacks[source];
                break;

            case PT_PAWN:
            default: UNREACHABLE("Pawn or unknown piece type");
        }

        attack_copy &= (b->state.side == C_WHITE ? ~b->pos.units[C_WHITE] : ~b->pos.units[C_BLACK]);
        while (attack_copy) {
            target = bb_lsb_index(attack_copy);
            if (get_bit(b->pos.units[b->state.side == C_WHITE ? C_BLACK : C_WHITE], target))
                movelist_add(ml, move_encode(source, target, piece, P_NONE, MVF_Capture));
            else
                movelist_add(ml, move_encode(source, target, piece, P_NONE, MVF_Quiet));
            pop_bit(attack_copy, target);
        }
        pop_bit(bitboard_copy, source);
    }
}

static void movelist_gen_white_castling(MoveList *ml, Board *b) {
    // Kingside castling
    if (get_bit(b->state.castling, CR_LK)) {
        // Check if path is obstructed
        if (!get_bit(b->pos.all_units, SQ_F1) && !get_bit(b->pos.all_units, SQ_G1)) {
            // Is e1 or f1 attacked by a black piece?
            if (!board_is_sq_attacked(b, SQ_E1, C_BLACK) && !board_is_sq_attacked(b, SQ_F1, C_BLACK))
                movelist_add(ml, move_encode(SQ_E1, SQ_G1, P_LK, P_NONE, MVF_Castling));
        }
    }
    // Queenside castling
    if (get_bit(b->state.castling, CR_LQ)) {
        // Check if path is obstructed
        if (!get_bit(b->pos.all_units, SQ_B1) && !get_bit(b->pos.all_units, SQ_C1) &&
            !get_bit(b->pos.all_units, SQ_D1)) {
            // Is d1 or e1 attacked by a black piece?
            if (!board_is_sq_attacked(b, SQ_D1, C_BLACK) && !board_is_sq_attacked(b, SQ_E1, C_BLACK))
                movelist_add(ml, move_encode(SQ_E1, SQ_C1, P_LK, P_NONE, MVF_Castling));
        }
    }
}

static void movelist_gen_black_castling(MoveList *ml, Board *b) {
    // Kingside castling
    if (get_bit(b->state.castling, CR_DK)) {
        // Check if path is obstructed
        if (!get_bit(b->pos.all_units, SQ_F8) && !get_bit(b->pos.all_units, SQ_G8)) {
            // Is e8 or f8 attacked by a white piece?
            if (!board_is_sq_attacked(b, SQ_E8, C_WHITE) && !board_is_sq_attacked(b, SQ_F8, C_WHITE))
                movelist_add(ml, move_encode(SQ_E8, SQ_G8, P_DK, P_NONE, MVF_Castling));
        }
    }
    // Queenside castling
    if (get_bit(b->state.castling, CR_DQ)) {
        // Check if path is obstructed
        if (!get_bit(b->pos.all_units, SQ_B8) && !get_bit(b->pos.all_units, SQ_C8) &&
            !get_bit(b->pos.all_units, SQ_D8)) {
            // Is d8 or e8 attacked by a white piece?
            if (!board_is_sq_attacked(b, SQ_D8, C_WHITE) && !board_is_sq_attacked(b, SQ_E8, C_WHITE))
                movelist_add(ml, move_encode(SQ_E8, SQ_C8, P_DK, P_NONE, MVF_Castling));
        }
    }
}

static void movelist_gen_king(MoveList *ml, Board *b) {
    // NOTE: Checks aren't handled by the move generator, it's handled by the make move function.

    movelist_gen_qrnbk(ml, b, PT_KING);
    // Generate castling moves
    if (b->state.side == C_WHITE) movelist_gen_white_castling(ml, b);
    else movelist_gen_black_castling(ml, b);
}

void movelist_generate(MoveList *ml, Board *b, Piece p) {
    switch (COLORLESS(p)) {
        case PT_PAWN: movelist_gen_pawn(ml, b); break;
        case PT_KNIGHT: movelist_gen_qrnbk(ml, b, PT_KNIGHT); break;
        case PT_BISHOP: movelist_gen_qrnbk(ml, b, PT_BISHOP); break;
        case PT_ROOK  : movelist_gen_qrnbk(ml, b, PT_ROOK); break;
        case PT_QUEEN: movelist_gen_qrnbk(ml, b, PT_QUEEN); break;
        case PT_KING: movelist_gen_king(ml, b); break;
    }
}

void movelist_generate_all(MoveList *ml, Board *b) {
    movelist_gen_pawn(ml, b);
    movelist_gen_qrnbk(ml, b, PT_KNIGHT);
    movelist_gen_qrnbk(ml, b, PT_BISHOP);
    movelist_gen_qrnbk(ml, b, PT_ROOK);
    movelist_gen_qrnbk(ml, b, PT_QUEEN);
    movelist_gen_king(ml, b);
}

void movelist_legal(MoveList *ml, Board *b) {
    MoveList temp = {0};
    movelist_generate_all(&temp, b);

    *ml = (MoveList){0};
    Board copy = *b;
    for (int i = 0; i < temp.count; i++) {
        if (move_make(b, temp.list[i], AllMoves)) {
            ml->list[ml->count] = temp.list[i];
            ml->count++;
        }
        *b = copy;
    }
}
