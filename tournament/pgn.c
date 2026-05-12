#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "pgn.h"
#include "board.h"
#include "defs.h"
#include "move.h"
#include "move_gen.h"

#define BUF_SIZE 64 * 1024
#define UNKNOWN_ROUND -1

typedef enum {
    PGN_TK_TAG_KEY = 1,
    PGN_TK_TAG_VALUE,
    PGN_TK_MOVE,
    PGN_TK_COMMENT,
    PGN_TK_GAME_OUTCOME,
} PGN_TokenKind;

typedef struct {
    String_View lexeme;
    PGN_TokenKind kind;
} PGN_Token;

typedef struct {
    PGN_Token *items;
    int count;
    int capacity;
} PGN_TokenList;

// ##############################################################################
// TODO: Eliminate this functions and replace them with sv_* functions from 'nob.h'
#define is_at_end(size, ind) (ind) >= (size)
static char peek_ahead(String_View sv, size_t ahead);
#define peek(sv) peek_ahead(sv, 0)

static String_View consume_ahead(String_View *sv, int n);
#define consume(sv) consume_ahead(sv, 1)

static int peek_while(String_View sv, int (*filter_func)(int c));
static void consume_while(String_View *out, String_View *sv, int (*filter_func)(int c));
// ##############################################################################

// static bool is_move_number(String_View *sv) {
//     char c;
//     while ((c = peek(*sv)) != '.') {
//         if (!isdigit(c))
//             return false;
//         consume(sv);
//     }
//     return true;
// }

static bool is_valid_move_letter(char c) {
    if (c == '\0')
        return false;

    const char *accepted_chars[5] = {
        "NBRQK",    // Piece types
        "abcdefgh", // file letters
        "12345678", // rank numbers
        "O0",       // Castling letter
        "-+#",      // Other symbols
    };
    for (int i = 0; i < 5; i++) {
        if (strchr(accepted_chars[i], c) != NULL)
            return true;
    }

    return false;
}

static bool is_move_text(String_View *sv) {
    if (sv == NULL)
        return false;

    char c = peek(*sv);
    while (c != '\0' && !isspace(c)) {
        if (is_valid_move_letter(c))
            return true;
        else
            consume(sv);

        c = peek(*sv);
    }
    return false;
}

static void token_append(PGN_TokenList *tl, String_View sv, PGN_TokenKind kind) {
    PGN_Token t = {.lexeme = sv, .kind = kind};

    da_append(tl, t);
}

static const char *token_to_cstr(PGN_Token t) {
    switch (t.kind) {
    case PGN_TK_TAG_KEY:
        return "key";
    case PGN_TK_TAG_VALUE:
        return "value";
    case PGN_TK_MOVE:
        return "move";
    case PGN_TK_COMMENT:
        return "comment";
    case PGN_TK_GAME_OUTCOME:
        return "outcome";
    default:
        UNREACHABLE("Unknown kind of token");
    }
}

/* ================== UTIL FUNCTIONS ================ */
int is_period(int c) { return c == '.'; }
int is_not_quote(int c) { return c != '"'; }
int is_not_space(int c) { return !isspace(c); }
int is_not_right_curly(int c) { return c != '}'; }
int is_not_period(int c) { return c != '.'; }
int is_digit(int c) { return isdigit(c); }
/* ================================================== */

static void pgn__parse_lines(Nob_String_View *sv, PGN_TokenList *tl) {
    String_View temp = {0};
    while (sv->count > 0) {
        char c = peek(*sv);
        if (c == '[') {
            // [KEY "VALUE"]
            // PGN meta data
            consume(sv); // Consume [

            consume_while(&temp, sv, &is_not_space);
            token_append(tl, temp, PGN_TK_TAG_KEY);

            consume(sv); // Consume ' '
            consume(sv); // Consume '"'

            consume_while(&temp, sv, &is_not_quote);
            token_append(tl, temp, PGN_TK_TAG_VALUE);

            consume(sv); // Consume '"'
            consume(sv); // Consume the closing square bracket mark
        } else if (c == '{') {
            consume(sv); // Consume '{'

            consume_while(&temp, sv, &is_not_right_curly);
            token_append(tl, temp, PGN_TK_COMMENT);

            consume(sv); // Consume '}'
        } else if (isdigit(c)) {
            int offset = peek_while(*sv, &is_digit);
            char ltr = peek_ahead(*sv, offset);
            if (ltr == '.') {
                // Move number
                // NOTE: +1 for the period
                consume_ahead(sv, offset + 1);
            } else if (ltr == '-') {
                String_View outcome = consume_ahead(sv, offset + 2);
                token_append(tl, outcome, PGN_TK_GAME_OUTCOME);
            }
        } else if (is_move_text(sv)) {
            consume_while(&temp, sv, &is_not_space);
            token_append(tl, temp, PGN_TK_MOVE);
        } else if (isspace(c)) {
            consume(sv);
        }
    }
}

