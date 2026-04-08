#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

#define is_at_end(size, ind) (ind) >= (size)

char peek_ahead(const char *buf, size_t size, size_t ind, size_t ahead) {
    if (is_at_end(size, ind))
        return '\0';
    return buf[ind + ahead];
}

char consume(char *buf, size_t size, size_t *ind) {
    (*ind)++;
    if (is_at_end(size, *ind))
        return '\0';
    char output = buf[(*ind) - 1];
    return output;
}

// WARNING: This method allocates memory
void update_dyn_string(char **dest, const char *src, size_t n) {
    *dest = (char *)realloc(*dest, (n + 1) * sizeof(char));
    if (*dest == NULL)
        return;
    memset(*dest, 0, n + 1);
    strncpy(*dest, src, n);
}

void consume_while(char **dest, char *src, size_t size, size_t *i, bool (*filter_func)(char c)) {
    size_t prev_ind = *i;
    size_t word_len = 0;
    while (filter_func(peek(src, size, *i))) {
        consume(src, size, i);
        word_len++;
    }
    if (dest != NULL) {
        update_dyn_string(dest, src + prev_ind, word_len);
    }
}
