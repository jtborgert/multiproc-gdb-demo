#!/usr/bin/env bash
# Startet das Szenario mit aktivem Debug-Gate in allen Workern.
# Terminal 1:  scripts/run.sh
# Terminal 2:  scripts/attach-gdb.sh
# Gate oeffnen: touch /tmp/worker_gate_go
set -euo pipefail
cd "$(dirname "$0")/.."

rm -f /tmp/worker_gate_go
export WORKER_DEBUG_GATE=1
export WORKER_BIN="$PWD/build/worker"
exec ./build/parentd "$@"