static PGN_GameResult pgn__parse_result(String_View lexeme) {
    if (sv_eq(lexeme, sv_from_cstr("1-0")))
        return PGN_GR_WHITE_WINS;
    else if (sv_eq(lexeme, sv_from_cstr("0-1")))
        return PGN_GR_BLACK_WINS;
    else if (sv_eq(lexeme, sv_from_cstr("1/2-1/2")))
        return PGN_GR_DRAW;
    else
        // @NOTE: an asterisk result '*' indicates that a
        // game is still ongoing
        return PGN_GR_ONGOING;
}

static int pgn__parse_round(String_View value) {
    if (isdigit(value.data[0])) {
        return atoi(value.data);
    }
    return UNKNOWN_ROUND;
}

static void pgn__parse(PGN *pgn, const PGN_TokenList tl) {
    int i;
    // Attempts to find the first instance of a key-value pair
    //      ASSUMPTION: the first instance of a key-value pair
    //                  is followed by all the rest of the key-value pairs
    for (i = 0; i < tl.count; i++) {
        if (tl.items[i].kind != PGN_TK_TAG_KEY && tl.items[i].kind != PGN_TK_TAG_VALUE)
            break;
    }

    for (int j = 0; j < (i - 1); j += 2) {
        String_View key = tl.items[j].lexeme;
        String_View value = tl.items[j + 1].lexeme;

        if (sv_eq(key, sv_from_cstr("Result"))) {
            pgn->result = pgn__parse_result(value);
        } else if (sv_eq(key, sv_from_cstr("Round"))) {
            pgn->round = pgn__parse_round(value);
        } else if (sv_eq(key, sv_from_cstr("Event"))) {
            pgn->event = value;
        } else if (sv_eq(key, sv_from_cstr("Site"))) {
            pgn->site = value;
        } else if (sv_eq(key, sv_from_cstr("Date"))) {
            pgn->date = value;
        } else if (sv_eq(key, sv_from_cstr("White"))) {
            pgn->white_player = value;
        } else if (sv_eq(key, sv_from_cstr("Black"))) {
            pgn->black_player = value;
        }
    }
}

void pgn_print(PGN pgn) {
    printf("Event: " SV_Fmt "\n", SV_Arg(pgn.event));
    printf(" Site: " SV_Fmt "\n", SV_Arg(pgn.site));
    printf(" Date: " SV_Fmt "\n", SV_Arg(pgn.date));
    if (pgn.round >= 0) {
        printf("Round: %d\n", pgn.round);
    } else {
        printf("Round: unknown\n");
    }
    printf("White: " SV_Fmt "\n", SV_Arg(pgn.white_player));
    printf("Black: " SV_Fmt "\n", SV_Arg(pgn.black_player));
}

bool pgn_read(char *filepath, PGN *pgn, String_Builder *sb) {
    FILE *fptr = fopen(filepath, "r");
    if (fptr == NULL) {
        fprintf(stderr, "[ERROR] Failed to open '%s'.\n", filepath);
        return false;
    }

    if (!nob_read_entire_file(filepath, sb)) return false;

    Nob_String_View sv = nob_sb_to_sv(*sb);
    PGN_TokenList tl = {0};
    pgn__parse_lines(&sv, &tl);

    for (int i = 0; i < tl.count; i++) {
        PGN_Token tok = tl.items[i];
        printf("[%d] (%s) " SV_Fmt "\n", i, token_to_cstr(tok), SV_Arg(tok.lexeme));
    }

    pgn__parse(pgn, tl);

    return true;
}

bool pgn_is_valid(PGN *pgn) {
    UNUSED(pgn);
    TODO("pgn_is_valid()");
}

static_assert(__count_pgn_gr == 4, "There should only be 4 game states.");
static const char *PGN_GR_STRS[] = { "*", "1-0", "0-1", "1/2-1/2" };

typedef enum {
    DA_BY_NONE = 0x0,
    DA_BY_FILE = 0x1,
    DA_BY_RANK = 0x2,
    DA_BY_BOTH = 0x3,
} PGN_Disambiguate;

static PGN_Disambiguate pgn__move_disambiguate_by(Move mv1, Board *board, PieceType pt) {
    MoveList ml = {0};
    movelist_generate_all(&ml, board);

    File f1 = (File)COL(mv1.source);
    Rank r1 = (Rank)ROW(mv1.source);
    Bitboard bb = 0ULL;

    for (int i = 0; i < ml.count; i++) {
        Move mv2 = ml.list[i];
        PieceType target_pt = board_get_piece(board, mv2.source).type;
        if (mv1.target == mv2.target && pt == target_pt) set_bit(bb, mv2.source);
    }

    PGN_Disambiguate pda = DA_BY_NONE;
    if (bb_count(bb & RANK_MASK[r1]) > 1) pda |= DA_BY_FILE;
    if (bb_count(bb & FILE_MASK[f1]) > 1) pda |= DA_BY_RANK;

    return pda;
}

