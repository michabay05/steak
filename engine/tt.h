#ifndef _TT_H_
#define _TT_H_

#include "defs.h"
#include "zobrist.h"

#define MB 0x100000
#define TT_SIZE 0x400000
// NOTE: This was set to 100,000 just to make sure that this value would not be confused with an
// actual score value. Because the score value is between plus or minus 50,000, this makes it
// evident that this value cannot be a score value.
#define TT_NO_ENTRY 100000

typedef ENUM_DEF(u8, TT_HashFlag) {
    THF_EXACT,
    THF_ALPHA,
    THF_BETA,
};

typedef struct {
    u64 key;
    // Current search depth
    int depth;
    TT_HashFlag flag;
    int score;
} TT_Entry;

void tt_clear(void);
int tt_read(ZB_Key key, int alpha, int beta, int depth);
void tt_write(ZB_Key key, int score, int depth, TT_HashFlag flag);

#endif // _TT_H_
