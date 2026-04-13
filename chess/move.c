Move move_encode(Sq source, Sq target, Piece piece, Piece promoted, bool isCapture,
                 bool isTwoSquarePush, bool isEnpassant, bool isCastling) {
    return source | (target << 6) | (piece << 12) | (promoted << 16) | (isCapture << 20) |
           (isTwoSquarePush << 21) | (isEnpassant << 22) | (isCastling << 23);
}

Sq move_get_source(const Move move) { return move & 0x3F; }
Sq move_get_target(const Move move) { return (move & 0xFC0) >> 6; }
Piece move_get_piece(const Move move) { return (move & 0xF000) >> 12; }
Piece move_get_promoted(const Move move) {
    int promoted = (move & 0xF0000) >> 16;
    return promoted ? promoted : E;
}

char move_promoted_char(const Move move) {
    char promoted;
    Piece promoted_piece = move_get_promoted(move);
    switch (promoted_piece) {
    case lQ:
        promoted = 'Q';
        break;
    case lR:
        promoted = 'R';
        break;
    case lB:
        promoted = 'B';
        break;
    case lN:
        promoted = 'N';
        break;
    case dQ:
        promoted = 'q';
        break;
    case dR:
        promoted = 'r';
        break;
    case dB:
        promoted = 'b';
        break;
    case dN:
        promoted = 'n';
        break;
    default:
        promoted = ' ';
        break;
    }
    return promoted;
}

bool move_is_capture(const Move move) { return move & 0x100000; }
bool move_is_two_square_push(const Move move) { return move & 0x200000; }
bool move_is_enpassant(const Move move) { return move & 0x400000; }
bool move_is_castling(const Move move) { return move & 0x800000; }

void move_to_str(const Move move, char *move_str) {
    // Source square
    move_str[0] = str_coords[move_get_source(move)][0];
    move_str[1] = str_coords[move_get_source(move)][1];
    // Target square
    move_str[2] = str_coords[move_get_target(move)][0];
    move_str[3] = str_coords[move_get_target(move)][1];
    // Promotion piece
    move_str[4] = move_promoted_char(move);
}

Move move_parse(char *move_str, Piece piece, bool is_capture, bool is_two_square_push,
                bool is_enpassant, bool is_castling) {
    int source = SQ(move_str[1] - '0', move_str[0] - 'a');
    int target = SQ(move_str[3] - '0', move_str[2] - 'a');
    Piece promoted = E;
    if (move_str && (move_str[4] >= 'a' && move_str[4] <= 'z')) {
        switch (move_str[4]) {
        case 'Q':
            promoted = lQ;
            break;
        case 'R':
            promoted = lR;
            break;
        case 'B':
            promoted = lB;
            break;
        case 'N':
            promoted = lN;
            break;
        case 'q':
            promoted = dQ;
            break;
        case 'r':
            promoted = dR;
            break;
        case 'b':
            promoted = dB;
            break;
        case 'n':
            promoted = dN;
            break;
        }
    }
    int ouptut = move_encode(source, target, piece, promoted, is_capture, is_two_square_push,
                             is_enpassant, is_castling);

    char move_temp[5];
    move_to_str(ouptut, move_temp);

    return ouptut;
}

bool move_make(Board *main, Move move, MoveType move_flag) {
    if (move_flag == CapturesOnly) {
        // Before recusively calling this method
        // ensure that this move is a capture
        if (move_is_capture(move)) {
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
    Sq source = move_get_source(move);
    Sq target = move_get_target(move);
    Piece piece = move_get_piece(move);
    Piece promoted = move_get_promoted(move);
    bool is_capture = move_is_capture(move);
    bool is_two_square_push = move_is_two_square_push(move);
    bool is_enpassant = move_is_enpassant(move);
    bool is_castling = move_is_castling(move);
    // Move piece from source sq to target sq
    pop_bit(main->pos.piece[piece], source);
    set_bit(main->pos.piece[piece], target);

    // If move is capture, remove the piece from the opponent's bitboard
    if (is_capture) {
        for (Piece p = (!main->state.side ? dP : lP); p <= (!main->state.side ? dK : lK); p++) {
            if (get_bit(main->pos.piece[p], target)) {
                pop_bit(main->pos.piece[p], target);
                // There's no need to keep looking for another piece because
                // only one piece can be captured
                break;
            }
        }
    }

    // If move is promotion, change the pawn to the desired piece
    if (promoted != E) {
        pop_bit(main->pos.piece[piece], target);
        set_bit(main->pos.piece[promoted], target);
    }

    // Unlike other captures, make sure to remove the "enpassant'd" pawn from the enemy bitboard
    if (is_enpassant) {
        Piece pawn = dP;
        Direction dir = NORTH;
        if (main->state.side) {
            pawn = lP;
            dir = SOUTH;
        }
        pop_bit(main->pos.piece[pawn], target + dir);
    }
    // Reset enpassant square, even if the current move was enpassant or not
    // because enpassant can only be played on the move after the two square pawn push
    main->state.enpassant = noSq;
    if (is_two_square_push) {
        if (!main->state.side)
            main->state.enpassant = target + NORTH;
        else
            main->state.enpassant = target + SOUTH;
    }

    // If move is castling, place the rook on the correct square
    // FYI, the king is already on the correct square (as specified in the move generation)
    if (is_castling) {
        // Target = king's target square
        switch (target) {
        case g1:
            pop_bit(main->pos.piece[lR], h1);
            set_bit(main->pos.piece[lR], f1);
            break;
        case c1:
            pop_bit(main->pos.piece[lR], a1);
            set_bit(main->pos.piece[lR], d1);
            break;
        case g8:
            pop_bit(main->pos.piece[dR], h8);
            set_bit(main->pos.piece[dR], f8);
            break;
        case c8:
            pop_bit(main->pos.piece[dR], a8);
            set_bit(main->pos.piece[dR], d8);
            break;
        default:
            break;
        }
    }
    main->state.castling &= castling_rights[source];
    main->state.castling &= castling_rights[target];
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
