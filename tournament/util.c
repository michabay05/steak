#pragma once

#define is_at_end(size, ind) (ind) >= (size)
char peek_ahead(String_View sv, size_t ahead) {
    if (sv.count == 0) return '\0';

    return sv.data[ahead];
}
#define peek(sv) peek_ahead(sv, 0)

char consume(String_View *sv) {
    if (sv->count == 0) return '\0';

    char output = sv->data[0];
    String_View temp = sv_chop_left(sv, 1);
    return temp.data[0];
}

void consume_while(String_View *out, String_View *sv,
    bool (*filter_func)(char c)
) {
    int len = 0;
    while (filter_func(peek_ahead(*sv, len))) {
        // consume(sv);
        len++;
    }

    String_View temp = sv_chop_left(sv, len);
    if (out != NULL) *out = temp;
}

