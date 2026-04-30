#include "move_gen.h"
#include "bitboard.h"
#include "defs.h"
#include "precalculate.h"


void movelist_add(MoveList *ml, Move move) {
    assert(ml->count < MOVE_GEN_MAX && "Surpassed max move count");
    ml->list[ml->count++] = move;
}

int movelist_search(MoveList ml, Sq source, Sq target, PieceType promoted) {
    // The minimum amount of information required to determine if two moves are the same are the
    // following: source, target, and promoted. The rest can be anything. The three can uniquely
    // describe a move.
    Move mv_target = move_encode(source, target, promoted, MVF_Quiet);
    for (int i = 0; i < ml.count; i++)
        if (move_eq(mv_target, ml.list[i])) return i;

    return -1;
}

#if 1
void movelist_print_list(MoveList ml) {
    printf("    Source   |   Target  |  Promoted  |  Capture  |  Two "
           "Square Push  |  Enpassant  |  Castling\n");
    printf("  "
           "---------------------------------------------------------------------"
           "--------------------------\n");
    for (int i = 0; i < ml.count; i++) {
        Move mv = ml.list[i];
        printf("       %s    |    %s     |     %c      |     %d     |   "
               "      %d         |      %d      |     %d\n",
            str_coords[mv.source], str_coords[mv.target],
            piece_char[TO_PIECE(C_BLACK, mv.promoted)],
            mv.flag == MVF_Capture,
            mv.flag == MVF_TwoSquarePush,
            mv.flag == MVF_Enpassant,
            mv.flag == MVF_Castling);
    }
    printf("\n    Total number of moves: %d\n", ml.count);
}
#endif

static void movelist_gen_pawn(MoveList *ml, Board *b) {
    #define in_enemy_back_rank(p, sq) (p == P_LP ? (SQ_A8 <= sq && sq <= SQ_H8) : (SQ_A1 <= sq && sq <= SQ_H1))

    #define in_starting_rank(p, sq) (p == P_LP ? (SQ_A2 <= sq && sq <= SQ_H2) : (SQ_A7 <= sq && sq <= SQ_H7))

    #define in_promotion_rank(p, sq) (p == P_LP ? (SQ_A7 <= sq && sq <= SQ_H7) : (SQ_A2 <= sq && sq <= SQ_H2))

    Bitboard bb_copy, attack_copy;
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

    bb_copy = b->pos.piece[pawn];
    while (bb_copy) {
        source = bb_lsb_index(bb_copy);
        target = source + direction;
        if (!get_bit(b->pos.all_units, target)) {
            if (in_promotion_rank(pawn, source)
                && in_enemy_back_rank(pawn, target)) {
                // Quiet Promotion moves
                movelist_add(ml, move_encode(source, target,
                    PT_QUEEN, MVF_Quiet));
                movelist_add(ml, move_encode(source, target,
                    PT_ROOK, MVF_Quiet));
                movelist_add(ml, move_encode(source, target,
                    PT_BISHOP, MVF_Quiet));
                movelist_add(ml, move_encode(source, target,
                    PT_KNIGHT, MVF_Quiet));
            } else {
                // Quiet moves
                movelist_add(ml, move_encode(
                    source, target, PT_NONE, MVF_Quiet));

                if (in_starting_rank(pawn, source)
                    && !get_bit(b->pos.all_units, target + direction))
                    movelist_add(ml, move_encode(source, target + direction,
                        PT_NONE, MVF_TwoSquarePush));
            }
        }

        // Capture moves
        attack_copy = pawn_attacks[b->state.side][source] & b->pos.units[b->state.side ^ 1];
        while (attack_copy) {
            Sq attack_target = bb_lsb_index(attack_copy);
            // Capture move
            if (in_promotion_rank(pawn, source)
                && in_enemy_back_rank(pawn, attack_target)) {
                movelist_add(ml, move_encode(source, attack_target,
                    PT_QUEEN, MVF_Capture));
                movelist_add(ml, move_encode(source, attack_target,
                    PT_ROOK, MVF_Capture));
                movelist_add(ml, move_encode(source, attack_target,
                    PT_BISHOP, MVF_Capture));
                movelist_add(ml, move_encode(source, attack_target,
                    PT_KNIGHT, MVF_Capture));
            } else
                movelist_add(ml, move_encode(source, attack_target,
                    PT_NONE, MVF_Capture));
            // Remove attack 'source' bit
            pop_bit(attack_copy, attack_target);
        }
        // Generate enpassant capture
        if (b->state.enpassant != SQ_NONE) {
            Bitboard enpassCapture =
                pawn_attacks[b->state.side][source] & (1ULL << b->state.enpassant);
            if (enpassCapture) {
                int enpassTarget = bb_lsb_index(enpassCapture);
                movelist_add(ml, move_encode(
                    source, enpassTarget, PT_NONE, MVF_Enpassant));
            }
        }
        // Remove bits
        pop_bit(bb_copy, source);
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
    Bitboard bitboard_copy = b->pos.piece[piece], attack_copy;
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
                movelist_add(ml,
                    move_encode(source, target, PT_NONE, MVF_Capture));
            else
                movelist_add(ml,
                    move_encode(source, target, PT_NONE, MVF_Quiet));
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
                movelist_add(ml,
                    move_encode(SQ_E1, SQ_G1, PT_NONE, MVF_Castling));
        }
    }
    // Queenside castling
    if (get_bit(b->state.castling, CR_LQ)) {
        // Check if path is obstructed
        if (!get_bit(b->pos.all_units, SQ_B1) && !get_bit(b->pos.all_units, SQ_C1) &&
            !get_bit(b->pos.all_units, SQ_D1)) {
            // Is d1 or e1 attacked by a black piece?
            if (!board_is_sq_attacked(b, SQ_D1, C_BLACK) && !board_is_sq_attacked(b, SQ_E1, C_BLACK))
                movelist_add(ml,
                    move_encode(SQ_E1, SQ_C1, PT_NONE, MVF_Castling));
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
                movelist_add(ml,
                    move_encode(SQ_E8, SQ_G8, PT_NONE, MVF_Castling));
        }
    }
    // Queenside castling
    if (get_bit(b->state.castling, CR_DQ)) {
        // Check if path is obstructed
        if (!get_bit(b->pos.all_units, SQ_B8) && !get_bit(b->pos.all_units, SQ_C8) &&
            !get_bit(b->pos.all_units, SQ_D8)) {
            // Is d8 or e8 attacked by a white piece?
            if (!board_is_sq_attacked(b, SQ_D8, C_WHITE) && !board_is_sq_attacked(b, SQ_E8, C_WHITE))
                movelist_add(ml,
                    move_encode(SQ_E8, SQ_C8, PT_NONE, MVF_Castling));
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
        case PT_PAWN  : movelist_gen_pawn(ml, b); break;
        case PT_KNIGHT: movelist_gen_qrnbk(ml, b, PT_KNIGHT); break;
        case PT_BISHOP: movelist_gen_qrnbk(ml, b, PT_BISHOP); break;
        case PT_ROOK  : movelist_gen_qrnbk(ml, b, PT_ROOK); break;
        case PT_QUEEN : movelist_gen_qrnbk(ml, b, PT_QUEEN); break;
        case PT_KING  : movelist_gen_king(ml, b); break;
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
    Board copy;
    for (int i = 0; i < temp.count; i++) {
        copy = *b;
        if (move_make(b, temp.list[i], AllMoves)) {
            ml->list[ml->count] = temp.list[i];
            ml->count++;
        }
        *b = copy;
    }
}
