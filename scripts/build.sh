#!/usr/bin/env bash
# Baut das Projekt und zeigt den Symbol-Status beider Binaries.
set -euo pipefail
cd "$(dirname "$0")/.."

cmake -S . -B build
cmake --build build -j

echo
echo "--- parentd: 'file' zeigt 'stripped' ---"
file build/parentd
echo "--- worker: 'file' zeigt 'with debug_info, not stripped' ---"
file build/worker
echo
echo "--- nm build/parentd (erwartet: no symbols) ---"
nm build/parentd || true
echo
echo "--- nm build/worker (Auszug) ---"
nm build/worker | grep -E "digit_sum|debug_gate" || true
