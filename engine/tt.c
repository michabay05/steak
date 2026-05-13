#include <string.h>

#include "tt.h"

static TT_Entry _TTABLE[TT_SIZE] = {0};

void tt_clear(void) {
    memset(_TTABLE, 0, sizeof(_TTABLE));
}

int tt_read(ZB_Key key, int alpha, int beta, int depth) {
    TT_Entry *entry = &_TTABLE[key % TT_SIZE];
    if (entry->key == key) {
        if (entry->depth >= depth) {
            if (entry->flag == THF_EXACT) return entry->score;
            if (entry->flag == THF_ALPHA && entry->score <= alpha) return alpha;
            if (entry->flag == THF_BETA  && entry->score >= beta) return beta;
        }
    }

    return TT_NO_ENTRY;
}

void tt_write(ZB_Key key, int score, int depth, TT_HashFlag flag) {
    TT_Entry *entry = &_TTABLE[key % TT_SIZE];
    entry->key = key;
    entry->score = score;
    entry->flag = flag;
    entry->depth = depth;
}

