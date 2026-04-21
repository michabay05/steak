#include "fen.h"
#include <stdbool.h>
#include <stdint.h>

FENInfo parse_fen(const char *fen) {
    FENInfo info = {0};
    // Piece placements
    uint8_t rank = 7, file = 0;

    // Set every square to be empty before setting values
    for (int i = 0; i < 64; i++)
        info.board[i] = P_NONE;

    while (fen && *fen != ' ') {
        if (*fen == '/') {
            rank--;
            file = 0;
            fen++;
            continue;
        } else if (*fen >= '0' && *fen <= '9') {
            file += *fen - '0';
            fen++;
            continue;
        }

        switch (*fen) {
        case 'K':
            info.board[SQ(rank, file)] = P_LK;
            break;
        case 'Q':
            info.board[SQ(rank, file)] = P_LQ;
            break;
        case 'R':
            info.board[SQ(rank, file)] = P_LR;
            break;
        case 'B':
            info.board[SQ(rank, file)] = P_LB;
            break;
        case 'N':
            info.board[SQ(rank, file)] = P_LN;
            break;
        case 'P':
            info.board[SQ(rank, file)] = P_LP;
            break;
        case 'k':
            info.board[SQ(rank, file)] = P_DK;
            break;
        case 'q':
            info.board[SQ(rank, file)] = P_DQ;
            break;
        case 'r':
            info.board[SQ(rank, file)] = P_DR;
            break;
        case 'b':
            info.board[SQ(rank, file)] = P_DB;
            break;
        case 'n':
            info.board[SQ(rank, file)] = P_DN;
            break;
        case 'p':
            info.board[SQ(rank, file)] = P_DP;
            break;
        }
        file++;
        fen++;
    }
    // Push pointer one more to account for space
    fen++;
    info.side = !(*fen == 'w');
    fen++;

    // Account for space and place on next char
    fen++;

    while (fen && *fen != ' ') {
        switch (*fen) {
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
        fen++;
    }
    // Account for space and place on next char
    fen++;

    if (*fen != '-') {
        int file = *fen - 'a';
        fen++;
        int rank = 8 - (*fen - '0');
        info.enpassant = SQ(rank, file);
        fen++;
    } else {
        fen++;
    }

    // Account for space and place on next char
    fen++;

    info.half_moves = *fen - '0';
    fen++;
    // Account for space and place on next char
    fen++;
    info.full_moves = *fen - '0';
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
