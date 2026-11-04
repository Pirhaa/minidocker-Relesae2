#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <signal.h>
#include <unistd.h>
#include "monitor.h"

#define MAX_PROCS 4096

struct proc_info {
    int  pid;
    long rss_kb;
    char name[64];
};

static int is_number(const char *s) {
    if (s[0] == '\0')
        return 0;
    for (int i = 0; s[i] != '\0'; i++) {
        if (!isdigit((unsigned char)s[i]))
            return 0;
    }
    return 1;
}

// Reads Name and VmRSS from /proc/<pid>/status (kernel threads have no VmRSS -> 0)
static int read_proc(int pid, struct proc_info *p) {
    char path[64], line[256];
    FILE *f;

    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    f = fopen(path, "r");
    if (f == NULL)
        return -1;

    p->pid = pid;
    p->rss_kb = 0;
    strcpy(p->name, "?");

    while (fgets(line, sizeof(line), f) != NULL) {
        if (strncmp(line, "Name:", 5) == 0)
            sscanf(line + 5, " %63[^\n]", p->name);
        else if (strncmp(line, "VmRSS:", 6) == 0)
            sscanf(line + 6, " %ld", &p->rss_kb);
    }
    fclose(f);
    return 0;
}

static int by_memory_desc(const void *a, const void *b) {
    const struct proc_info *x = a, *y = b;
    if (y->rss_kb > x->rss_kb) return 1;
    if (y->rss_kb < x->rss_kb) return -1;
    return 0;
}

void list_processes(void) {
    static struct proc_info procs[MAX_PROCS];
    DIR *dir;
    struct dirent *entry;
    int count = 0;

    dir = opendir("/proc");
    if (dir == NULL) {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dir)) != NULL && count < MAX_PROCS) {
        if (!is_number(entry->d_name))
            continue;
        if (read_proc(atoi(entry->d_name), &procs[count]) == 0)
            count++;
    }
    closedir(dir);

    qsort(procs, count, sizeof(procs[0]), by_memory_desc);

    printf("%-8s %-12s %s\n", "PID", "MEMORY(KB)", "NAME");
    printf("----------------------------------------\n");
    for (int i = 0; i < count; i++)
        printf("%-8d %-12ld %s\n", procs[i].pid, procs[i].rss_kb, procs[i].name);
    printf("----------------------------------------\n");
    printf("Total processes: %d (sorted by memory, biggest first)\n", count);
}

void print_status(int pid) {
    char path[64], line[256];
    FILE *f;

    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    f = fopen(path, "r");
    if (f == NULL) {
        perror(path);
        return;
    }
    while (fgets(line, sizeof(line), f) != NULL) {
        if (strncmp(line, "State:", 6) == 0 || strncmp(line, "VmRSS:", 6) == 0)
            printf("%s", line);
    }
    fclose(f);
}

// A zombie has already finished (Lab 6, Sec 9), so it does not count as alive
int process_alive(int pid) {
    char path[64], line[256];
    int alive = 0;
    FILE *f;

    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    f = fopen(path, "r");
    if (f == NULL)
        return 0;
    while (fgets(line, sizeof(line), f) != NULL) {
        if (strncmp(line, "State:", 6) == 0) {
            alive = (strchr(line, 'Z') == NULL);
            break;
        }
    }
    fclose(f);
    return alive;
}

// Lab 6, Sec 7: plain kill (SIGTERM) first, SIGKILL only as a last resort
int stop_process(int pid) {
    if (pid <= 1 || pid == getpid()) {
        printf("Refusing to stop PID %d\n", pid);
        return 1;
    }
    if (kill(pid, SIGTERM) != 0) {
        perror("kill");
        return 1;
    }
    printf("Sent SIGTERM to %d, waiting...\n", pid);

    for (int i = 0; i < 20; i++) {          // wait up to ~2 seconds
        usleep(100000);
        if (!process_alive(pid)) {
            printf("PID %d stopped.\n", pid);
            return 0;
        }
    }
    printf("PID %d ignored SIGTERM, sending SIGKILL\n", pid);
    kill(pid, SIGKILL);
    return 0;
}
