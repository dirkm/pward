#ifndef PWARD_PROC_INFO_H
#define PWARD_PROC_INFO_H

#include <sys/types.h>
#include <stdbool.h>

extern int proc_info_hdr();

extern int proc_info_pid(pid_t pid);

extern int proc_info_proc(proc_t* p);

#endif
