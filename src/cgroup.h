#ifndef CGROUP_H
#define CGROUP_H

int  limit_pid(int pid, const char *mem, const char *pids);
void show_status(int pid);
void cleanup_cgroup(int pid);

#endif
