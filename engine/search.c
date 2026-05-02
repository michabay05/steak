#include "engine.h"

// NOTE: Most valuable victim, Least valuable attacker
// The primary idea behind this is to guide the move ordering process.
// Typically, captures are often more worthwhile for the engine to look into as
// opposed to other quiet moves. For instance, let's say that the opponent
// captures our queen. Spending search time looking through options other than
// recapturing the queen will, more often than not, yield in a waste of time.
// Among capture moves, some are more valuable than others like a pawn
// capturing a queen. It is that idea that is encoded in the array below. Moves
// where the least valuable attacker captures the most valuable victim are
// sorted higher in the searching process rather the opposite (most valuable
// attacker capturing the least valuable victim: aka. queen capturing a pawn).

/*
    (Victims) Pawn Knight Bishop   Rook  Queen   King
  (Attackers)
        Pawn   105    205    305    405    505    605
      Knight   104    204    304    404    504    604
      Bishop   103    203    303    403    503    603
        Rook   102    202    302    402    502    602
       Queen   101    201    301    401    501    601
        King   100    200    300    400    500    600
*/

// MVV LVA [attacker][victim]
static const int MVV_LVA[12][12] = {
    {105, 205, 305, 405, 505, 605,  105, 205, 305, 405, 505, 605},
	{104, 204, 304, 404, 504, 604,  104, 204, 304, 404, 504, 604},
	{103, 203, 303, 403, 503, 603,  103, 203, 303, 403, 503, 603},
	{102, 202, 302, 402, 502, 602,  102, 202, 302, 402, 502, 602},
	{101, 201, 301, 401, 501, 601,  101, 201, 301, 401, 501, 601},
	{100, 200, 300, 400, 500, 600,  100, 200, 300, 400, 500, 600},

    {105, 205, 305, 405, 505, 605,  105, 205, 305, 405, 505, 605},
	{104, 204, 304, 404, 504, 604,  104, 204, 304, 404, 504, 604},
	{103, 203, 303, 403, 503, 603,  103, 203, 303, 403, 503, 603},
	{102, 202, 302, 402, 502, 602,  102, 202, 302, 402, 502, 602},
	{101, 201, 301, 401, 501, 601,  101, 201, 301, 401, 501, 601},
	{100, 200, 300, 400, 500, 600,  100, 200, 300, 400, 500, 600}
};

#define INFINITY 50000
// Used in Late-Move Reductions (LMRs)
#define FULL_DEPTH_MOVES 4
#define REDUCTION_LIMIT 3
#define NULL_MOVE_REDUCTION 2

struct {
    u32 ply;
    u32 nodes;

    // NOTE: Killer moves
    // Typically, captures produce beta-cutoff (aka. moves that fail high).
    // However, sometimes, quiet moves also produce beta cutoffs. Killer moves
    // store those quiet moves that produce beta-cutoffs. It only stores 2
    // killer moves.
    Move killer_moves[2][MAX_PLY];

    // NOTE: History moves
    // The purpose of this history move array is to store what piece, on what
    // target square causes the lower bound (aka. the alpha) to increase. Much
    // like the killer moves, these history moves are quiet moves that increase
    // the alpha (lower bound)
    i16 history_moves[12][64];

    // NOTE: This PV node collection system was taken from TSCP (as stated by
    // maksimKorzh)
    u8 pv_length[MAX_PLY];
    Move pv_table[MAX_PLY][MAX_PLY];

    bool follow_pv, score_pv;
} S_INFO = {0};

void enable_pv_scoring(MoveList *mvs) {
    S_INFO.follow_pv = false;

    for (int i = 0; i < mvs->count; i++) {
        if (move_eq(S_INFO.pv_table[0][S_INFO.ply], mvs->list[i])) {
            S_INFO.score_pv = true;
            S_INFO.follow_pv = true;
        }
    }
}