static void pgn__move_to_san(Move move, Board *board, String_Builder *sb) {
    if (move.flag == MVF_Castling) {
        if (COL(move.target) == FILE_G) sb_append_cstr(sb, "O-O");
        else if (COL(move.target) == FILE_C) sb_append_cstr(sb, "O-O-O");
        return;
    }

    PieceType pt = board_get_piece(board, move.source).type;
    switch (pt) {
        case PT_PAWN  : break;
        case PT_KNIGHT: sb_append(sb, 'N'); break;
        case PT_BISHOP: sb_append(sb, 'B'); break;
        case PT_ROOK  : sb_append(sb, 'R'); break;
        case PT_QUEEN : sb_append(sb, 'Q'); break;
        case PT_KING  : sb_append(sb, 'K'); break;
    }

    char file_ltr = str_coords[move.source][0];
    char rank_ltr = str_coords[move.source][1];
    if (pt != PT_PAWN && pt != PT_KING) {
        PGN_Disambiguate pda = pgn__move_disambiguate_by(move, board, pt);
        switch (pda) {
            case DA_BY_NONE: break;
            case DA_BY_FILE: sb_append(sb, file_ltr); break;
            case DA_BY_RANK: sb_append(sb, rank_ltr); break;
            case DA_BY_BOTH: sb_append_cstr(sb, str_coords[move.source]); break;
        }
    }

    if (move.flag == MVF_Capture || move.flag == MVF_Enpassant) {
        if (pt == PT_PAWN) sb_append(sb, file_ltr);
        sb_append(sb, 'x');
    }

    sb_append_cstr(sb, str_coords[move.target]);
}

void pgn_export(Game *game, String_Builder *sb) {
    // Header section
    sb_appendf(sb, "[Event \"michabay05's local tournament\"]\n");
    sb_appendf(sb, "[Site \"michabay05's computer\"]\n");

    time_t current = time(NULL);
    struct tm *curr_tm = localtime(&current);
    sb_appendf(sb, "[Date \"%d.%02d.%02d\"]\n", curr_tm->tm_year + 1900, curr_tm->tm_mon + 1, curr_tm->tm_mday);
    sb_appendf(sb, "[Time \"%02d:%02d:%02d\"]\n", curr_tm->tm_hour, curr_tm->tm_min, curr_tm->tm_sec);
    sb_appendf(sb, "[Round \"??\"]\n");
    if (game->white_name.count > 0) {
        sb_appendf(sb, "[White \""SV_Fmt"\"]\n", SV_Arg(game->white_name));
    } else {
        sb_appendf(sb, "[White \"??\"]\n");
    }
    if (game->black_name.count > 0) {
        sb_appendf(sb, "[Black \""SV_Fmt"\"]\n", SV_Arg(game->black_name));
    } else {
        sb_appendf(sb, "[Black \"??\"]\n");
    }

    sb_appendf(sb, "[Result \"%s\"]\n\n", PGN_GR_STRS[game->state]);

    // Move section
    Board board = {0};
    board_parse_fen_cstr(&board, game->start_fen);
    String_Builder temp = {0};

    for (int i = 0; i < game->history.count; i++) {
        temp.count = 0;

        Move move = game->history.items[i];
        pgn__move_to_san(move, &board, &temp);
        sb_append_null(&temp);

        if (!move_make(&board, move, AllMoves)) {
            UNREACHABLE("Illegal move found in game record\n");
        }

        if (i % 2 == 0) {
            // White's move
            sb_appendf(sb, "%d.%s", (i / 2) + 1, temp.items);
        } else {
            // Black's move
            sb_appendf(sb, " %s ", temp.items);
        }
    }

    sb_appendf(sb, " %s\n", PGN_GR_STRS[game->state]);
    sb_free(temp);
}

// is not end-of-line
// static int is_not_eol(int c) { return c != '\r' && c != '\n'; }

static char peek_ahead(String_View sv, size_t ahead) {
    if (sv.count == 0)
        return '\0';

    return sv.data[ahead];
}

static String_View consume_ahead(String_View *sv, int n) {
    if (sv->count == 0)
        return (String_View){0};
    return sv_chop_left(sv, n);
}

static int peek_while(String_View sv, int (*filter_func)(int c)) {
    int len = 0;
    while (filter_func(peek_ahead(sv, len))) {
        // consume(sv);
        len++;
    }

    return len;
}

static void consume_while(String_View *out, String_View *sv, int (*filter_func)(int c)) {
    int len = 0;
    while (filter_func(peek_ahead(*sv, len))) {
        // consume(sv);
        len++;
    }

    String_View temp = sv_chop_left(sv, len);
    if (out != NULL)
        *out = temp;
}
