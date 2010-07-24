#include <proc/readproc.h>
#include <search.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

const char proc_dump_header[]="   PID    STIME CMD\n";
const char proc_dump_format[]="%6d %8llu %s\n";

static inline
int cmppid(const void* l, const void* r)
{
   return *((pid_t*)l)-*((pid_t*)r);
}

static inline
void cleanup_proc(proc_t* p)
{
   if (p->cmdline)
      free((void*)*p->cmdline);
}

#define SWAP(Type,parg1,parg2)                  \
   do                                           \
   {                                            \
      Type *p1=parg1, *p2=parg2, tmp=*p1;       \
      *p1=*p2;                                  \
      *p2=tmp;                                  \
   }                                            \
   while(0)

static
int init_check_procs(size_t nProcs, pid_t* pids, unsigned long long* startTimes,
		     bool verbose)
{
   PROCTAB* ptp=openproc(PROC_FILLARG|PROC_FILLSTAT);
   if(!ptp)
   {
      fprintf(stderr, "Error: cannot access /proc.\n");
      exit(-1);
   }

   if(verbose)
   {
      printf("monitoring:\n");
      printf(proc_dump_header);
   }

   pid_t* pidsstart=pids;
   proc_t buf;
   while(readproc(ptp,&buf))
   {
      pid_t* p=
         (pid_t*)lfind(&buf.tgid,pids,&nProcs,sizeof(pid_t),cmppid);
      if(p!=NULL)
      {
         SWAP(pid_t,p,pids);

         *startTimes=buf.start_time;
         --nProcs;++pids;++startTimes;

         if(verbose)
            printf(proc_dump_format,buf.tgid,buf.start_time,buf.cmd);
      }
      cleanup_proc(&buf);
   }
   if(verbose)
      printf("-----\n");

   closeproc(ptp);
   return pids-pidsstart; // processes_found
}

/* return-value:  */
/*   Normally: number of processes still found */
/*   if treshold is not met: overestimate (number of processes still to be considered)  */

static
int check_procs(size_t nProcs,pid_t* pids, unsigned long long* startTimes,
		size_t treshold, bool verbose)
{
   PROCTAB* ptp=openproc(PROC_FILLSTAT);
   if(!ptp)
   {
      fprintf(stderr, "Error: cannot access /proc.\n");
      exit(-2);
   }

   pid_t* pidsstart=pids;
   proc_t buf;
   while(nProcs && readproc(ptp,&buf))
   {
      pid_t* p=
         (pid_t*)lfind(&buf.tgid,pids,&nProcs,sizeof(pid_t),cmppid);
      if(p!=NULL)
      {
         --nProcs;
         if(startTimes[p-pids]==buf.start_time)
         {
            /* pids are stored in the order that they are found;
               we assume that the next run will returns results in the same order
               this should speed up our linear array lookup */
            SWAP(pid_t,p,pids);
            SWAP(unsigned long long,startTimes,startTimes+(p-pids));
            /* an entry can only be found once, so increment base value */
            ++pids,++startTimes;
            if((pids-pidsstart)>treshold)
            {
               pids+=nProcs;
               /* return overestimate (pretend all remaining pids are matches) */
               cleanup_proc(&buf);
               break;
            }
         }
         else
         {
            if(verbose)
            {
               printf("reused process id detected: %d\n",buf.tgid);
               printf(proc_dump_format,buf.tgid,buf.start_time,buf.cmd);
            }
            /* ignore restarted pids in future runs */
            *p=pids[nProcs];
            startTimes[p-pids]=startTimes[nProcs];
         }
      }
      cleanup_proc(&buf);
   }
   closeproc(ptp);
   return pids-pidsstart;
}

int
proc_observe_processes(size_t nProcsInit,pid_t* pids,size_t running,bool batch,
		       bool verbose, int nInterval)
{
   unsigned long long startTimes[nProcsInit];
   for(int i=0;i<nProcsInit;++i)
   {
      startTimes[i]=~0ULL;
   }

   size_t nProcs=init_check_procs(nProcsInit,pids,startTimes,verbose);
   if(!batch && !nProcs)
   {
      if(verbose)
         fprintf(stderr, "no processes found, bailing out\n");
      return -4;
   }

   while(1)
   {
      nProcs=check_procs(nProcs,pids,startTimes,running,verbose);
      if(nProcs<=running)
         break;
      sleep(nInterval); // TODO: maybe time has to include time taken for check_procs
   }

   return 0;
}
