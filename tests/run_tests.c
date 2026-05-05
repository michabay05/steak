#define NOB_IMPLEMENTATION
#include "../chess/chess_unity.c"

#include "./test_precalculate.c"
#include "./test_fen.c"

typedef struct {
    int succ, fail;
} TestResult;

int main(void) {
    attack_init();

    TestResult tr = {0};
    test_precalculate_main();
    test_fen_main();

    return 0;
}
