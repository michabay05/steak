#include "defs.h"

static u32 randomState = 1804289383;
static inline u32 random_u32(void) {
    u32 number = randomState;

    // XOR shift algorithm
    number ^= number << 13;
    number ^= number >> 17;
    number ^= number << 5;

    // Update random number state
    randomState = number;

    // Return random number
    return number;
}

static inline u64 random_u64(void) {
    u64 rand1, rand2, rand3, rand4;
    rand1 = (u64)(random_u32() & 0xFFFF);
    rand2 = (u64)(random_u32() & 0xFFFF);
    rand3 = (u64)(random_u32() & 0xFFFF);
    rand4 = (u64)(random_u32() & 0xFFFF);
    return rand1 | (rand2 << 16) | (rand3 << 32) | (rand4 << 48);
}

// Used to generate sparse, random 64-bit numbers
// -> Sparse: the number of 1 (on-bits) are minimal
static inline u64 pseudo_random_magic(void) {
    return random_u64() & random_u64() & random_u64();
}

