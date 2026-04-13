void movelist_add(MoveList *ml, Move move) {
    assert(ml->count < MOVE_GEN_MAX);
    ml->list[ml->count++] = move;
}

Move movelist_search(const MoveList ml, Sq source, Sq target, Piece promoted) {
    for (int i = 0; i < ml.count; i++) {
        // Parse move info
        Sq listMoveSource = move_get_source(ml.list[i]);
        Sq listMoveTarget = move_get_target(ml.list[i]);
        Piece listMovePromoted = move_get_promoted(ml.list[i]);
        // Check if source and target match
        if (listMoveSource == source && listMoveTarget == target && listMovePromoted == promoted)
            // Return index of move from movelist, if true
            return ml.list[i];
    }
    return 0;
}

void movelist_print_list(const MoveList ml) {
    printf("    Source   |   Target  |  Piece  |  Promoted  |  Capture  |  Two "
           "Square Push  |  Enpassant  |  Castling\n");
    printf("  "
           "---------------------------------------------------------------------"
           "--------------------------------------\n");
    for (int i = 0; i < ml.count; i++) {
        printf("       %s    |    %s     |    %c    |     %c      |     %d     |   "
               "      %d         |      %d      |     %d\n",
               str_coords[move_get_source(ml.list[i])], str_coords[move_get_target(ml.list[i])],
               piece_char[move_get_piece(ml.list[i])], piece_char[move_get_promoted(ml.list[i])],
               move_is_capture(ml.list[i]), move_is_two_square_push(ml.list[i]),
               move_is_enpassant(ml.list[i]), move_is_castling(ml.list[i]));
    }
    printf("\n    Total number of moves: %d\n", ml.count);
}

static void movelist_gen_pawn(MoveList *ml, const Board *const b) {
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
                movelist_add(ml, move_encode(source, target, piece,
                                             (b->state.side == LIGHT ? lQ : dQ), 0, 0, 0, 0));
                movelist_add(ml, move_encode(source, target, piece,
                                             (b->state.side == LIGHT ? lR : dR), 0, 0, 0, 0));
                movelist_add(ml, move_encode(source, target, piece,
                                             (b->state.side == LIGHT ? lB : dB), 0, 0, 0, 0));
                movelist_add(ml, move_encode(source, target, piece,
                                             (b->state.side == LIGHT ? lN : dN), 0, 0, 0, 0));
            } else {
                movelist_add(ml, move_encode(source, target, piece, E, 0, 0, 0, 0));
                if ((source >= doublePushStart && source <= doublePushStart + 7) &&
                    !get_bit(b->pos.units[BOTH], target + direction))
                    movelist_add(ml, move_encode(source, target + direction, piece, E, 0, 1, 0, 0));
            }
        }
        // Capture moves
        attack_copy = pawn_attacks[b->state.side][source] & b->pos.units[b->state.side ^ 1];
        while (attack_copy) {
            target = bb_lsb_index(attack_copy);
            // Capture move
            if ((source >= promotionStart) && (source <= promotionStart + 7)) {
                movelist_add(ml, move_encode(source, target, piece,
                                             (b->state.side == LIGHT ? lQ : dQ), 1, 0, 0, 0));
                movelist_add(ml, move_encode(source, target, piece,
                                             (b->state.side == LIGHT ? lR : dR), 1, 0, 0, 0));
                movelist_add(ml, move_encode(source, target, piece,
                                             (b->state.side == LIGHT ? lB : dB), 1, 0, 0, 0));
                movelist_add(ml, move_encode(source, target, piece,
                                             (b->state.side == LIGHT ? lN : dN), 1, 0, 0, 0));
            } else
                movelist_add(ml, move_encode(source, target, piece, E, 1, 0, 0, 0));
            // Remove 'source' bit
            pop_bit(attack_copy, target);
        }
        // Generate enpassant capture
        if (b->state.enpassant != noSq) {
            uint64_t enpassCapture =
                pawn_attacks[b->state.side][source] & (1ULL << b->state.enpassant);
            if (enpassCapture) {
                int enpassTarget = bb_lsb_index(enpassCapture);
                movelist_add(ml, move_encode(source, enpassTarget, piece, E, 1, 0, 1, 0));
            }
        }
        // Remove bits
        pop_bit(bitboard_copy, source);
    }
}

