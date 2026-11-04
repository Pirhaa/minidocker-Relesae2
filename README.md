# minidocker
Mini Docker - Lightweight Container Runtime in C (OS Project)

## Build
    make

## Usage
    ./minidocker list                                 # processes, biggest memory first
    sudo ./minidocker run [--mem 50M] [--pids 20] alpine [cmd]
    sudo ./minidocker limit <pid> <mem> <pids>        # e.g. limit 1234 50M 20
    ./minidocker status <pid>                         # RUNNING / EXITED / KILLED (OOM)
    ./minidocker stop <pid>                           # SIGTERM, then SIGKILL if needed
