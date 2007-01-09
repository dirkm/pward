#ifndef PWARD_PROC_IMPL_H
#define PWARD_PROC_IMPL_H

#include <sys/types.h>

extern int
proc_observe_processes(pid_t pids[*], size_t running, _Bool batch,_Bool verbose, int nInterval);

#endif
