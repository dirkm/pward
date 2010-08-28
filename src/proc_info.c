

#include "proc_info.h"

static const char proc_dump_header[]="   PID    STIME CMD\n";
static const char proc_dump_format[]="%6d %8llu %s\n";


int proc_info_hdr()
{
   printf("monitoring:\n");
   printf(proc_dump_header);   
}

int proc_info_pid(pid_t pid)
{
   char cmdline[MAX_CMDLENGTH];
   read_cmdline(cmdline,sizeof(cmdline),pid);
   printf(proc_dump_format,pid,*st_it,cmdline);
}

int proc_info_proc(proc_t* p)
{
}
