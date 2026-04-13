typedef struct {
    uint64_t piece[12];
    uint64_t units[3];
} Position;

typedef struct {
    PieceColor side;
    int castling;
    Sq enpassant;
    int half_moves;
    int full_moves;
} State;

typedef struct {
    Position pos;
    State state;
} Board;

extern const int castling_rights[64];

void pos_add_piece(Position *pos, Piece piece, Sq sq);
void pos_remove_piece(Position *pos, Piece piece, Sq sq);
Piece pos_get_piece(const Position pos, Sq sq);
void pos_update_units(Position *pos);
void state_change_side(State *state);
void board_set_from_fen(Board *board, FENInfo fen);
void board_print(const Board *const b);
bool board_is_sq_attacked(const Board *const b, Sq sq, PieceColor side);
bool board_is_in_check(const Board *const b);
