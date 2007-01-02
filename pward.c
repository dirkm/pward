#include <proc/readproc.h>
#include <stdio.h>
#include <stdlib.h>
#include <search.h>
#include <limits.h>
#include <errno.h>
#include <getopt.h>
#include <string.h>

/* gcc -g -lproc-3.2.6 pward.c -std=c99 -o pward -Wall */

/* blocked, non blocked */
/* wait for process, socket ... */
/* verbose, terse */

/* TODO: */
/* rewrite using bsd process monitoring */

/*   alternative names: babysit, nurse , pnurse, pward, (barrier considered bad) */
/* find way to allocate command-string on stack */
/* add extra checks like ownership (does not make sense if it takes pids) HOORAH */
/* -u root u */

#define VERSION "1"
#define MAX_CMD_LENGTH 1024

const char proc_dump_header[]="PID   STIME    CMD\n";
const char proc_dump_format[]="%5d %8llu %s\n";

static void
print_usage(const char* name)
{
  printf(
"   %s: version "VERSION" \n"
"usage: %s -hv [-f] | [-r running_procs | -s stopped_procs] -e cmd_at_exit -i interval_time\n"
"  running_procs: stop if only n processes are running : (default 0)\n"
"  stopped_procs: stop if n processes are stopped : (default 1)\n"
"  cmd_at_exit: command to be executed if treshold met\n"
"  interval_time: polling interval\n",name,name);
}

/* TODO: add error-handling */

static 
int convert_to_number(const char* arg)
{
  errno = 0;
  char* endptr;
  int result=strtol(arg,&endptr, 10);
  
  if (((errno == ERANGE) && (result == LONG_MAX || result == LONG_MIN))
      ||((errno != 0) && (result == 0))
      ||(*endptr!='\0'))
    {
      printf ("non numeric parameter '%s'\n", optarg);
      exit(-1);
    }
  return result;
}

static
void cleanup_proc(proc_t* p)
{
  if (p->cmdline)
    free((void*)*p->cmdline);
}

static
int cmppid(const void* l, const void* r)
{
  return *((pid_t*)l)-*((pid_t*)r);
}

#define SWAP(Type,parg1,parg2) \
{ \
  Type *p1=parg1, *p2=parg2, tmp=*p1; \
  *p1=*p2; \
  *p2=tmp; \
}

static
int init_check_procs(size_t nProcs, pid_t* pids, unsigned long long* startTimes, _Bool verbose)
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
/*   if treshold not met: overestimate ->  number of processes still to be considered  */

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
	      ++pids,++startTimes; /* entry can only be found once, so increment base value */
	      if((pids-pidsstart)>treshold)
		{
		  pids+=nProcs; /* return overestimate (pretend all remaining entries are matches) */
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

int main(int argc,const char* argv[])
{
  _Bool verbose=0;
  _Bool batch=0;

  char cmd[MAX_CMD_LENGTH]="\0";

  size_t running=0;

  _Bool stopCondition=0;
  size_t stopped=0;
  int nOptions=0;
  int nInterval=1;

  while(1)
    {
      static struct option long_options[]=
	{
	  {"verbose", 0, 0, 'v'},
	  {"help", 0, 0, 'h'},
	  {"running", 2, 0, 'r'},
	  {"stopped", 2, 0, 's'},
	  {"exec", 1, 0, 'e'},
	  {"batch", 0, 0, 'b'},
	  {"interval", 1, 0, 'i'},
	  {0, 0, 0, 0}
	};
    
      int c = getopt_long(argc,(char* const*)argv, "vhfr::s::e:bi:",
		       long_options,NULL);
      if(c==-1)
	break;

      ++nOptions;
      switch(c)
	{
	case 'v':
	  verbose=1;
	  break;
	case 'h':
	  print_usage(argv[0]);
	  exit(-1);
	case 'r':
	  if(optarg!=NULL)
	    running=convert_to_number(optarg);
	  break;
	case 's':
	  stopCondition=1;
	  stopped=(optarg!=NULL)?
	    convert_to_number(optarg):1;
	  break;
	case 'e':
	  strncpy(cmd,optarg,MAX_CMD_LENGTH);
	  if(cmd[MAX_CMD_LENGTH-1]!='\x0')
	    {
	      printf ("command longer than max allowed length '%d'\n", MAX_CMD_LENGTH);
	      exit(-3);
	    }
	  break;
	case 'b':
	  batch=1;
	  break;
	case 'i':
	  nInterval=convert_to_number(optarg);
	  break;
	}
    }

  int nLastOptionIndex=nOptions+1; /* exclude program-name */
  size_t nProcsInit=argc-nLastOptionIndex;

  /* recalculate 'stop' in terms of number of running processes */
  if(stopCondition)
    {
      if(nProcsInit<stopped)
	{
	  if(!batch)
	    fprintf(stderr, "warning: requesting more stopped processes than the number of processes at startup\n");
	}
      else if(running<nProcsInit-stopped)
	running=nProcsInit-stopped;
    }

  pid_t pids[nProcsInit];
  unsigned long long startTimes[nProcsInit];
  for(int i=0;i<nProcsInit;++i)
    {
      pids[i]=convert_to_number(argv[nLastOptionIndex+i]); 
      startTimes[i]=~0ULL;
    }

  size_t nProcs=init_check_procs(nProcsInit,pids,startTimes,verbose);
  if(!batch && !nProcs)
    {
      fprintf(stderr, "no processes found, bailing out\n");
      exit(-4);
    }

  while(1)
    {
      nProcs=check_procs(nProcs,pids,startTimes,running,verbose);
      if(nProcs<=running)
	break;
      sleep(nInterval);
    }
  
  if(*cmd!='\x0')
    system(cmd);
  return 0;
}
