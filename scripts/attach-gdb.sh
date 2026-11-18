#!/usr/bin/env bash
# Dockt gdb an den juengsten laufenden Worker-Prozess an.
# Vorteil von 'gdb ./build/worker -p PID': gdb laedt die Symbole des
# Worker-Binaries, selbst wenn der laufende Prozess von einem fremden,
# gestrippten Parent gestartet wurde.
set -euo pipefail
cd "$(dirname "$0")/.."

PID=$(pgrep -n -x worker)
echo "Attache gdb an Worker-PID ${PID}"
exec gdb -q -x scripts/gdb-attach.cmds ./build/worker -p "${PID}"