static void movelist_gen_knight(MoveList *ml, const Board *const b) {
    int source, target, piece = b->state.side == LIGHT ? lN : dN;
    uint64_t bitboard_copy = b->pos.piece[piece], attack_copy;
    while (bitboard_copy) {
        source = bb_lsb_index(bitboard_copy);

        attack_copy = knight_attacks[source] &
                      (b->state.side == LIGHT ? ~b->pos.units[LIGHT] : ~b->pos.units[DARK]);
        while (attack_copy) {
            target = bb_lsb_index(attack_copy);
            if (get_bit(b->pos.units[b->state.side == LIGHT ? DARK : LIGHT], target))
                movelist_add(ml, move_encode(source, target, piece, E, 1, 0, 0, 0));
            else
                movelist_add(ml, move_encode(source, target, piece, E, 0, 0, 0, 0));
            pop_bit(attack_copy, target);
        }
        pop_bit(bitboard_copy, source);
    }
}

static void movelist_gen_bishop(MoveList *ml, const Board *const b) {
    int source, target, piece = b->state.side == LIGHT ? lB : dB;
    uint64_t bitboard_copy = b->pos.piece[piece], attack_copy;
    while (bitboard_copy) {
        source = bb_lsb_index(bitboard_copy);

        attack_copy = get_bishop_attack(source, b->pos.units[BOTH]) &
                      (b->state.side == LIGHT ? ~b->pos.units[LIGHT] : ~b->pos.units[DARK]);

        while (attack_copy) {
            target = bb_lsb_index(attack_copy);
            if (get_bit(b->pos.units[b->state.side == LIGHT ? DARK : LIGHT], target))
                movelist_add(ml, move_encode(source, target, piece, E, 1, 0, 0, 0));
            else
                movelist_add(ml, move_encode(source, target, piece, E, 0, 0, 0, 0));
            pop_bit(attack_copy, target);
        }
        pop_bit(bitboard_copy, source);
    }
}

static void movelist_gen_rook(MoveList *ml, const Board *const b) {
    int source, target, piece = b->state.side == LIGHT ? lR : dR;
    uint64_t bitboard_copy = b->pos.piece[piece], attack_copy;
    while (bitboard_copy) {
        source = bb_lsb_index(bitboard_copy);

        attack_copy = get_rook_attack(source, b->pos.units[BOTH]) &
                      (b->state.side == LIGHT ? ~b->pos.units[LIGHT] : ~b->pos.units[DARK]);
        while (attack_copy) {
            target = bb_lsb_index(attack_copy);
            if (get_bit(b->pos.units[b->state.side == LIGHT ? DARK : LIGHT], target))
                movelist_add(ml, move_encode(source, target, piece, E, 1, 0, 0, 0));
            else
                movelist_add(ml, move_encode(source, target, piece, E, 0, 0, 0, 0));
            pop_bit(attack_copy, target);
        }
        pop_bit(bitboard_copy, source);
    }
}

static void movelist_gen_queen(MoveList *ml, const Board *const b) {
    int source, target, piece = b->state.side == LIGHT ? lQ : dQ;
    uint64_t bitboard_copy = b->pos.piece[piece], attack_copy;
    while (bitboard_copy) {
        source = bb_lsb_index(bitboard_copy);

        attack_copy = get_queen_attack(source, b->pos.units[BOTH]) &
                      (b->state.side == LIGHT ? ~b->pos.units[LIGHT] : ~b->pos.units[DARK]);
        while (attack_copy) {
            target = bb_lsb_index(attack_copy);
            if (get_bit(b->pos.units[b->state.side == LIGHT ? DARK : LIGHT], target))
                movelist_add(ml, move_encode(source, target, piece, E, 1, 0, 0, 0));
            else
                movelist_add(ml, move_encode(source, target, piece, E, 0, 0, 0, 0));
            pop_bit(attack_copy, target);
        }
        pop_bit(bitboard_copy, source);
    }
}

