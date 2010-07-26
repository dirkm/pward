#include <linux/limits.h>
#include <proc_impl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <proc/readproc.h>

static const char proc_dump_header[]="   PID    STIME CMD\n";
static const char proc_dump_format[]="%6d %8llu %s\n";

static const char proc_dir[]="/proc";

const int MAX_CMDLENGTH=256;

/* get_proc_stats writes perror when the process does not exist.
   the code below works around the issue, but performance suffers and a potential race
   is introduced */

static inline
proc_t * hacked_get_proc_stats(pid_t pid, proc_t *p)
{
   char path[PATH_MAX];
   struct stat statbuf;

   sprintf(path, "/proc/%d", pid);
   return stat(path, &statbuf)?
      NULL:
      get_proc_stats(pid,p);
}

static
size_t init_check_procs(size_t nProcs, pid_t* pids, unsigned long long* start_times,
                        bool verbose)
{
   struct stat statbuf;
   if(stat(proc_dir,&statbuf))
   {
      fprintf(stderr, "Error: cannot access /proc.\n");
      exit(-1);
   }

   if(verbose)
   {
      printf("monitoring:\n");
      printf(proc_dump_header);
   }
   pid_t* pid_it=pids;
   unsigned long long* st_it=start_times;
   while(pid_it<pids+nProcs)
   {
      proc_t buf;
      proc_t* r=hacked_get_proc_stats(*pid_it,&buf);
      if(r)
      {
         *st_it=buf.start_time;
         if(verbose)
         {
            char cmdline[MAX_CMDLENGTH];
            read_cmdline(cmdline,sizeof(cmdline),*pid_it);
            printf(proc_dump_format,*pid_it,*st_it,cmdline);
         }
         ++pid_it,++st_it;
      }
      else
      {
         fprintf(stderr,"WARNING: process %d not found\n",*pid_it);
         --nProcs;
         *pids=pids[nProcs];
      }
   }
   if(verbose)
      printf("-----\n");
   return nProcs; /* processes_found */
}

/* return-value:  */
/*   Normally: number of processes still found */
/*   if treshold is not met: overestimate (number of processes still to be considered)  */

static
size_t check_procs(size_t nProcs,pid_t* pids, unsigned long long* start_times,
                   size_t treshold, bool verbose)
{
   pid_t* pid_it=pids;
   unsigned long long* st_it=start_times;
   while(pid_it<pids+nProcs)
   {
      proc_t buf;
      proc_t* r=hacked_get_proc_stats(*pid_it,&buf);
      if(r)
      {
         if(*st_it==buf.start_time)
         {
            if((pid_it-pids)<treshold)
            {
               ++pid_it;++st_it;
               continue;
            }
            else
               break; /* nProcs will be an overestimate */
         }
         else if(verbose)
         {
            printf("reused process id detected: %d\n",buf.tgid);
            printf(proc_dump_format,buf.tgid,buf.start_time,buf.cmd);
         }
      }
      --nProcs;
      *pid_it=pids[nProcs];
      *st_it=start_times[nProcs];
   }
   return nProcs;
}

int
proc_observe_processes(size_t nProcsInit,pid_t* pids,size_t running,bool batch,
                       bool verbose, int nInterval)
{
   unsigned long long start_times[nProcsInit];

   size_t nProcs=init_check_procs(nProcsInit,pids,start_times,verbose);
   if(!batch && !nProcs)
   {
      if(verbose)
         printf("no processes found, bailing out\n");
      return -1;
   }

   while(nProcs>running)
   {
      sleep(nInterval);
      nProcs=check_procs(nProcs,pids,start_times,running,verbose);
   }
   return 0;
}
