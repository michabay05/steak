#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../nob.h"
#include "../chess/chess.h"
#include "util.c"

#define BUF_SIZE 64 * 1024
#define UNKNOWN_ROUND -1

typedef enum {
    PGN_GR_WHITE_WINS,
    PGN_GR_BLACK_WINS,
    PGN_GR_DRAW,
    PGN_GR_ONGOING,
} PGN_GameResult;

typedef struct {
    String_Builder sb;
    String_View event;
    String_View site;
    String_View date;
    int round;
    String_View white_player;
    String_View black_player;
    PGN_GameResult result;
} PGN;

typedef enum {
    PGN_TK_TAG_KEY = 1,
    PGN_TK_TAG_VALUE,
    PGN_TK_MOVE,
    PGN_TK_COMMENT,
    PGN_TK_GAME_OUTCOME,
} PGN_TokenKind;

typedef struct
{
    String_View lexeme;
    PGN_TokenKind kind;
} PGN_Token;

typedef struct
{
    PGN_Token *items;
    int count;
    int capacity;
} PGN_TokenList;

static bool is_move_number(String_View *sv) {
    char c;
    while ((c = peek(*sv)) != '.') {
        if (!isdigit(c)) return false;
        consume(sv);
    }
    return true;
}

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
    if (sv == NULL) return false;

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

static void token_append(
    PGN_TokenList *tl, String_View sv, PGN_TokenKind kind
) {
    PGN_Token t = {
        .lexeme = sv,
        .kind = kind
    };

    da_append(tl, t);
}

static const char *token_to_cstr(PGN_Token t) {
    switch (t.kind) {
        case PGN_TK_TAG_KEY: return "key";
        case PGN_TK_TAG_VALUE: return "value";
        case PGN_TK_MOVE: return "move";
        case PGN_TK_COMMENT: return "comment";
        case PGN_TK_GAME_OUTCOME: return "outcome";
        default: UNREACHABLE("Unknown kind of token");
    }
}

/* ================== UTIL FUNCTIONS ================ */
int is_period(char c) { return c == '.'; }
int is_not_quote(char c) { return c != '"'; }
int is_not_space(char c) { return !isspace(c); }
int is_not_right_curly(char c) { return c != '}'; }
int is_not_period(char c) { return c != '.'; }
int is_digit(char c) { return isdigit(c); }
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
    printf("Event: "SV_Fmt"\n", SV_Arg(pgn.event));
    printf(" Site: "SV_Fmt"\n", SV_Arg(pgn.site));
    printf(" Date: "SV_Fmt"\n", SV_Arg(pgn.date));
    if (pgn.round >= 0) {
        printf("Round: %d\n", pgn.round);
    } else {
        printf("Round: unknown\n");
    }
    printf("White: "SV_Fmt"\n", SV_Arg(pgn.white_player));
    printf("Black: "SV_Fmt"\n", SV_Arg(pgn.black_player));
}

bool pgn_read(char *filepath, PGN *pgn) {
    FILE *fptr = fopen(filepath, "r");
    if (fptr == NULL) {
        fprintf(stderr, "[ERROR] Failed to open '%s'.\n", filepath);
        return false;
    }

    pgn->sb = (String_Builder){0};
    if (!nob_read_entire_file(filepath, &pgn->sb)) return false;
    Nob_String_View sv = nob_sb_to_sv(pgn->sb);

    PGN_TokenList tl = {0};
    pgn__parse_lines(&sv, &tl);

    for (int i = 0; i < tl.count; i++) {
        PGN_Token tok = tl.items[i];
        printf("[%d] (%s) "SV_Fmt"\n",
            i, token_to_cstr(tok), SV_Arg(tok.lexeme));
    }

    pgn__parse(pgn, tl);

    return true;
}

void pgn_deinit(PGN *pgn) {
    sb_free(pgn->sb);
}