int score_move(Board *board, Move mv) {
    // NOTE: About the constants below (20k, 10k, 9k, etc.)
    // These constants do not have any real meaning to them. The only reason
    // they are there is to establish some semblance of priority. Without the
    // constants, the scores are typically below 1000 each. As a result,
    // sometimes a history move might be scored higher than a capture move,
    // which is not intended. Therefore, to ensure the following order (PV
    // moves > Capture > 1st killer > 2nd killer > history > unsorted), these
    // constants were add. It might as well have been (2M, 1M, 900k, 800k),
    // instead of (20k, 10k, 9k, 8k).

    if (S_INFO.score_pv) {
        // TODO: Implement move comparison
        if (move_eq(S_INFO.pv_table[0][S_INFO.ply], mv)) {
            S_INFO.score_pv = false;
            return 20000;
        }
    }

    Piece piece = pos_get_piece(board->pos, mv.source);
    if (mv.flag == MVF_Capture) {
        Piece captured_piece = P_LP;

        // pick up bitboard piece index ranges depending on side
        Piece start, end;

        // pick up side to move
        if (board->state.side == C_WHITE) {
            start = P_DP;
            end = P_DK;
        } else {
            start = P_LP;
            end = P_LK;
        }

        // loop over bitboards opposite to the current side to move
        for (Piece piece = start; piece <= end; piece++) {
            // if there's a piece on the target square
            if (get_bit(board->pos.piece[piece], mv.target)) {
                // remove it from corresponding bitboard
                captured_piece = piece;
                break;
            }
        }

        return MVV_LVA[piece][captured_piece] + 10000;
    } else {
        if (move_eq(S_INFO.killer_moves[0][S_INFO.ply], mv)) {
            // Score 1st killer move
            return 9000;
        } else if (move_eq(S_INFO.killer_moves[1][S_INFO.ply], mv)) {
            // Score 2nd killer move
            return 8000;
        } else {
            // Score history move
            return S_INFO.history_moves[piece][mv.target];
        }
    }
}

void sort_moves(Board *board, MoveList *mvs) {
    int move_scores[MOVE_GEN_MAX] = {0};

    // Score all the moves within the move list
    for (int count = 0; count < mvs->count; count++)
        move_scores[count] = score_move(board, mvs->list[count]);

    // loop over current move within a move list
    for (int current_move = 0; current_move < mvs->count; current_move++) {
        // loop over next move within a move list
        for (int next_move = current_move + 1; next_move < mvs->count; next_move++) {
            // compare current and next move scores
            if (move_scores[current_move] < move_scores[next_move]) {
                // swap scores
                int temp_score = move_scores[current_move];
                move_scores[current_move] = move_scores[next_move];
                move_scores[next_move] = temp_score;

                // swap moves
                Move temp_move = mvs->list[current_move];
                mvs->list[current_move] = mvs->list[next_move];
                mvs->list[next_move] = temp_move;
            }
        }
    }
}

static void _uci_checkup() {
    int time_ms = nanos_since_unspecified_epoch() / (1000 * 1000);
    if (U_INFO.timeset && time_ms > U_INFO.stoptime) {
        U_INFO.stopped = true;
    }

    // TODO: read GUI input
}

int quiescence(Board *board, int alpha, int beta) {
    if ((S_INFO.nodes & 2047) == 0) {
        _uci_checkup();
    }

    S_INFO.nodes++;

    int eval = evaluate(board);

    // Fail-hard beta cutoff: node (move) fails high
    if (eval >= beta) return beta;

    // PV node (move)
    if (eval > alpha) alpha = eval;

    MoveList mvs = {0};
    movelist_generate_all(&mvs, board);
    sort_moves(board, &mvs);

    for (int i = 0; i < mvs.count; i++) {
        Move mv = mvs.list[i];
        Board clone = *board;
        S_INFO.ply++;

        if (!move_make(board, mv, CapturesOnly)) {
            S_INFO.ply--;
            continue;
        }

        int score = -quiescence(board, -beta, -alpha);
        S_INFO.ply--;

        // Take move back
        *board = clone;

        // If time is up, return quickly
        if (U_INFO.stopped) return 0;

        // Fail-hard beta cutoff: node (move) fails high
        if (score >= beta) return beta;

        if (score > alpha) {
            // A better move has been found
            // PV node (move)
            alpha = score;
        }
    }

    // Node (move) fails low
    return alpha;
}


// NOTE: Fail-hard vs Fail-soft framework
// In fail-hard framework, the score returned by negamax is restricted to fall
// within [alpha, beta]; however, in the fail-soft framework, the score may 
// extend outside the [alpha, beta] interval

