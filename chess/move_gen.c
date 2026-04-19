#include "../nob.h"

#include "move_gen.h"
#include "bitboard.h"
#include "defs.h"
#include "move.h"
#include "precalculate.h"
#include <stdint.h>

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
    uint64_t bitboard_copy, attack_copy;
    int promotionStart, direction, doublePushStart, piece;
    int source, target;
    // If side to move is white
    if (b->state.side == LIGHT) {
        piece = lP;
        promotionStart = a7;
        direction = SOUTH;
        doublePushStart = a2;
    }
    // If side to move is black
    else {
        piece = dP;
        promotionStart = a2;
        direction = NORTH;
        doublePushStart = a7;
    }

    bitboard_copy = b->pos.piece[piece];

    while (bitboard_copy) {
        source = bb_lsb_index(bitboard_copy);
        target = source + direction;
        if ((b->state.side == LIGHT ? target >= a8 : target <= h1) &&
            !get_bit(b->pos.units[BOTH], target)) {
            // Quiet moves
            // Promotion
            if ((source >= promotionStart) && (source <= promotionStart + 7)) {
                movelist_add(ml, move_encode(
                    source, target, piece, (b->state.side == LIGHT ? lQ : dQ), MVF_Quiet));
                movelist_add(ml, move_encode(source, target, piece,
                    (b->state.side == LIGHT ? lR : dR), MVF_Quiet));
                movelist_add(ml, move_encode(source, target, piece,
                    (b->state.side == LIGHT ? lB : dB), MVF_Quiet));
                movelist_add(ml, move_encode(source, target, piece,
                    (b->state.side == LIGHT ? lN : dN), MVF_Quiet));
            } else {
                movelist_add(ml, move_encode(source, target, piece, E, MVF_Quiet));
                if ((source >= doublePushStart && source <= doublePushStart + 7) &&
                    !get_bit(b->pos.units[BOTH], target + direction))
                    movelist_add(ml, move_encode(source, target + direction, piece, E, MVF_TwoSquarePush));
            }
        }
        // Capture moves
        attack_copy = pawn_attacks[b->state.side][source] & b->pos.units[b->state.side ^ 1];
        while (attack_copy) {
            target = bb_lsb_index(attack_copy);
            // Capture move
            if ((source >= promotionStart) && (source <= promotionStart + 7)) {
                movelist_add(ml, move_encode(source, target, piece,
                                             (b->state.side == LIGHT ? lQ : dQ), MVF_Capture));
                movelist_add(ml, move_encode(source, target, piece,
                                             (b->state.side == LIGHT ? lR : dR), MVF_Capture));
                movelist_add(ml, move_encode(source, target, piece,
                                             (b->state.side == LIGHT ? lB : dB), MVF_Capture));
                movelist_add(ml, move_encode(source, target, piece,
                                             (b->state.side == LIGHT ? lN : dN), MVF_Capture));
            } else
                movelist_add(ml, move_encode(source, target, piece, E, MVF_Capture));
            // Remove 'source' bit
            pop_bit(attack_copy, target);
        }
        // Generate enpassant capture
        if (b->state.enpassant != noSq) {
            uint64_t enpassCapture =
                pawn_attacks[b->state.side][source] & (1ULL << b->state.enpassant);
            if (enpassCapture) {
                int enpassTarget = bb_lsb_index(enpassCapture);
                movelist_add(ml, move_encode(source, enpassTarget, piece, E, MVF_Enpassant));
            }
        }
        // Remove bits
        pop_bit(bitboard_copy, source);
    }
}

static void movelist_gen_qrnbk(MoveList *ml, Board *b, PieceType pt) {
    Piece piece;
    switch (pt) {
        case KNIGHT: piece = b->state.side == LIGHT ? lN : dN; break;
        case BISHOP: piece = b->state.side == LIGHT ? lB : dB; break;
        case ROOK  : piece = b->state.side == LIGHT ? lR : dR; break;
        case QUEEN : piece = b->state.side == LIGHT ? lQ : dQ; break;
        case KING  : piece = b->state.side == LIGHT ? lK : dK; break;

        case PAWN:
        default: UNREACHABLE("Pawn or unknown piece type");
    }

    Sq source, target;
    uint64_t bitboard_copy = b->pos.piece[piece], attack_copy;
    while (bitboard_copy) {
        source = bb_lsb_index(bitboard_copy);

        switch (pt) {
            case KNIGHT: attack_copy = knight_attacks[source]; break;
            case BISHOP: attack_copy = get_bishop_attack(source, b->pos.units[BOTH]); break;
            case ROOK  : attack_copy = get_rook_attack(source, b->pos.units[BOTH]); break;
            case QUEEN : attack_copy = get_queen_attack(source, b->pos.units[BOTH]); break;
            case KING  : attack_copy = king_attacks[source]; break;

            case PAWN:
            default: UNREACHABLE("Pawn or unknown piece type");
        }

        attack_copy &= (b->state.side == LIGHT ? ~b->pos.units[LIGHT] : ~b->pos.units[DARK]);
        while (attack_copy) {
            target = bb_lsb_index(attack_copy);
            if (get_bit(b->pos.units[b->state.side == LIGHT ? DARK : LIGHT], target))
                movelist_add(ml, move_encode(source, target, piece, E, MVF_Capture));
            else
                movelist_add(ml, move_encode(source, target, piece, E, MVF_Quiet));
            pop_bit(attack_copy, target);
        }
        pop_bit(bitboard_copy, source);
    }
}

