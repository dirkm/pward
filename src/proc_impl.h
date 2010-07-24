#ifndef PWARD_PROC_IMPL_H
#define PWARD_PROC_IMPL_H

#include <sys/types.h>
#include <stdbool.h>

extern int
proc_observe_processes(size_t nProcsInit,pid_t* pids, size_t running, bool batch,bool verbose, int nInterval);

#endif
