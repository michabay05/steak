#include "engine.h"

#include "psqt.h"

int MATERIAL_SCORE[12] = {
     100,  300,  350,  500,  1000,  10000,
    -100, -300, -350, -500, -1000, -10000,
};

int evaluate(Board *board) {
    int score = 0;

    Bitboard bb;
    Sq square;

    for (Piece piece = P_LP; piece < P_COUNT; piece++) {
        bb = board->pos.piece[piece];
        while (bb) {
            square = bb_lsb_index(bb);

            score += MATERIAL_SCORE[piece];

            switch (piece) {
                case P_LP: score += PAWN_PSQT[square]; break;
                case P_LN: score += KNIGHT_PSQT[square]; break;
                case P_LB: score += BISHOP_PSQT[square]; break;
                case P_LR: score += ROOK_PSQT[square]; break;
                case P_LK: score += KING_PSQT[square]; break;

                case P_DP: score += PAWN_PSQT[FLIP(square)]; break;
                case P_DN: score += KNIGHT_PSQT[FLIP(square)]; break;
                case P_DB: score += BISHOP_PSQT[FLIP(square)]; break;
                case P_DR: score += ROOK_PSQT[FLIP(square)]; break;
                case P_DK: score += KING_PSQT[FLIP(square)]; break;

                case P_LQ:
                case P_DQ: score += 0; break;

                case P_NONE:
                default:
                    fprintf(stderr, "Unknown kind of piece: %d\n", piece);
                    break;
            }

            pop_bit(bb, square);
        }
    }

    return board->state.side == C_WHITE ? score : -score;
}

