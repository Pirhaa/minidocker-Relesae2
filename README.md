# Mini Docker (Lightweight Container Runtime)

A small container runtime for Linux, written in C. It runs a program in its own isolated space with a private hostname, process list and filesystem, and keeps it inside fixed memory, process and CPU limits.

## About the Project

**Mini Docker** is the semester project for the Operating Systems course (CS251) at Sukkur IBA University.

Tools like Docker look complicated from outside, but they are built on a few features of the Linux kernel. This project uses those features directly, so we can see how an operating system keeps one application separate from the rest of the machine.

You start a container with one command. The runtime gives it a separate view of processes and hostname (namespaces), a separate root folder (chroot) with a writable layer on top of a read-only image (OverlayFS), and fixed limits on memory, processes and CPU (cgroups v2). A monitoring command reads `/proc` to list processes and report when one was killed for using too much memory.

## Objectives

- Show how a child process can be started inside new namespaces.
- Hide the host's processes and hostname from the container.
- Give each container its own root folder with chroot.
- Let a container write files without changing the base image (OverlayFS).
- Limit memory, number of processes and CPU time with cgroups v2.
- Watch processes through `/proc` and detect out-of-memory kills.

## Features

- [x] Commands: `run`, `list`, `limit`, `status`, `stop`.
- [x] Container started with Linux system calls.
- [x] Own process list (PID namespace).
- [x] Own hostname (UTS namespace).
- [x] Own mount points (mount namespace).
- [x] Own root folder using chroot (Alpine Linux image).
- [x] Writable layer over a read-only image (OverlayFS).
- [x] Memory limit (cgroups v2).
- [x] Process count limit (cgroups v2).
- [x] CPU limit (cgroups v2).
- [x] Process monitoring from `/proc`.
- [x] OOM kill detection and cleanup when the container exits.
- [ ] Security restrictions using Seccomp / Linux capabilities (future work).

## Technology Stack

| Technology         | Purpose                                      |
| ------------------ | -------------------------------------------- |
| C                  | Main programming language                    |
| Linux / Ubuntu     | Development and execution platform           |
| GCC                | C compiler                                   |
| GNU Make           | Build system                                 |
| Linux Namespaces   | PID, mount and hostname (UTS) isolation      |
| chroot             | Filesystem isolation                         |
| OverlayFS          | Layered storage                              |
| Linux cgroups v2   | Memory, process and CPU limits               |
| Linux system calls | Low-level OS operations                      |
| procfs             | Process monitoring and diagnostics           |
| GDB                | Debugging                                    |
| strace             | System call tracing                          |
| Git & GitHub       | Version control and collaboration            |

## Project Structure

```text
minidocker/
├── docs/
│   └── report.md
├── src/
│   ├── main.c         command line (list, run, limit, status, stop)
│   ├── container.c    namespaces, chroot, starting the process
│   ├── cgroup.c       cgroup limits and status
│   ├── overlay.c      OverlayFS setup and cleanup
│   ├── image.c        image check
│   └── monitor.c      /proc process monitoring
├── images/
│   └── alpine/        Alpine root filesystem (see step 3)
├── Makefile
├── LICENSE
└── README.md
```

## Prerequisites

- Ubuntu or another Linux system (we used Ubuntu in VirtualBox).
- `gcc` and `make` to build the code.
- `git` to clone the repository.
- A kernel with namespaces and OverlayFS (standard on Ubuntu).
- cgroups v2 (the default on Ubuntu 22.04 and newer).
- `sudo` rights, because creating namespaces and cgroups needs root.

Install the tools if needed:

```bash
sudo apt update
sudo apt install gcc make git
```

## Getting Started

### 1. Clone the repository

```bash
git clone https://github.com/Pirhaa/minidocker-Relesae2
cd minidocker-Relesae2
```

### 2. Build the project

```bash
make
```

The build finishes with zero errors and zero warnings. To rebuild from scratch:

```bash
make clean
make
```

### 3. Prepare the Alpine image (one time only)

The runtime needs an Alpine Linux root filesystem in `images/alpine`.

1. Download the **Mini root filesystem** for **x86_64** from https://alpinelinux.org/downloads/ (the file is named like `alpine-minirootfs-<version>-x86_64.tar.gz`).
2. Extract it:

```bash
mkdir -p images/alpine
sudo tar -xzf alpine-minirootfs-*.tar.gz -C images/alpine
ls images/alpine        # should show bin, etc, usr ...
```

### 4. Run a container

Creating namespaces needs root.

```bash
sudo ./minidocker run alpine /bin/sh
```

The prompt changes to `/ #`, which means you are inside the container. Type `exit` to leave. When the container stops, the runtime prints its cgroup status.

```bash
# with resource limits (cgroups v2)
sudo ./minidocker run --mem 100M --pids 10 alpine /bin/sh
sudo ./minidocker run --cpu "50000 100000" alpine /bin/sh
```