static void movelist_gen_white_castling(MoveList *ml, Board *b) {
    // Kingside castling
    if (get_bit(b->state.castling, cr_lK)) {
        // Check if path is obstructed
        if (!get_bit(b->pos.units[BOTH], f1) && !get_bit(b->pos.units[BOTH], g1)) {
            // Is e1 or f1 attacked by a black piece?
            if (!board_is_sq_attacked(b, e1, DARK) && !board_is_sq_attacked(b, f1, DARK))
                movelist_add(ml, move_encode(e1, g1, lK, E, MVF_Castling));
        }
    }
    // Queenside castling
    if (get_bit(b->state.castling, cr_lQ)) {
        // Check if path is obstructed
        if (!get_bit(b->pos.units[BOTH], b1) && !get_bit(b->pos.units[BOTH], c1) &&
            !get_bit(b->pos.units[BOTH], d1)) {
            // Is d1 or e1 attacked by a black piece?
            if (!board_is_sq_attacked(b, d1, DARK) && !board_is_sq_attacked(b, e1, DARK))
                movelist_add(ml, move_encode(e1, c1, lK, E, MVF_Castling));
        }
    }
}

static void movelist_gen_black_castling(MoveList *ml, Board *b) {
    // Kingside castling
    if (get_bit(b->state.castling, cr_dK)) {
        // Check if path is obstructed
        if (!get_bit(b->pos.units[BOTH], f8) && !get_bit(b->pos.units[BOTH], g8)) {
            // Is e8 or f8 attacked by a white piece?
            if (!board_is_sq_attacked(b, e8, LIGHT) && !board_is_sq_attacked(b, f8, LIGHT))
                movelist_add(ml, move_encode(e8, g8, dK, E, MVF_Castling));
        }
    }
    // Queenside castling
    if (get_bit(b->state.castling, cr_dQ)) {
        // Check if path is obstructed
        if (!get_bit(b->pos.units[BOTH], b8) && !get_bit(b->pos.units[BOTH], c8) &&
            !get_bit(b->pos.units[BOTH], d8)) {
            // Is d8 or e8 attacked by a white piece?
            if (!board_is_sq_attacked(b, d8, LIGHT) && !board_is_sq_attacked(b, e8, LIGHT))
                movelist_add(ml, move_encode(e8, c8, dK, E, MVF_Castling));
        }
    }
}

static void movelist_gen_king(MoveList *ml, Board *b) {
    // NOTE: Checks aren't handled by the move generator, it's handled by the make move function.

    movelist_gen_qrnbk(ml, b, KING);
    // Generate castling moves
    if (b->state.side == LIGHT)
        movelist_gen_white_castling(ml, b);
    else
        movelist_gen_black_castling(ml, b);
}

void movelist_generate(MoveList *ml, Board *b, Piece p) {
    switch (COLORLESS(p)) {
        case PAWN: movelist_gen_pawn(ml, b); break;
        case KNIGHT: movelist_gen_qrnbk(ml, b, KNIGHT); break;
        case BISHOP: movelist_gen_qrnbk(ml, b, BISHOP); break;
        case ROOK  : movelist_gen_qrnbk(ml, b, ROOK); break;
        case QUEEN: movelist_gen_qrnbk(ml, b, QUEEN); break;
        case KING: movelist_gen_king(ml, b); break;
    }
}

void movelist_generate_all(MoveList *ml, Board *b) {
    movelist_gen_pawn(ml, b);
    movelist_gen_qrnbk(ml, b, KNIGHT);
    movelist_gen_qrnbk(ml, b, BISHOP);
    movelist_gen_qrnbk(ml, b, ROOK);
    movelist_gen_qrnbk(ml, b, QUEEN);
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