// NOTE: Alpha-beta pruning
// When starting the negamax function, alpha and beta are assumed to represent
// the lowest and highest bounds for the score produced (under the fail-hard
// framework. If score is less than alpha, then the associated root move is
// ignored because the move associated with alpha is better. If a score is
// greater than beta, the associated root move is ignored because the opponent,
// who is assumed to be a just as rational as us, would never allow us to go
// do those sequence of moves yielding a score greater than beta. If a score
// is greater than alpha but less than beta, the associated root move is
// considered to be the best. As a result, the lower bound (alpha) is updated
// to be this score because any score less than this new alpha would be ignored.
int negamax(Board *board, int alpha, int beta, int depth) {
    if ((S_INFO.nodes & 2047) == 0) {
        _uci_checkup();
    }

    // bool found_pv = true;

    S_INFO.pv_length[S_INFO.ply] = S_INFO.ply;

    if (depth <= 0) return quiescence(board, alpha, beta);

    // Exit condition: TOO_DEEP_IN_SEARCH_TREE (may even overflow some arrays)
    if (S_INFO.ply > MAX_PLY - 1) return evaluate(board);
    S_INFO.nodes++;

    // Check extension
    bool in_check = board_is_sq_attacked(board,
        bb_lsb_index(
            board->pos.piece[board->state.side == C_WHITE ? P_LK : P_DK]),
        board->state.side ^ 1
    );
    if (in_check) depth++;

    // NOTE: Null Move Pruning
    // Given that the objective here is to increase the number of beta-cutoff
    // (fail-high cutoffs), the idea behind null move pruning is to allow the
    // opponent to make an additional move. If the current position is so good
    // that allowing the opponent to make a move does not cause a disruption,
    // we can prune nodes (moves) that come after this node.
    if (depth >= 3 && !in_check && S_INFO.ply != 0) {
        // Preserve board state
        Board clone = *board;

        // Switch sides (essentially gifting the opponent an extra move)
        state_change_side(&board->state);

        // Reset enpassant square (b/c on the opponent's additional move we
        // don't want the opponent to make an enpassant capture...as it does
        // not make any sense to do so)
        board->state.enpassant = SQ_NONE;

        // Search moves with reduced depht to find beta cutoffs
        int score = -negamax(board, -beta, -beta + 1, depth - 1 - NULL_MOVE_REDUCTION);

        // Take move back
        *board = clone;

        if (U_INFO.stopped) return 0;

        if (score >= beta) return beta;
    }

    int legal_move_count = 0;
    // Number of moves searched in the move list
    int moves_searched = 0;
    MoveList mvs = {0};
    movelist_generate_all(&mvs, board);
    if (S_INFO.follow_pv) enable_pv_scoring(&mvs);
    sort_moves(board, &mvs);

    for (int i = 0; i < mvs.count; i++) {
        Move mv = mvs.list[i];
        Board clone = *board;
        S_INFO.ply++;

        if (!move_make(board, mv, AllMoves)) {
            S_INFO.ply--;
            continue;
        }
        legal_move_count++;

        int score;
        if (moves_searched == 0) {
            score = -negamax(board, -beta, -alpha, depth - 1);
        } else {
            // Late-Move Reductions

            // TODO: Research Chess Programming Wiki to find out if there are more conditions used to determine if it is ok to reduce
            bool ok_to_reduce = !in_check && mv.flag != MVF_Capture
                && mv.promoted == PT_NONE;

            if (moves_searched >= FULL_DEPTH_MOVES
                && depth >= REDUCTION_LIMIT && ok_to_reduce)
            {
                score = -negamax(board, -alpha - 1, -alpha, depth - 2);
            } else {
                // Hack to ensure full depth search is done
                score = alpha + 1;
            }

            // NOTE: Principal Variation Search (PVS)
            if (score > alpha) {
                // This recursive call serves to ensure that the PV node is
                // actually the best possible move this side can make.
                score = -negamax(board, -alpha - 1, -alpha, depth - 1);

                // If the previous call failed to uphold the assumption that the
                // current move is the best move for this side (aka. that current
                // move is the PV node), then the search is done again but this
                // time with the full bandwidth. Although the recursive call is
                // being called twice, the cons as a result of it pale in
                // comparison to the savings offered by PVS
                if ((score > alpha) && (score < beta)) {
                    // Further aside: If score was in fact greater than the alpha
                    // value, then the assumption that this move was in the PV node
                    // no longer holds thus, the entire search should done all over
                    // again.
                    score = -negamax(board, -beta, -alpha, depth - 1);
                }
            }
        }

        S_INFO.ply--;

        // Take move back
        *board = clone;

        if (U_INFO.stopped) return 0;

        moves_searched++;

        // Fail-hard beta cutoff: node (move) fails high
        if (score >= beta) {
            if (mv.flag != MVF_Capture) {
                S_INFO.killer_moves[1][S_INFO.ply] = S_INFO.killer_moves[0][S_INFO.ply];
                S_INFO.killer_moves[0][S_INFO.ply] = mv;
            }
            return beta;
        }

        if (score > alpha) {
            // A better move has been found
            if (mv.flag != MVF_Capture) {
                S_INFO.history_moves[pos_get_piece(board->pos, mv.source)][mv.target] += depth;
            }

            // PV node (move)
            alpha = score;

            // Update PV move list
            S_INFO.pv_table[S_INFO.ply][S_INFO.ply] = mv;

            // Copy move from deeper ply into a current ply's line
            for (int next = S_INFO.ply + 1; next < S_INFO.pv_length[S_INFO.ply + 1]; next += 1) {
                S_INFO.pv_table[S_INFO.ply][next] = S_INFO.pv_table[S_INFO.ply + 1][next];
            }

            // Adjust PV length
            S_INFO.pv_length[S_INFO.ply] = S_INFO.pv_length[S_INFO.ply + 1];
        }
    }

    if (legal_move_count == 0) {
        // No legal moves found in current position; either checkmate or
        // stalemate
        if (in_check) {
            // NOTE: The '+ ply' here is very important because removing
            // would prevent the engine from detecting checkmates at higher
            // depths.

            // return mating score
            return -49000 + S_INFO.ply;
        } else {
            // return stalemate score
            return 0;
        }
    }

    // Node (move) fails low
    return alpha;
}

