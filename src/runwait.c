#define _POSIX_C_SOURCE 200809L

#include "common.h"
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

static void usage(const char *a) {
    fprintf(stderr, "Usage: %s <cmd> [args]\n", a);
    exit(1);
}

static double d(struct timespec a, struct timespec b) {
    return (b.tv_sec - a.tv_sec) + (b.tv_nsec - a.tv_nsec) / 1e9;
}

int main(int c, char **v) {
    if (c < 2) usage(v[0]);

    struct timespec t0, t1;
    if (clock_gettime(CLOCK_MONOTONIC, &t0) != 0) {
        fprintf(stderr, "clock_gettime: %s\n", strerror(errno));
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "fork: %s\n", strerror(errno));
        return 1;
    }

    if (pid == 0) {
        
        execvp(v[1], &v[1]);

        
        fprintf(stderr, "execvp('%s'): %s\n", v[1], strerror(errno));
        _exit(127);
    }


    int status = 0;
    pid_t w = waitpid(pid, &status, 0);
    if (w < 0) {
        fprintf(stderr, "waitpid: %s\n", strerror(errno));
        return 1;
    }

    if (clock_gettime(CLOCK_MONOTONIC, &t1) != 0) {
        fprintf(stderr, "clock_gettime: %s\n", strerror(errno));
        return 1;
    }

    double elapsed = d(t0, t1);

    
    printf("Child PID: %d\n", pid);

    if (WIFEXITED(status)) {
        printf("Exit code: %d\n", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("Terminated by signal: %d\n", WTERMSIG(status));
#ifdef WCOREDUMP
        if (WCOREDUMP(status)) printf("(core dumped)\n");
        else printf("\n");
#else
        printf("\n");
#endif
    } else {
        
        printf("Child ended with unknown status: 0x%x\n", status);
    }

    printf("Elapsed time: %.6f seconds\n", elapsed);
    return 0;
}