static void movelist_gen_white_castling(MoveList *ml, const Board *const b) {
    // Kingside castling
    if (get_bit(b->state.castling, cr_lK)) {
        // Check if path is obstructed
        if (!get_bit(b->pos.units[BOTH], f1) && !get_bit(b->pos.units[BOTH], g1)) {
            // Is e1 or f1 attacked by a black piece?
            if (!board_is_sq_attacked(b, e1, DARK) && !board_is_sq_attacked(b, f1, DARK))
                movelist_add(ml, move_encode(e1, g1, lK, E, 0, 0, 0, 1));
        }
    }
    // Queenside castling
    if (get_bit(b->state.castling, cr_lQ)) {
        // Check if path is obstructed
        if (!get_bit(b->pos.units[BOTH], b1) && !get_bit(b->pos.units[BOTH], c1) &&
            !get_bit(b->pos.units[BOTH], d1)) {
            // Is d1 or e1 attacked by a black piece?
            if (!board_is_sq_attacked(b, d1, DARK) && !board_is_sq_attacked(b, e1, DARK))
                movelist_add(ml, move_encode(e1, c1, lK, E, 0, 0, 0, 1));
        }
    }
}

static void movelist_gen_black_castling(MoveList *ml, const Board *const b) {
    // Kingside castling
    if (get_bit(b->state.castling, cr_dK)) {
        // Check if path is obstructed
        if (!get_bit(b->pos.units[BOTH], f8) && !get_bit(b->pos.units[BOTH], g8)) {
            // Is e8 or f8 attacked by a white piece?
            if (!board_is_sq_attacked(b, e8, LIGHT) && !board_is_sq_attacked(b, f8, LIGHT))
                movelist_add(ml, move_encode(e8, g8, dK, E, 0, 0, 0, 1));
        }
    }
    // Queenside castling
    if (get_bit(b->state.castling, cr_dQ)) {
        // Check if path is obstructed
        if (!get_bit(b->pos.units[BOTH], b8) && !get_bit(b->pos.units[BOTH], c8) &&
            !get_bit(b->pos.units[BOTH], d8)) {
            // Is d8 or e8 attacked by a white piece?
            if (!board_is_sq_attacked(b, d8, LIGHT) && !board_is_sq_attacked(b, e8, LIGHT))
                movelist_add(ml, move_encode(e8, c8, dK, E, 0, 0, 0, 1));
        }
    }
}

static void movelist_gen_king(MoveList *ml, const Board *const b) {
    // NOTE: Checks aren't handled by the move generator, it's handled by the make move function.
    int source, target, piece = b->state.side == LIGHT ? lK : dK;
    uint64_t bitboard = b->pos.piece[piece], attack;
    while (bitboard != 0) {
        source = bb_lsb_index(bitboard);

        attack = king_attacks[source] &
                 (b->state.side == LIGHT ? ~b->pos.units[LIGHT] : ~b->pos.units[DARK]);
        while (attack != 0) {
            target = bb_lsb_index(attack);

            if (get_bit((b->pos.units[b->state.side == LIGHT ? DARK : LIGHT]), target))
                movelist_add(ml, move_encode(source, target, piece, E, 1, 0, 0, 0));
            else
                movelist_add(ml, move_encode(source, target, piece, E, 0, 0, 0, 0));

            // Remove target bit to move onto the next bit
            pop_bit(attack, target);
        }
        // Remove source bit to move onto the next bit
        pop_bit(bitboard, source);
    }
    // Generate castling moves
    if (b->state.side == LIGHT)
        movelist_gen_white_castling(ml, b);
    else
        movelist_gen_black_castling(ml, b);
}

void movelist_generate(MoveList *ml, const Board *const b, Piece p) {
    switch (COLORLESS(p)) {
    case PAWN:
        movelist_gen_pawn(ml, b);
        break;
    case KNIGHT:
        movelist_gen_knight(ml, b);
        break;
    case BISHOP:
        movelist_gen_bishop(ml, b);
        break;
    case ROOK:
        movelist_gen_rook(ml, b);
        break;
    case QUEEN:
        movelist_gen_queen(ml, b);
        break;
    case KING:
        movelist_gen_king(ml, b);
        break;
    }
}

void movelist_generate_all(MoveList *ml, const Board *const b) {
    movelist_gen_pawn(ml, b);
    movelist_gen_knight(ml, b);
    movelist_gen_bishop(ml, b);
    movelist_gen_rook(ml, b);
    movelist_gen_queen(ml, b);
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
