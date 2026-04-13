#include <stdio.h>
#include <unistd.h>

#define NOB_IMPLEMENTATION
#include "comm.c"

#define BUF_SIZE 64*1024

int main(void) {
    Engine engine = {0};
    char *engine_path = "../stockfish-ubuntu-x86-64-avx2";
    if (!load_engine(engine_path, &engine)) {
        fprintf(stderr, "Failed to load engine!\n");
        return 1;
    }

    char buf[BUF_SIZE] = {0};

    send_to_engine(engine, "position startpos\n");
    send_to_engine(engine, "go depth 10\n");
    // memset(buf, 0, BUF_SIZE);
    printf("Started waiting!\n");
    sleep(3);
    printf("Done waiting!\n");

    read_from_engine(engine, buf, BUF_SIZE);
    printf(">>> '%s'\n", buf);

    return 0;
}
