#include <proc/readproc.h>
#include <search.h>
#include <stdlib.h>
#include <stdio.h>

const char proc_dump_header[]="PID   STIME    CMD\n";
const char proc_dump_format[]="%5d %8llu %s\n";

static
int cmppid(const void* l, const void* r)
{
  return *((pid_t*)l)-*((pid_t*)r);
}

static
void cleanup_proc(proc_t* p)
{
  if (p->cmdline)
    free((void*)*p->cmdline);
}

#define SWAP(Type,parg1,parg2) \
{ \
  Type *p1=parg1, *p2=parg2, tmp=*p1; \
  *p1=*p2; \
  *p2=tmp; \
}

static
int init_check_procs(size_t nProcs, pid_t* pids, unsigned long long* startTimes,
		     _Bool verbose)
{
  proc_t buf;
  pid_t* pidsstart=pids;

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
/*   if treshold not met: overestimate ->  
     number of processes still to be considered  */

static
int check_procs(size_t nProcs, pid_t* pids, unsigned long long* startTimes,
		size_t treshold, _Bool verbose)
{
  proc_t buf;
  pid_t* pidsstart=pids;

  PROCTAB* ptp=openproc(PROC_FILLSTAT);
  if(!ptp)
    {
      fprintf(stderr, "Error: cannot access /proc.\n");
      exit(-2);
    }

  while(nProcs && readproc(ptp,&buf))
    {
      pid_t* p=
	(pid_t*)lfind(&buf.tgid,pids,&nProcs,sizeof(pid_t),cmppid);
      if(p!=NULL)
	{
	  --nProcs;	  
	  if(startTimes[p-pids]==buf.start_time)
	    {
	      SWAP(pid_t,p,pids);
	      SWAP(unsigned long long,startTimes,startTimes+(p-pids));
	      /* entry can only be found once, so increment base value */
	      ++pids,++startTimes;
	      if((pids-pidsstart)>treshold)
		{
		  pids+=nProcs; 
		  /* return overestimate (pretend all remaining are matches) */
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
proc_observe_processes(size_t nProcsInit, pid_t* pids,size_t running,_Bool batch,_Bool verbose, int nInterval)
{
  unsigned long long startTimes[nProcsInit];

  for(int i=0;i<nProcsInit;++i)
    {
      startTimes[i]=~0ULL;
    }

  size_t nProcs=init_check_procs(nProcsInit,pids,startTimes,verbose);
  if(!batch && !nProcs)
    {
      fprintf(stderr, "no processes found, bailing out\n");
      return -4;
    }

  while(1)
    {
      nProcs=check_procs(nProcs,pids,startTimes,running,verbose);
      if(nProcs<=running)
	break;
      sleep(nInterval);
    }

  return 0;
}
