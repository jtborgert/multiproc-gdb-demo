# Demo-Projekt: Multi-Process-Debugging mit GDB

Szenario: `parentd` (Release, **ohne** Debug-Symbole, gestrippt) startet
mehrere `worker`-Prozesse (Debug-Build mit `-g -O0`). Gedebuggt werden soll
der Worker-Code. Der Job `123456789` loest einen eingebauten Bug aus
(Stack-Buffer-Overflow in `digit_sum`).

Die vollstaendige Anleitung (drei GDB-Methoden, Troubleshooting,
Cheat Sheet) befindet sich im begleitenden Canvas-Dokument.

## Schnellstart

```bash
./scripts/build.sh                      # bauen + Symbol-Status pruefen

# Terminal 1: Szenario mit Debug-Gate starten (Worker warten auf gdb)
./scripts/run.sh

# Terminal 2: gdb an den juengsten Worker an docken
./scripts/attach-gdb.sh
#   -> in gdb: break digit_sum ; continue

# Gate oeffnen, damit die Worker loslaufen:
touch /tmp/worker_gate_go
```

Hinweis: `parentd` findet `worker` automatisch neben dem eigenen Binary
(via /proc/self/exe). Override bei Bedarf: `WORKER_BIN=/pfad/zu/worker`.

## Alternativen ohne Gate

```bash
# Methode B: gdb steuert den Start (Parent ohne Symbole ist ok!)
gdb -q ./build/parentd
  (gdb) set detach-on-fork off
  (gdb) set follow-fork-mode child
  (gdb) run
  ... Worker crashen -> backtrace mit vollen Symbolen

# Methode C: direkt an laufende Worker an docken
gdb -q ./build/worker -p "$(pgrep -n -x worker)"
```

## Projektstruktur

```
parent/main.cpp     Release-Parent (fork + exec, Pipes, Round-Robin-Jobs)
worker/worker.cpp   Debug-Ziel: Debug-Gate + Bug in digit_sum()
scripts/build.sh    Build + Verifikation (stripped vs. debug_info)
scripts/run.sh      Szenario mit aktivem Debug-Gate starten
scripts/attach-gdb.sh  dockt gdb an den juengsten Worker an
scripts/gdb-attach.cmds  gdb-Kommandos nach dem Attach
```

Hinweis: `123456789` ist absichtlich ein 9-stelliger Wert, `digit_sum()`
nutzt ein Array mit 8 Plaetzen. Je nach Compiler/Stack-Protector crasht der
Worker mit SIGSEGV oder SIGABRT (stack smashing detected).
