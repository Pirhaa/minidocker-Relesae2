#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "cgroup.h"
#include "monitor.h"

#define CG_ROOT "/sys/fs/cgroup"

// Lab 5: open, write, close. Returns 0 on success, -1 on failure.
static int write_file(const char *path, const char *value, int quiet) {
    int fd = open(path, O_WRONLY);
    if (fd < 0) {
        if (!quiet) perror(path);
        return -1;
    }
    ssize_t n = write(fd, value, strlen(value));
    close(fd);
    if (n < 0) {
        if (!quiet) perror(path);
        return -1;
    }
    return 0;
}

// Lab 5: open, read, close. Strips the trailing newline.
static int read_file(const char *path, char *buf, size_t size) {
    int fd = open(path, O_RDONLY);
    if (fd < 0)
        return -1;
    ssize_t n = read(fd, buf, size - 1);
    close(fd);
    if (n < 0)
        return -1;
    buf[n] = '\0';
    if (n > 0 && buf[n - 1] == '\n')
        buf[n - 1] = '\0';
    return 0;
}

static void cg_dir(int pid, char *out, size_t size) {
    snprintf(out, size, CG_ROOT "/minidocker/p%d", pid);
}

// 1. LIMIT LAGANE WALA FUNCTION (works for a container or any running PID)
int limit_pid(int pid, const char *mem, const char *pids) {
    char dir[256], path[300], pid_str[16];

    cg_dir(pid, dir, sizeof(dir));

    mkdir(CG_ROOT "/minidocker", 0755);     // already exists is fine
    if (mkdir(dir, 0755) != 0 && errno != EEXIST) {
        perror(dir);
        return -1;
    }

    // Controllers enable karo (parent ko batana parta hai, tab child mein files aati hain)
    write_file(CG_ROOT "/cgroup.subtree_control", "+memory +pids", 1);
    write_file(CG_ROOT "/minidocker/cgroup.subtree_control", "+memory +pids", 1);

    snprintf(path, sizeof(path), "%s/memory.max", dir);
    if (write_file(path, mem, 0) != 0)
        return -1;

    snprintf(path, sizeof(path), "%s/memory.swap.max", dir);   // best effort
    write_file(path, "0", 1);

    snprintf(path, sizeof(path), "%s/pids.max", dir);
    if (write_file(path, pids, 0) != 0)
        return -1;

    // Process ko cgroup mein daalo (limits yahan se lagu hoti hain)
    snprintf(path, sizeof(path), "%s/cgroup.procs", dir);
    snprintf(pid_str, sizeof(pid_str), "%d", pid);
    if (write_file(path, pid_str, 0) != 0)
        return -1;

    return 0;
}

// 2. STATUS DIKHANE WALA FUNCTION
void show_status(int pid) {
    char dir[256], path[300], buf[512];
    long oom_kills = 0;
    char *p;

    cg_dir(pid, dir, sizeof(dir));
    printf("\n--- Cgroup Status (PID %d) ---\n", pid);

    if (access(dir, F_OK) != 0) {
        printf("No limits applied to PID %d (no cgroup found)\n", pid);
        return;
    }

    snprintf(path, sizeof(path), "%s/memory.current", dir);
    if (read_file(path, buf, sizeof(buf)) == 0)
        printf("Memory Used : %ld KB\n", atol(buf) / 1024);

    snprintf(path, sizeof(path), "%s/memory.max", dir);
    if (read_file(path, buf, sizeof(buf)) == 0)
        printf("Memory Limit: %s\n", buf);

    snprintf(path, sizeof(path), "%s/pids.max", dir);
    if (read_file(path, buf, sizeof(buf)) == 0)
        printf("PIDs Limit  : %s\n", buf);

    // memory.events has a line like "oom_kill 3" -- read the real number
    snprintf(path, sizeof(path), "%s/memory.events", dir);
    if (read_file(path, buf, sizeof(buf)) == 0) {
        p = strstr(buf, "oom_kill ");
        if (p != NULL)
            oom_kills = atol(p + 9);
    }

    if (oom_kills > 0)
        printf("Result: KILLED (memory limit exceeded, %ld process(es) killed by OOM)\n", oom_kills);
    else if (process_alive(pid))
        printf("Result: RUNNING\n");
    else
        printf("Result: EXITED normally\n");
}

// 3. CLEANUP WALA FUNCTION
void cleanup_cgroup(int pid) {
    char dir[256];
    cg_dir(pid, dir, sizeof(dir));
    rmdir(dir);
}
