#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static void usage(const char *a) {
    fprintf(stderr, "Usage: %s <command> [args...]\n", a);
    exit(1);
}

static double elapsed_seconds(const struct timespec *a, const struct timespec *b) {
    long sec = b->tv_sec - a->tv_sec;
    long nsec = b->tv_nsec - a->tv_nsec;
    return (double)sec + (double)nsec / 1e9;
}

int main(int argc, char **argv) {
    if (argc < 2) usage(argv[0]);

    struct timespec t0, t1;
    if (clock_gettime(CLOCK_MONOTONIC, &t0) != 0) {
        perror("clock_gettime");
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        execvp(argv[1], &argv[1]);
        fprintf(stderr, "Error: execvp failed: %s\n", strerror(errno));
        _exit(127);
    }

    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        perror("waitpid");
        return 1;
    }

    if (clock_gettime(CLOCK_MONOTONIC, &t1) != 0) {
        perror("clock_gettime");
        return 1;
    }

    int exit_code = 0;
    if (WIFEXITED(status)) {
        exit_code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        exit_code = 128 + WTERMSIG(status);
    } else {
        exit_code = 1;
    }

    double elapsed = elapsed_seconds(&t0, &t1);

   
    printf("pid=%ld elapsed=%.3f exit=%d\n", (long)pid, elapsed, exit_code);

    return 0;
}
