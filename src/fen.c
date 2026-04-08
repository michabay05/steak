#include "fen.h"

FENInfo parse_fen(const char *fen) {
    FENInfo info = {0};
    // Piece placements
    int ind = 0;

    // Set every square to be empty before setting values
    for (int i = 0; i < 64; i++)
        info.board[i] = E;

    while (fen && *fen != ' ') {
        if (*fen == '/') {
            fen++;
            continue;
        } else if (*fen >= '0' && *fen <= '9') {
            ind += *fen - '0';
            fen++;
            continue;
        }

        switch (*fen) {
        case 'K':
            info.board[ind] = lK;
            break;
        case 'Q':
            info.board[ind] = lQ;
            break;
        case 'R':
            info.board[ind] = lR;
            break;
        case 'B':
            info.board[ind] = lB;
            break;
        case 'N':
            info.board[ind] = lN;
            break;
        case 'P':
            info.board[ind] = lP;
            break;
        case 'k':
            info.board[ind] = dK;
            break;
        case 'q':
            info.board[ind] = dQ;
            break;
        case 'r':
            info.board[ind] = dR;
            break;
        case 'b':
            info.board[ind] = dB;
            break;
        case 'n':
            info.board[ind] = dN;
            break;
        case 'p':
            info.board[ind] = dP;
            break;
        }
        ind++;
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
