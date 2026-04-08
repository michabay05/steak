#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pgn.h"
#include "utils.h"

#define BUF_SIZE 64 * 1024

typedef enum {
    PGN_TK_TAG_KEY = 1,
    PGN_TK_TAG_VALUE,
    PGN_TK_MOVE,
    PGN_TK_COMMENT,
    PGN_TK_GAME_OUTCOME,
} PGN_TokenKind;

typedef struct
{
    char *lexeme;
    PGN_TokenKind kind;
} PGN_Token;

typedef struct
{
    PGN_Token *items;
    size_t size;
    size_t capacity;
} PGN_TokenList;

static void tl_deinit(PGN_TokenList *tl) {
    for (size_t i = 0; i < tl->size; i++) {
        free(tl->items[i].lexeme);
    }
    free(tl->items);
}

static bool is_move_number(char *buf, size_t size, size_t ind) {
    size_t i = ind;
    char c;
    while ((c = peek(buf, size, i)) != '.') {
        if (!isdigit(c))
            return false;
        consume(buf, size, &i);
    }
    return true;
}

static bool is_valid_move_letter(char c) {
    if (c == '\0')
        return false;

    char *accepted_chars[5] = {
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

static bool is_move_text(char *buf, size_t size, size_t ind) {
    if (buf == NULL)
        return false;
    size_t i = ind;
    char c = peek(buf, size, i);
    while (c != '\0' && !isspace(c)) {
        if (is_valid_move_letter(c))
            return true;
        else
            consume(buf, size, &i);

        c = peek(buf, size, i);
    }
    return false;
}

static void token_append(PGN_TokenList *tl, char *lexeme, PGN_TokenKind kind) {
    PGN_Token t = {NULL, 0};
    if (lexeme)
        update_dyn_string(&t.lexeme, lexeme, strlen(lexeme));
    t.kind = kind;

    list_append(tl, t);
}

#if 0
static void token_print(PGN_Token t)
{
    const char* kind_str;
    switch (t.kind) {
        case PGN_TK_TAG_KEY:
            kind_str = "key";
            break;
        case PGN_TK_TAG_VALUE:
            kind_str = "value";
            break;
        case PGN_TK_MOVE:
            kind_str = "move";
            break;
        case PGN_TK_COMMENT:
            kind_str = "comment";
            break;
        case PGN_TK_GAME_OUTCOME:
            kind_str = "outcome";
            break;
    }
    printf("[TOKEN: %7s] %s\n", kind_str, t.lexeme);
}
#endif

/* ================== UTIL FUNCTIONS ================ */
bool is_period(char c) { return c == '.'; }

bool is_not_quote(char c) { return c != '"'; }
bool is_not_space(char c) { return !isspace(c); }
bool is_not_right_curly(char c) { return c != '}'; }
bool is_not_period(char c) { return c != '.'; }
/* ================================================== */

static void parse_lines(char *text, PGN_TokenList *tl) {
    if (text == NULL)
        return;
    const size_t size = strlen(text);

    size_t i = 0;
    char *temp = NULL;
    while (i < size) {
        char c = peek(text, size, i);
        if (c == '[') {
            // [KEY "VALUE"]
            // PGN meta data
            consume(text, size, &i); // Consume [

            consume_while(&temp, text, size, &i, &is_not_space);
            token_append(tl, temp, PGN_TK_TAG_KEY);

            consume(text, size, &i); // Consume ' '
            consume(text, size, &i); // Consume '"'

            consume_while(&temp, text, size, &i, &is_not_quote);
            token_append(tl, temp, PGN_TK_TAG_VALUE);

            consume(text, size, &i); // Consume '"'
            consume(text, size, &i); // Consume the closing square bracket mark
        } else if (c == '{') {
            consume(text, size, &i); // Consume '{'

            consume_while(&temp, text, size, &i, &is_not_right_curly);
            token_append(tl, temp, PGN_TK_COMMENT);

            consume(text, size, &i); // Consume '}'
        } else if (is_move_number(text, size, i)) {
            consume_while(NULL, text, size, &i, &is_not_period);
            consume_while(NULL, text, size, &i, &is_period);
        } else if (is_move_text(text, size, i)) {
            consume_while(&temp, text, size, &i, &is_not_space);
            token_append(tl, temp, PGN_TK_MOVE);
        } else if (isspace(c)) {
            consume(text, size, &i);
        }
    }

    if (temp)
        free(temp);
}

static PGN_GameResult pgn_parse_result(char *lexeme) {
    if (!strncmp(lexeme, "1-0", 3))
        return PGN_GR_WHITE_WINS;
    else if (!strncmp(lexeme, "0-1", 3))
        return PGN_GR_BLACK_WINS;
    else if (!strncmp(lexeme, "1/2-1/2", 7))
        return PGN_GR_DRAW;
    else
        // @NOTE: an asterisk result '*' indicates that a
        // game is still ongoing
        return PGN_GR_ONGOING;
}

static int pgn_parse_round(char *value) {
    if (value != NULL && isdigit(value[0])) {
        return atoi(value);
    }
    return UNKNOWN_ROUND;
}

static void pgn_parse(PGN *pgn, const PGN_TokenList tl) {
    size_t i;
    // Attempts to find the first instance of a key-value pair
    //      ASSUMPTION: the first instance of a key-value pair
    //                  is followed by all the rest of the key-value pairs
    for (i = 0; i < tl.size; i++) {
        if (tl.items[i].kind != PGN_TK_TAG_KEY && tl.items[i].kind != PGN_TK_TAG_VALUE)
            break;
    }

    char **dest;
    for (size_t j = 0; j < (i - 1); j += 2) {
        dest = NULL;
        char *key = tl.items[j].lexeme;
        char *value = tl.items[j + 1].lexeme;
        if (!key)
            continue;
        if (!strncmp(key, "Result", 6)) {
            pgn->result = pgn_parse_result(value);
        } else if (!strncmp(key, "Round", 5)) {
            pgn->round = pgn_parse_round(value);
        } else if (!strncmp(key, "Event", 5)) {
            dest = &pgn->event;
        } else if (!strncmp(key, "Site", 4)) {
            dest = &pgn->site;
        } else if (!strncmp(key, "Date", 4)) {
            dest = &pgn->date;
        } else if (!strncmp(key, "White", 5)) {
            dest = &pgn->white_player;
        } else if (!strncmp(key, "Black", 5)) {
            dest = &pgn->black_player;
        }

        if (dest) {
            update_dyn_string(dest, value, strlen(value));
        }
    }
}

void pgn_print(const PGN *const pgn) {
    printf("Event: %s\n", pgn->event);
    printf(" Site: %s\n", pgn->site);
    printf(" Date: %s\n", pgn->date);
    printf("Round: %d\n", pgn->round);
    printf("White: %s\n", pgn->white_player);
    printf("Black: %s\n", pgn->black_player);
}

bool read_pgn(char *filepath, PGN *pgn) {
    FILE *fptr = fopen(filepath, "r");
    if (fptr == NULL) {
        fprintf(stderr, "[ERROR] Failed to open '%s'.\n", filepath);
        return false;
    }

    char *entire_text = NULL;

    fseek(fptr, 0, SEEK_END);
    long fsize = ftell(fptr);
    rewind(fptr);

    entire_text = (char *)calloc(fsize + 1, sizeof(char));
    fread(entire_text, fsize, 1, fptr);
    fclose(fptr);
    entire_text[fsize] = 0;

    PGN_TokenList tl = {0};
    parse_lines(entire_text, &tl);
    free(entire_text);

    tl.items[tl.size - 1].kind = PGN_TK_GAME_OUTCOME;
    pgn_parse(pgn, tl);
    tl_deinit(&tl);

    return true;
}

void pgn_deinit(PGN *pgn) {
    free(pgn->event);
    free(pgn->site);
    free(pgn->date);
    free(pgn->white_player);
    free(pgn->black_player);
}

int pgn_main(void) {
    PGN pgn = {0};
    if (!read_pgn("examples/without_comments.pgn", &pgn)) {
        return 1;
    }
    pgn_print(&pgn);
    pgn_deinit(&pgn);

    return 0;
}
