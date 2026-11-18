cmake_minimum_required(VERSION 3.16)
project(multiproc_gdb_demo CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Worker: das eigentliche Debug-Ziel.
# Volle Debug-Symbole, keine Optimierung, Frame-Pointer erhalten.
# ---------------------------------------------------------------------------
add_executable(worker worker/worker.cpp)
target_compile_options(worker PRIVATE -g -O0 -fno-omit-frame-pointer -Wall -Wextra)

# ---------------------------------------------------------------------------
# Parent: Release-Mode OHNE Debug-Symbole.
# -O2 + NDEBUG, und das Binary wird beim Linken komplett gestrippt (-Wl,-s).
# ---------------------------------------------------------------------------
add_executable(parentd parent/main.cpp)
target_compile_options(parentd PRIVATE -O2 -DNDEBUG -Wall -Wextra)
# Symboltabelle beim Linken komplett entfernen -> "nm: no symbols"
target_link_options(parentd PRIVATE -Wl,-s)
