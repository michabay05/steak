#include "engine.h"

#include "psqt.h"

int MATERIAL_SCORE[2][6] = {
    { 100,  300,  350,  500,  1000,  10000},
    {-100, -300, -350, -500, -1000, -10000},
};

int evaluate(Board *board) {
    int score = 0;

    Bitboard bb;
    Sq square;

    for (Color c = C_WHITE; c <= C_BLACK; c++) {
        for (PieceType pt = PT_PAWN; pt <= PT_KING; pt++) {
            bb = board->piece[c][pt];
            while (bb) {
                square = bb_lsb_index(bb);

                score += MATERIAL_SCORE[c][pt];

                switch (pt) {
                    case PT_PAWN:
                        score += (c == C_WHITE)
                            ? PAWN_PSQT[square] : -PAWN_PSQT[FLIP(square)];
                        break;

                    case PT_KNIGHT:
                        score += (c == C_WHITE)
                            ? KNIGHT_PSQT[square] : -KNIGHT_PSQT[FLIP(square)];
                        break;

                    case PT_BISHOP:
                        score += (c == C_WHITE)
                            ? BISHOP_PSQT[square] : -BISHOP_PSQT[FLIP(square)];
                        break;

                    case PT_ROOK:
                        score += (c == C_WHITE)
                            ? ROOK_PSQT[square] : -ROOK_PSQT[FLIP(square)];
                        break;

                    case PT_KING:
                        score += (c == C_WHITE)
                            ? KING_PSQT[square] : -KING_PSQT[FLIP(square)];
                        break;

                    case PT_QUEEN: score += 0; break;

                    case PT_NONE:
                    default:
                        fprintf(stderr, "Unknown kind of piece: %d\n", pt);
                        break;
                }

                pop_bit(bb, square);
            }
        }
    }

    return board->side == C_WHITE ? score : -score;
}

