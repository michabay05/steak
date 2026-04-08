#include <assert.h>

#define INIT_CAPACITY 256

#define list_append(da, item)                                                                      \
    do {                                                                                           \
        if ((da)->size >= (da)->capacity) {                                                        \
            (da)->capacity = (da)->capacity == 0 ? INIT_CAPACITY : (da)->capacity * 2;             \
            (da)->items = realloc((da)->items, (da)->capacity * sizeof(*(da)->items));             \
            assert((da)->items != NULL && "Buy more RAM lol");                                     \
        }                                                                                          \
                                                                                                   \
        (da)->items[(da)->size++] = (item);                                                        \
    } while (0)

#define peek(buf, size, ind) peek_ahead(buf, size, ind, 0)
#define peek_next(buf, size, ind) peek_ahead(buf, size, ind, 1)

char peek_ahead(const char *buf, size_t size, size_t ind, size_t ahead);
char consume(char *buf, size_t size, size_t *ind);
// WARNING: The method below allocates memory
void update_dyn_string(char **dest, const char *src, size_t n);
void consume_while(char **dest, char *src, size_t size, size_t *i, bool (*filter_func)(char c));