void search_position(Board *board, int depth) {
    // Reset UCI "time's up flag"
    U_INFO.stopped = false;

    // Reset helper data structures for a new search loop.
    S_INFO.nodes = 0;
    S_INFO.follow_pv = false;
    S_INFO.score_pv = false;

    memset(S_INFO.killer_moves, 0, sizeof(S_INFO.killer_moves));
    memset(S_INFO.history_moves, 0, sizeof(S_INFO.history_moves));
    memset(S_INFO.pv_length, 0, sizeof(S_INFO.pv_length));
    memset(S_INFO.pv_table, 0, sizeof(S_INFO.pv_table));

    char buf[6] = {0};
    int alpha = -INFINITY, beta = INFINITY;
    // NOTE: Iterative Deepening
    // The primary objective of iterative deepening is to allow the engine to
    // pause before entering a deeper search to check if it has ran out of time
    // or not. As it progresses deeper and deeper into the search tree, it
    // prints out updates about the number of nodes search, PV nodes, etc.
    for (int curr_depth = 1; curr_depth <= depth; curr_depth++) {
        if (U_INFO.stopped) break;

        S_INFO.follow_pv = true;

        int score = negamax(board, alpha, beta, curr_depth);
        if (score <= alpha || score >= beta) {
            alpha = -INFINITY;
            beta = INFINITY;
            continue;
        }

        // NOTE: Aspiration Window
        // 'Aspiration Window' is an enhancement of iterative deepening. The core assumption behind
        // it that the next search will produce a score similar to current score. Similar score is
        // defined the current score plus or minus a certain window (in this case, 50 pts).
        alpha = score - 50;
        beta  = score + 50;

        String_Builder sb = {0};
        sb_appendf(&sb, "info score cp %d depth %d nodes %d time %lu pv",
            score, curr_depth, S_INFO.nodes,
            (nanos_since_unspecified_epoch() / (1000*1000)) - U_INFO.starttime
        );
        // Print all moves in the PV line
        for (int i = 0; i < S_INFO.pv_length[0]; i++) {
            move_to_str(S_INFO.pv_table[0][i], buf);
            sb_appendf(&sb, " %s", buf);
        }
        sb_append_null(&sb);
        printf("%s\n", sb.items);
    }

    move_to_str(S_INFO.pv_table[0][0], buf);
    printf("bestmove %s\n", buf);
}

