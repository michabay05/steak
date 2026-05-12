#include "./test_precalculate.c"
#include "./test_fen.c"
#include "./test_moves.c"
#include "./test_zobrist.c"

int main(void) {
    attack_init();
    zobrist_init();

    test_precalculate_main();
    test_fen_main();
    test_moves_main();
    test_zobrist_main();

    return 0;
}
