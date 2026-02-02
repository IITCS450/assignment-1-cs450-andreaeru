#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>



static void usage(const char *a) {
    fprintf(stderr, "Usage: %s <pid>\n", a);
    exit(1);
}

static int isnum(const char *s) {
    if (!s || !*s) return 0;
    for (; *s; s++) {
        if (!isdigit((unsigned char)*s)) return 0;
    }
    return 1;
}

static void die_open(const char *path) {
    if (errno == ENOENT) {
        fprintf(stderr, "Error: PID not found\n");
    } else if (errno == EACCES) {
        fprintf(stderr, "Error: Permission denied\n");
    } else {
        fprintf(stderr, "Error: cannot open %s: %s\n", path, strerror(errno));
    }
    exit(1);
}

static char *read_file(const char *path, size_t *out_sz) {
    FILE *f = fopen(path, "rb");
    if (!f) die_open(path);

    if (fseek(f, 0, SEEK_END) != 0) {
        fprintf(stderr, "Error: fseek failed on %s\n", path);
        fclose(f);
        exit(1);
    }

    long n = ftell(f);
    if (n < 0) {
        fprintf(stderr, "Error: ftell failed on %s\n", path);
        fclose(f);
        exit(1);
    }
    rewind(f);

    char *buf = malloc((size_t)n + 1);
    if (!buf) {
        fprintf(stderr, "Error: out of memory\n");
        fclose(f);
        exit(1);
    }

    size_t got = fread(buf, 1, (size_t)n, f);
    fclose(f);
    buf[got] = '\0';
    if (out_sz) *out_sz = got;
    return buf;
}



static void parse_stat(const char *path,
                       char *out_state,
                       long *out_ppid,
                       unsigned long long *out_cpu_ticks) {
    char *line = read_file(path, NULL);

    char *rp = strrchr(line, ')');
    if (!rp) {
        *out_state = '?';
        *out_ppid = -1;
        *out_cpu_ticks = 0;
        free(line);
        return;
    }

    char *p = rp + 1;
    while (*p == ' ') p++;

    int field = 3;
    char state = '?';
    long ppid = -1;
    unsigned long long utime = 0, stime = 0;

    char *save = NULL;
    for (char *tok = strtok_r(p, " ", &save);
         tok;
         tok = strtok_r(NULL, " ", &save), field++) {

        if (field == 3) state = tok[0];
        else if (field == 4) ppid = strtol(tok, NULL, 10);
        else if (field == 14) utime = strtoull(tok, NULL, 10);
        else if (field == 15) { stime = strtoull(tok, NULL, 10); break; }
    }

    *out_state = state;
    *out_ppid = ppid;
    *out_cpu_ticks = utime + stime;

    free(line);
}


static long parse_vmrss_kb(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) die_open(path);

    char line[512];
    long vmrss = -1;

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "VmRSS:", 6) == 0) {
            char *p = line + 6;
            while (*p && !isdigit((unsigned char)*p)) p++;
            if (*p) vmrss = strtol(p, NULL, 10);
            break;
        }
    }

    fclose(f);
    return vmrss;
}

static void print_cmdline(const char *path) {
    size_t sz = 0;
    char *buf = read_file(path, &sz);

    if (sz == 0 || buf[0] == '\0') {
        printf("Command line: [empty]\n");
        free(buf);
        return;
    }

    for (size_t i = 0; i < sz; i++) {
        if (buf[i] == '\0') buf[i] = ' ';
    }

    while (sz > 0 && buf[sz - 1] == ' ') {
        buf[--sz] = '\0';
    }

    printf("Command line: %s\n", buf);
    free(buf);
}



int main(int c, char **v) {
    if (c != 2 || !isnum(v[1])) usage(v[0]);

    char stat_path[256], status_path[256], cmdline_path[256];
    snprintf(stat_path, sizeof(stat_path), "/proc/%s/stat", v[1]);
    snprintf(status_path, sizeof(status_path), "/proc/%s/status", v[1]);
    snprintf(cmdline_path, sizeof(cmdline_path), "/proc/%s/cmdline", v[1]);

    char state;
    long ppid;
    unsigned long long cpu_ticks;

    parse_stat(stat_path, &state, &ppid, &cpu_ticks);
    long vmrss_kb = parse_vmrss_kb(status_path);

    long hz = sysconf(_SC_CLK_TCK);
    double cpu_seconds = (hz > 0) ? (double)cpu_ticks / hz : 0.0;

    printf("Process state: %c\n", state);
    printf("Parent PID: %ld\n", ppid);
    print_cmdline(cmdline_path);
    printf("CPU time (user+system): %.2f seconds\n", cpu_seconds);

    if (vmrss_kb >= 0)
        printf("Resident memory (VmRSS): %ld kB\n", vmrss_kb);
    else
        printf("Resident memory (VmRSS): [not found]\n");

    return 0;
}
