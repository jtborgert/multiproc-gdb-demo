// worker/worker.cpp
// Debug-Build MIT Symbolen (-g -O0). Dieser Code ist das eigentliche Debug-Ziel.
//
// Protokoll: liest Zahlen (eine pro Zeile) von stdin und schreibt
// "OK <value> -> Quersumme <sum>" nach stdout.

#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <string>

namespace {

// Debug-Gate: nur aktiv, wenn die Umgebungsvariable WORKER_DEBUG_GATE gesetzt
// ist. Haelt den Worker an, bis die Gate-Datei existiert. Das gibt dir Zeit,
// gdb an den Prozess anzudocken, bevor er auch nur einen Job verarbeitet.
// Gate loesen (in einem anderen Terminal):  touch /tmp/worker_gate_go
void debug_gate() {
    const char* enabled = std::getenv("WORKER_DEBUG_GATE");
    if (enabled == nullptr || *enabled == '\0') return;

    const char* path = std::getenv("WORKER_GATE_FILE");
    const std::string gate_file =
        (path && *path) ? std::string(path) : std::string("/tmp/worker_gate_go");

    std::fprintf(stderr,
                  "[worker] PID=%ld DEBUG-GATE: warte, bis %s existiert "
                  "(loesen mit: touch %s)\n",
                  static_cast<long>(getpid()), gate_file.c_str(),
                  gate_file.c_str());
    while (::access(gate_file.c_str(), F_OK) != 0) {
        sleep(1);
    }
    std::fprintf(stderr, "[worker] PID=%ld: Gate offen, los geht's\n",
                 static_cast<long>(getpid()));
}

// Berechnet die Quersumme von 'value'.
//
// BUG: 'digits' hat 8 Plaetze. Werte mit mehr als 8 Ziffern (z.B. der Job
// 123456789) schreiben ueber das Arrayende hinaus -> Stack-Buffer-Overflow.
// Genau hier sollst du mit GDB auf der Spur sein.
int digit_sum(long value) {
    int digits[8];
    int count = 0;
    long v = value;
    while (v > 0) {
        digits[count] = static_cast<int>(v % 10);  // <-- BUG-Zeile
        v /= 10;
        ++count;
    }
    int sum = 0;
    for (int i = 0; i < count; ++i) sum += digits[i];
    return sum;
}

}  // namespace

int main() {
    debug_gate();

    char line[64];
    while (std::fgets(line, sizeof(line), stdin) != nullptr) {
        const long value = std::atol(line);
        const int sum = digit_sum(value);  // <-- hier passiert der Crash
        std::printf("OK %ld -> Quersumme %d\n", value, sum);
        std::fflush(stdout);
    }
    return 0;
}
