#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include "container.h"
#include "image.h"
#include "cgroup.h"
#include "overlay.h"

#define STACK_SIZE (1024 * 1024)

struct child_args {
    const char *rootfs;
    char *const *cmd;
    int pipefd[2];
    char id[32];           // unique per container, e.g. c4821
    char merged_dir[256];  // Overlay ke liye
};

static int child_main(void *arg) {
    struct child_args *a = (struct child_args *)arg;
    char c;

    // 1. Pipe synchronization (Lab 8): parent ke "go" tak ruko
    close(a->pipefd[1]);
    if (read(a->pipefd[0], &c, 1) != 1)
        return 1;
    close(a->pipefd[0]);

    // 2. Hostname set karo (UTS namespace)
    sethostname("minidocker", 10);

    // 3. Mount private (Mount namespace)
    mount(NULL, "/", NULL, MS_PRIVATE | MS_REC, NULL);

    // 4. Overlay Setup Call
    if (setup_overlay(a->rootfs, a->id, a->merged_dir) != 0)
        return 1;

    // 5. Chroot merged_dir par
    if (chroot(a->merged_dir) != 0) {
        perror("chroot");
        return 1;
    }
    if (chdir("/") != 0) {
        perror("chdir");
        return 1;
    }

    // 6. /proc mount karo (PID namespace ka /proc)
    if (mount("proc", "/proc", "proc", 0, NULL) != 0)
        perror("mount /proc");

    // 7. Command chalao
    execv(a->cmd[0], a->cmd);
    perror("execv");
    return 1;
}

int run_container(const char *image, char *const argv[], const char *mem, const char *pids) {
    struct child_args args;
    int status;

    if (image_check(image) != 0) return 1;

    args.rootfs = image;
    args.cmd = argv;
    snprintf(args.id, sizeof(args.id), "c%d", getpid());

    if (pipe(args.pipefd) < 0) {
        perror("pipe");
        return 1;
    }

    char *stack = malloc(STACK_SIZE);
    if (!stack) {
        perror("malloc");
        return 1;
    }

    int flags = CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWUTS | SIGCHLD;
    pid_t pid = clone(child_main, stack + STACK_SIZE, flags, &args);
    if (pid < 0) {
        perror("clone");
        close(args.pipefd[0]);
        close(args.pipefd[1]);
        free(stack);
        return 1;
    }

    close(args.pipefd[0]);   // parent sirf likhta hai (Lab 8 rule)

    printf("Container started, host PID = %d (mem=%s, pids=%s)\n", pid, mem, pids);

    // PEHLE limit lagao, PHIR child ko "go" do, taake child limit ke bagair na chale
    if (limit_pid(pid, mem, pids) != 0)
        printf("Warning: cgroup limits failed\n");

    if (write(args.pipefd[1], "g", 1) != 1)
        perror("write");
    close(args.pipefd[1]);

    waitpid(pid, &status, 0);

    int code = 0;
    if (WIFEXITED(status)) {
        code = WEXITSTATUS(status);
        printf("Container exited with code %d\n", code);
    } else if (WIFSIGNALED(status)) {
        code = 128 + WTERMSIG(status);
        printf("Container killed by signal %d\n", WTERMSIG(status));
    }

    show_status(pid);
    cleanup_cgroup(pid);
    cleanup_overlay(args.id);

    free(stack);
    return code;
}
