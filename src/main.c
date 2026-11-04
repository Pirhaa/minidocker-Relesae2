#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "monitor.h"
#include "container.h"
#include "cgroup.h"

static void usage(void) {
    printf("Usage:\n"
           "  minidocker list                                  processes, biggest memory first\n"
           "  minidocker run [--mem 50M] [--pids 20] <image> [cmd...]\n"
           "  minidocker limit <pid> <mem> <pids>              limit a running process (e.g. 50M 20)\n"
           "  minidocker status <pid>                          RUNNING / EXITED / KILLED (OOM)\n"
           "  minidocker stop <pid>                            SIGTERM, then SIGKILL if needed\n");
}

// "max" or only digits
static int valid_num(const char *s) {
    if (strcmp(s, "max") == 0) return 1;
    if (s[0] == '\0') return 0;
    for (int i = 0; s[i] != '\0'; i++)
        if (!isdigit((unsigned char)s[i])) return 0;
    return 1;
}

// "max", or digits with an optional K/M/G at the end (e.g. 50M)
static int valid_mem(const char *s) {
    char buf[32];
    size_t n = strlen(s);

    if (n == 0 || n >= sizeof(buf)) return 0;
    strcpy(buf, s);
    if (strchr("KMGkmg", buf[n - 1]) != NULL && n > 1)
        buf[n - 1] = '\0';
    return valid_num(buf);
}

static int parse_pid(const char *s) {
    int pid = atoi(s);
    return (valid_num(s) && pid > 0) ? pid : -1;
}

static int need_root(void) {
    if (geteuid() != 0) {
        printf("Error: this command needs root, run it with sudo\n");
        return 1;
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        usage();
        return 1;
    }

    if (strcmp(argv[1], "list") == 0) {
        list_processes();
        return 0;
    }

    if (strcmp(argv[1], "status") == 0 && argc == 3) {
        int pid = parse_pid(argv[2]);
        if (pid < 0) { usage(); return 1; }
        print_status(pid);
        show_status(pid);
        return 0;
    }

    if (strcmp(argv[1], "stop") == 0 && argc == 3) {
        int pid = parse_pid(argv[2]);
        if (pid < 0) { usage(); return 1; }
        return stop_process(pid);
    }

    if (strcmp(argv[1], "limit") == 0 && argc == 5) {
        int pid = parse_pid(argv[2]);
        if (pid < 0 || !valid_mem(argv[3]) || !valid_num(argv[4])) { usage(); return 1; }
        if (need_root()) return 1;
        if (limit_pid(pid, argv[3], argv[4]) != 0) {
            printf("Error: could not apply limits to PID %d\n", pid);
            return 1;
        }
        printf("Limits applied to PID %d: mem=%s pids=%s\n", pid, argv[3], argv[4]);
        return 0;
    }

    if (strcmp(argv[1], "run") == 0) {
        const char *mem = "50M", *pids = "20";
        int i = 2;

        while (i < argc && strncmp(argv[i], "--", 2) == 0) {
            if (strcmp(argv[i], "--mem") == 0 && i + 1 < argc) {
                mem = argv[i + 1];
            } else if (strcmp(argv[i], "--pids") == 0 && i + 1 < argc) {
                pids = argv[i + 1];
            } else {
                usage();
                return 1;
            }
            i += 2;
        }
        if (i >= argc || !valid_mem(mem) || !valid_num(pids)) {
            usage();
            return 1;
        }
        if (need_root()) return 1;

        const char *image = argv[i++];
        char *default_cmd[] = {"/bin/sh", NULL};
        char *const *cmd = (i < argc) ? &argv[i] : default_cmd;
        return run_container(image, cmd, mem, pids);
    }

    usage();
    return 1;
}