### 5. Manage and monitor

```bash
./minidocker list                        # processes, biggest memory first (reads /proc)
sudo ./minidocker limit <PID> 50M 20     # apply memory and process limits from outside
sudo ./minidocker status <PID>           # RUNNING / EXITED / KILLED (OOM)
sudo ./minidocker stop <PID>             # SIGTERM, then SIGKILL if needed
```

`<PID>` is the host PID. The `run` command prints it as `Container started, host PID = ...`, and `list` shows it too.

| Command | Description |
| ------- | ----------- |
| `run [--mem M] [--pids N] [--cpu "Q P"] <image> [cmd]` | Start a container with limits |
| `list` | Show processes sorted by memory usage (from `/proc`) |
| `limit <PID> <mem> <pids>` | Apply memory and process limits to a running process |
| `status <PID>` | Show cgroup usage and limits, and RUNNING / EXITED / KILLED |
| `stop <PID>` | Stop a process with SIGTERM, then SIGKILL if needed |

| Option | Description | Default |
| ------ | ----------- | ------- |
| `--mem` | Memory limit, e.g. `50M` | `50M` |
| `--pids` | Maximum number of processes | `20` |
| `--cpu` | `"quota period"` in microseconds, `"50000 100000"` is 50% of one CPU | no limit |

> **Notes**
> - Always write the memory unit (`50M`, not `50`). A plain `50` means 50 bytes.
> - When starting a program directly, give its full path, for example `/bin/sleep 3`.
> - The container has no `/dev` device files.
> - Run `status` before the container exits, because its cgroup is removed when it stops.
> - Limiting system processes (PID below 100) is blocked on purpose to protect the desktop.

## Demonstration

These are the commands used for the screenshots in the project report.

**1. Hostname isolation (UTS namespace)**

```bash
hostname                                 # host: Pirha
sudo ./minidocker run alpine /bin/sh
hostname                                 # container: minidocker
```

**2. Process isolation (PID namespace)**

```bash
ps aux | wc -l                           # host: about 190 processes
# inside the container
sleep 1000 &
ps                                       # only 3 processes, the shell is PID 1
```

**3. Limit a container process from outside**

Terminal 1:

```bash
sudo ./minidocker run alpine /bin/sh
sleep 10000 &
```

Terminal 2:

```bash
ps aux | grep sleep                      # note the PID
sudo ./minidocker limit <PID> 50M 20
sudo ./minidocker status <PID>           # Memory Limit: 52428800, PIDs Limit: 20
sudo ./minidocker stop <PID>
```

**4. Filesystem isolation (chroot and mount namespace)**

```bash
cat /etc/os-release | head -2            # host: Ubuntu
ls /home                                 # host: user folder
sudo ./minidocker run alpine /bin/sh
cat /etc/os-release | head -2            # container: Alpine Linux
ls /home                                 # container: empty
```

**5. Layered storage (OverlayFS)**

```bash
sudo ./minidocker run alpine /bin/sh     # note the id in "[Overlay] Ready: images/c<N>_merged"
echo "hello overlay" > /tmp/test.txt
```

In a second terminal, before the container exits:

```bash
ls images/c<N>_upper/tmp                 # test.txt is here
ls images/alpine/tmp                     # empty, the base image is unchanged
```

**6. Resource issue detection (OOM kill)**

```bash
tail /dev/zero &                         # note the PID
sudo ./minidocker limit <PID> 50M 10
sudo ./minidocker status <PID>
# Result: KILLED (memory limit exceeded, 1 process(es) killed by OOM)
```

**7. CPU limit**

```bash
sudo ./minidocker run --cpu "50000 100000" alpine /bin/sh
while true; do :; done                   # endless loop inside the container
```

In a second terminal:

```bash
sudo ./minidocker status <PID>           # CPU Limit : 50000 100000
top -p <PID>                             # %CPU stays near 50
```

**8. Process count limit**

```bash
sudo ./minidocker run --pids 5 alpine /bin/sh
for i in 1 2 3 4 5 6 7 8; do sleep 100 & done
# /bin/sh: can't fork: Resource temporarily unavailable
exit                                     # status shows PIDs Limit : 5
```

**9. Process monitoring (`/proc`)**

```bash
./minidocker list | head -8
```

## Project Team


| Name        | CMS ID       |
| ----------- | ------------ |
| Pirha Mahar | 053-24-0028  |
| Araza Memon | 053-24-0020  |
| Sumera Naz  | 053-24-00023 |

## Project Information

- **University:** Sukkur IBA University
- **Department:** Computer Science
- **Course:** CS251 – Operating Systems
- **Program:** BS Software Engineering
- **Semester:** BS-V
- **Session:** Fall 2026
- **Instructor:** Sir Abbas Mehdi


