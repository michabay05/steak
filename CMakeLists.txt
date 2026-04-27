cmake_minimum_required(VERSION 3.10)
project(cchess C)

set(CMAKE_C_COMPILER "gcc")

# ==========================================
# >>>> Chess Library
# ==========================================
# Add all .c files belonging to the library here
add_library(libchess STATIC
    chess/bitboard.c
    chess/board.c
    chess/fen.c
    chess/move.c
    chess/move_gen.c
    chess/precalculate.c
)

# Allow executables linking to this library to find its .h files automatically
target_include_directories(libchess PUBLIC library)

# ==========================================
# >>>> Perft
# ==========================================
add_executable(perft chess/perft.c)
target_link_libraries(perft PRIVATE libchess)

# ==========================================
# >>>> Engine
# ==========================================
add_executable(engine engine/uci.c engine/eval.c engine/search.c)

# Include exe1's own headers and link the static library
target_include_directories(engine PRIVATE engine)
target_link_libraries(engine PRIVATE libchess)

