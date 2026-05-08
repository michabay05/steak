int is_space(int c) { return isspace(c); }
// is alphabetic or numeric
int is_alnum(int c) { return isalnum(c); }
// is not end-of-line
int is_not_eol(int c) { return c != '\r' && c != '\n'; }

#define is_at_end(size, ind) (ind) >= (size)
char peek_ahead(String_View sv, size_t ahead) {
    if (sv.count == 0) return '\0';

    return sv.data[ahead];
}
#define peek(sv) peek_ahead(sv, 0)

String_View consume_ahead(String_View *sv, int n) {
    if (sv->count == 0) return (String_View){0};
    return sv_chop_left(sv, n);
}
#define consume(sv) consume_ahead(sv, 1)

int peek_while(String_View sv, int (*filter_func)(char c)) {
    int len = 0;
    while (filter_func(peek_ahead(sv, len))) {
        // consume(sv);
        len++;
    }

    return len;
}

void consume_while(String_View *out, String_View *sv,
    int (*filter_func)(int c)
) {
    int len = 0;
    while (filter_func(peek_ahead(*sv, len))) {
        // consume(sv);
        len++;
    }

    String_View temp = sv_chop_left(sv, len);
    if (out != NULL) *out = temp;
}

