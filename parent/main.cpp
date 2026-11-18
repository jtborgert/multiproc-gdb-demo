// parent/main.cpp
// Release-Build OHNE Debug-Symbole (-O2 -DNDEBUG, Binary wird gestrippt).
// Spawst Worker-Prozesse und verteilt Jobs über Pipes (Round-Robin).
//
// Dieser Prozess ist absichtlich "undurchsichtig": keine Symbole, optimiert.
// Die Worker-Prozesse sind das eigentliche Debug-Ziel.

#include <sys/wait.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace {

struct Worker {
    pid_t pid = -1;
    int to_worker = -1;    // Parent -> Worker (stdin des Workers)
    int from_worker = -1;  // Worker -> Parent (stdout des Workers)
};

[[noreturn]] void die(const char* msg) {
    perror(msg);
    std::exit(1);
}

std::string worker_path() {
    // 1) Expliziter Override per Umgebungsvariable.
    if (const char* p = std::getenv("WORKER_BIN"); p && *p) return p;
    // 2) Default: Worker-Binary neben dem eigenen Binary suchen
    //    (parentd liegt im Build-Ordner, worker auch).
    char buf[4096];
    const ssize_t len = ::readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len > 0) {
        buf[len] = '\0';
        const std::string self(buf);
        const std::size_t slash = self.find_last_of('/');
        if (slash != std::string::npos) return self.substr(0, slash + 1) + "worker";
    }
    return "./worker";
}

Worker spawn_worker(const std::string& path) {
    int in_pipe[2];   // Parent -> Worker
    int out_pipe[2];  // Worker -> Parent
    if (pipe(in_pipe) == -1) die("pipe");
    if (pipe(out_pipe) == -1) die("pipe");

    pid_t pid = fork();
    if (pid == -1) die("fork");

    if (pid == 0) {
        // Child: Pipes auf stdin/stdout umleiten, dann Worker-Binary laden.
        if (dup2(in_pipe[0], STDIN_FILENO) == -1) die("dup2");
        if (dup2(out_pipe[1], STDOUT_FILENO) == -1) die("dup2");
        ::close(in_pipe[0]);
        ::close(in_pipe[1]);
        ::close(out_pipe[0]);
        ::close(out_pipe[1]);
        execl(path.c_str(), "worker", static_cast<char*>(nullptr));
        die("execl");  // wird nur erreicht, wenn exec scheitert
    }

    ::close(in_pipe[0]);
    ::close(out_pipe[1]);
    return Worker{pid, in_pipe[1], out_pipe[0]};
}

}  // namespace

int main(int argc, char** argv) {
    const int num_workers = (argc > 1) ? std::atoi(argv[1]) : 2;

    // "123456789" ist der kritische Job, der den Bug im Worker auslöst.
    const std::vector<long> jobs = {11, 42, 7, 123456789, 5,
                                    99, 3,  8, 123456789, 17};

    std::vector<Worker> workers;
    for (int i = 0; i < num_workers; ++i) {
        workers.push_back(spawn_worker(worker_path()));
    }

    std::fprintf(stderr, "[parent] %d Worker gestartet (PIDs:", num_workers);
    for (const auto& w : workers) {
        std::fprintf(stderr, " %ld", static_cast<long>(w.pid));
    }
    std::fprintf(stderr, ")\n");

    // Jobs verteilen (Round-Robin).
    for (std::size_t i = 0; i < jobs.size(); ++i) {
        Worker& w = workers[i % workers.size()];
        if (dprintf(w.to_worker, "%ld\n", jobs[i]) < 0) die("dprintf");
    }

    // Schreib-Enden schliessen -> Worker sehen EOF und beenden sich.
    for (auto& w : workers) ::close(w.to_worker);

    // Ergebnisse einsammeln und Worker-Status auswerten.
    int crashes = 0;
    for (auto& w : workers) {
        char buf[256];
        ssize_t n;
        while ((n = ::read(w.from_worker, buf, sizeof(buf) - 1)) > 0) {
            buf[n] = '\0';
            std::fputs(buf, stdout);
            std::fflush(stdout);
        }
        ::close(w.from_worker);

        int status = 0;
        waitpid(w.pid, &status, 0);
        if (WIFEXITED(status)) {
            std::fprintf(stderr, "[parent] Worker %ld: normal beendet (Exit %d)\n",
                          static_cast<long>(w.pid), WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            ++crashes;
            std::fprintf(stderr, "[parent] Worker %ld: von Signal %d beendet (Crash!)\n",
                         static_cast<long>(w.pid), WTERMSIG(status));
        }
    }

    std::fprintf(stderr, "[parent] fertig, %d Crash(es)\n", crashes);
    return (crashes == 0) ? 0 : 2;
}
