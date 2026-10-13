#pragma once
/*
 * Crash isolation shared by bench/ and bench-kem/: each scheme .so is loaded and
 * benchmarked in its own forked child, so a scheme that crashes (SIGABRT,
 * SIGSEGV, SIGFPE, ...), hangs or fails its warm-up check costs only its own
 * row, which becomes a "DNF" row, and the run continues with the next scheme.
 *
 * The child reports to the parent over a pipe, one message per line:
 *   N <iters> <name>   the scheme was loaded and matches the filters
 *   R <row>            a complete results row
 *   D <reason>         the scheme did not finish (e.g. failed its warm-up check)
 * The parent prints the row only if the child exited cleanly after sending it, so
 * a crash can never leave a partial or garbled row. Inside the child stdout is
 * redirected to stderr: what a scheme prints itself never lands between rows.
 *
 * BENCH_TIMEOUT (seconds, default 3600, 0 = none) bounds each scheme's wall-clock
 * time; on expiry the child is killed with SIGKILL and the reason is "timeout".
 *
 * Fork happens before dlopen, so the scheme's own constructors run in the child
 * too. CPU affinity is inherited, and the cycle counter is opened per scheme
 * inside the child's worker thread (cycles_setup() in bench_run()).
 */
#include <errno.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define BENCH_TIMEOUT_DEFAULT 3600

enum { BENCH_SKIPPED, BENCH_DONE, BENCH_DNF };

/* Runs in the child: load so_path, and if it matches, send "N", then "R" or "D". */
typedef void (*bench_child_fn)(const char *so_path, void *ctx, FILE *msg);
/* Prints a DNF results row for the named scheme. */
typedef void (*bench_dnf_fn)(const char *name, int iters, const char *reason);

static const char *bench_signame(int sig) {
    switch (sig) {
    case SIGABRT: return "SIGABRT";
    case SIGSEGV: return "SIGSEGV";
    case SIGFPE:  return "SIGFPE";
    case SIGILL:  return "SIGILL";
    case SIGBUS:  return "SIGBUS";
    case SIGTRAP: return "SIGTRAP";
    case SIGSYS:  return "SIGSYS";
    case SIGKILL: return "SIGKILL";
    case SIGTERM: return "SIGTERM";
    case SIGPIPE: return "SIGPIPE";
    default: {
        static char buf[32];
        snprintf(buf, sizeof buf, "signal-%d", sig);
        return buf;
    }
    }
}

static int bench_timeout_seconds(void) {
    const char *e = getenv("BENCH_TIMEOUT");
    if (!e || !*e) return BENCH_TIMEOUT_DEFAULT;
    char *end;
    long v = strtol(e, &end, 10);
    if (*end || v < 0 || v > INT_MAX / 1000) {
        fprintf(stderr, "BENCH_TIMEOUT=%s is not a number of seconds; using %d\n",
                e, BENCH_TIMEOUT_DEFAULT);
        return BENCH_TIMEOUT_DEFAULT;
    }
    return (int)v;
}

static double bench_now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

/* Run one scheme in a child. Returns BENCH_SKIPPED (not loaded / filtered out),
 * BENCH_DONE (a results row was printed) or BENCH_DNF (a DNF row was printed). */
static int bench_isolated(const char *so_path, bench_child_fn child, void *ctx,
                          bench_dnf_fn print_dnf) {
    fflush(stdout);
    fflush(stderr);
    int fds[2];
    if (pipe(fds) != 0) {
        perror("bench: pipe");
        return BENCH_SKIPPED;
    }
    pid_t pid = fork();
    if (pid < 0) {
        perror("bench: fork");
        close(fds[0]);
        close(fds[1]);
        return BENCH_SKIPPED;
    }
    if (pid == 0) {
        close(fds[0]);
        dup2(STDERR_FILENO, STDOUT_FILENO);
        FILE *msg = fdopen(fds[1], "w");
        if (msg) {
            child(so_path, ctx, msg);
            fclose(msg);
        }
        fflush(stdout);
        fflush(stderr);
        _exit(0);  /* no atexit handlers or library destructors of the scheme */
    }
    close(fds[1]);

    char buf[16384];
    size_t len = 0;
    int timeout = bench_timeout_seconds(), timed_out = 0;
    double deadline = bench_now() + timeout;
    for (;;) {
        int wait_ms = -1;
        if (timeout > 0) {
            double left = deadline - bench_now();
            if (left <= 0) { timed_out = 1; break; }
            wait_ms = left * 1000 > INT_MAX - 1 ? INT_MAX : (int)(left * 1000) + 1;
        }
        struct pollfd p = {fds[0], POLLIN, 0};
        int r = poll(&p, 1, wait_ms);
        if (r < 0 && errno == EINTR) continue;
        if (r < 0) { perror("bench: poll"); break; }
        if (r == 0) continue;
        char tmp[4096];
        ssize_t n = read(fds[0], tmp, sizeof tmp);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) break;  /* EOF: the child is exiting */
        size_t take = (size_t)n < sizeof buf - 1 - len ? (size_t)n : sizeof buf - 1 - len;
        memcpy(buf + len, tmp, take);
        len += take;
    }
    close(fds[0]);
    buf[len] = '\0';
    if (timed_out) kill(pid, SIGKILL);
    int status = 0;
    while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {}

    char name[256] = "", reason[64] = "";
    const char *row = NULL;
    int iters = 0;
    for (char *line = buf, *next; line && *line; line = next) {
        next = strchr(line, '\n');
        if (!next) break;  /* incomplete trailing line: ignore */
        *next++ = '\0';
        if (line[0] == 'N' && line[1] == ' ') {
            char *sp = strchr(line + 2, ' ');
            if (sp) {
                iters = atoi(line + 2);
                snprintf(name, sizeof name, "%s", sp + 1);
            }
        } else if (line[0] == 'R' && line[1] == ' ') {
            row = line + 2;
        } else if (line[0] == 'D' && line[1] == ' ') {
            snprintf(reason, sizeof reason, "%s", line + 2);
        }
    }

    int clean = !timed_out && WIFEXITED(status) && WEXITSTATUS(status) == 0;
    if (!reason[0]) {
        if (timed_out)
            snprintf(reason, sizeof reason, "timeout");
        else if (WIFSIGNALED(status) && WTERMSIG(status) == SIGALRM) {
            /* A single operation exceeded BENCH_OP_LIMIT (bench/harness.h). */
            const char *lim = getenv("BENCH_OP_LIMIT");
            snprintf(reason, sizeof reason, "sign-or-verify-over-%ss", lim && *lim ? lim : "600");
        } else if (WIFSIGNALED(status))
            snprintf(reason, sizeof reason, "%s", bench_signame(WTERMSIG(status)));
        else if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
            snprintf(reason, sizeof reason, "exit-%d", WEXITSTATUS(status));
        else if (!row)
            snprintf(reason, sizeof reason, "no-result");
    }

    if (!name[0]) {
        /* Crashed before naming itself (e.g. in a constructor): no row possible. */
        if (!clean)
            fprintf(stderr, "bench: %s: %s while loading\n", so_path, reason);
        return BENCH_SKIPPED;
    }
    if (clean && row && !reason[0]) {
        printf("%s\n", row);
        fflush(stdout);
        return BENCH_DONE;
    }
    print_dnf(name, iters, reason);
    return BENCH_DNF;
}
