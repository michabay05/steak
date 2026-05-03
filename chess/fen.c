#include "fen.h"

inline FENInfo parse_fen_cstr(const char *fen) {
    return parse_fen_sv(sv_from_cstr(fen));
}

FENInfo parse_fen_sv(String_View fen) {
    u32 i = 0;
    FENInfo info = {0};
    // Piece placements
    u8 rank = 7, file = 0;

    // Set every square to be empty before setting values
    for (int i = 0; i < 64; i++)
        info.board[i] = P_NONE;

    while (i < fen.count && fen.data[i] != ' ') {
        if (fen.data[i] == '/') {
            rank--;
            file = 0;
            i++;
            continue;
        } else if (fen.data[i] >= '0' && fen.data[i] <= '9') {
            file += fen.data[i] - '0';
            i++;
            continue;
        }

        Sq sq = SQ(rank, file);
        switch (fen.data[i]) {
            case 'K': info.board[sq] = P_LK; break;
            case 'Q': info.board[sq] = P_LQ; break;
            case 'R': info.board[sq] = P_LR; break;
            case 'B': info.board[sq] = P_LB; break;
            case 'N': info.board[sq] = P_LN; break;
            case 'P': info.board[sq] = P_LP; break;
            case 'k': info.board[sq] = P_DK; break;
            case 'q': info.board[sq] = P_DQ; break;
            case 'r': info.board[sq] = P_DR; break;
            case 'b': info.board[sq] = P_DB; break;
            case 'n': info.board[sq] = P_DN; break;
            case 'p': info.board[sq] = P_DP; break;
        }
        file++;
        i++;
    }
    // Push pointer one more to account for space
    i++;
    info.side = !(fen.data[i] == 'w');
    i++;

    // Account for space and place on next char
    i++;

    while (i < fen.count && fen.data[i] != ' ') {
        switch (fen.data[i]) {
        case 'K':
            set_bit(info.castling, 0);
            break;
        case 'Q':
            set_bit(info.castling, 1);
            break;
        case 'k':
            set_bit(info.castling, 2);
            break;
        case 'q':
            set_bit(info.castling, 3);
            break;
        }
        i++;
    }
    // Account for space and place on next char
    i++;

    info.enpassant = SQ_NONE;
    if (fen.data[i] != '-') {
        int file = fen.data[i] - 'a';
        i++;
        int rank = 8 - (fen.data[i] - '0');
        info.enpassant = SQ(rank, file);
        i++;
    } else {
        i++;
    }

    // Account for space and place on next char
    i++;

    info.half_moves = fen.data[i] - '0';
    i++;
    // Account for space and place on next char
    i++;
    info.full_moves = fen.data[i] - '0';
    return info;
}

void fen_info_print(FENInfo *fen) {
    for (int r = 0; r < 8; r++) {
        printf("|");
        for (int f = 0; f < 8; f++) {
            printf(" %2d |", fen->board[SQ(r, f)]);
        }
        printf("\n");
    }
    printf("\n");
    printf("      Side: %c\n", !fen->side ? 'w' : 'b');
    printf("  Castling: %d\n", fen->castling);
    printf(" Enpassant: %d\n", fen->enpassant);
    printf("Full moves: %d\n", fen->full_moves);
    printf("Half moves: %d\n", fen->half_moves);
}
