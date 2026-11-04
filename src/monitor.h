#ifndef MONITOR_H
#define MONITOR_H

void list_processes(void);        // all processes, biggest memory (VmRSS) first
void print_status(int pid);       // State + VmRSS of one process
int  process_alive(int pid);      // 1 if running/sleeping, 0 if gone or zombie
int  stop_process(int pid);       // SIGTERM first, SIGKILL if it does not stop

#endif
