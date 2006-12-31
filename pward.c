#include <proc/readproc.h>
#include <proc/sysinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <search.h>
#include <limits.h>
#include <errno.h>
#include <getopt.h>
#include <string.h>

// gcc -g -lproc-3.2.6 pward.c -std=c99 -o pward

// blocked, non blocked
// wait for process, socket ...
// verbose, terse

// TODO

// rewrite using bsd process monitoring

// make wake-up period configurable
//   alternative names: babysit, nurse , pnurse, pward, (barrier considered bad)

// add extra checks like ownership (does not make sense if it takes pids) HOORAH
// -u root u

#define VERSION "1"
#define MAX_CMD_LENGTH 1024

static void
print_usage(const char* name)
{
  printf(
"   %s: version "VERSION" \n"
"usage: %s -hv [-f] | [-r running_processes | -s stopped_processes] -e cmd_at_exit\n"
"  running_processes: stop if only n processes are running : (default 0)\n"
"  stopped_processes: stop if n processes are stopped : (default 1)\n",name,name);
}

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
int init_check_procs(PROCTAB* ptp, size_t nProcs, pid_t* pids, unsigned long long* startTimes, _Bool batch)
{
  proc_t buf;
  pid_t* pidsstart=pids;
  printf("checked processes\n");
  while(readproc(ptp,&buf))
    {
      pid_t* p=
	(pid_t*)lfind(&buf.tgid,pids,&nProcs,sizeof(pid_t),cmppid);
      if(p!=NULL)
	{
	  SWAP(pid_t,p,pids);

	  *startTimes=buf.start_time;  
	  --nProcs;++pids;++startTimes;

	  if(!batch)
	      printf("%d %llu %s\n",buf.tgid,buf.start_time,buf.cmd);
	}
      cleanup_proc(&buf);
    }
  if(!batch)
    printf("=====\n");
  return pids-pidsstart; // processes_found
}

// BUG!!!
// TODO: split between return value and number of processes that still need checking

static
int check_procs(PROCTAB* ptp, size_t nProcs, pid_t* pids, unsigned long long* startTimes, 
		size_t treshold,_Bool batch)
{
  proc_t buf;
  pid_t* pidsstart=pids;

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
	      ++pids,++startTimes; // entry can only be found once, so increment base value
	      if((pids-pidsstart)>=treshold)
		{
		  cleanup_proc(&buf);
		  return pids-pidsstart;
		}
	    }
	  else
	    {
	      if(!batch)
		{
		  printf("detected reused process id %d\n",buf.tgid);
		  printf("%d %llu %s\n",buf.tgid,buf.start_time,buf.cmd);
		}
	      // ignore restarted pids in future runs
	      *p=pids[nProcs];
	      startTimes[p-pids]=startTimes[nProcs];
	    }
	}
      cleanup_proc(&buf);
    }
  return pids-pidsstart;
}

int main(int argc,const char* argv[])
{
  _Bool verbose=0;
  _Bool batch=0;

  int running=0;
  char cmd[MAX_CMD_LENGTH]="\0";
  int stopped=argc; // overestimate
  int nOptions=0;

  while(1)
    {
      static struct option long_options[]=
	{
	  {"verbose", 0, 0, 'v'},
	  {"help", 0, 0, 'h'},
	  {"running", 2, 0, 'r'},
	  {"stopped", 2, 0, 's'},
	  {"exec", 1, 0, 'e'},
	  {"batch", 1, 0, 'b'},
	  {0, 0, 0, 0}
	};
    
      int c = getopt_long(argc,(char* const*)argv, "vhfr::s::e:",
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
	  stopped=(optarg!=NULL)?
	    convert_to_number(optarg):1;
	  break;
	case 'e':
	  printf ("option short e with value '%s'\n", optarg);
	  strncpy(cmd,optarg,MAX_CMD_LENGTH);
	  if(cmd[MAX_CMD_LENGTH-1]!='\x0')
	    {
	      printf ("command longer than max allowed length '%d'\n", MAX_CMD_LENGTH);
	    }
	  break;
	case 'b':
	  batch=1;
	  break;
	}
    }

  int nLastOptionIndex=nOptions+1; // exclude program-name
  size_t nProcsInit=argc-nLastOptionIndex;

  pid_t pids[nProcsInit];
  unsigned long long startTimes[nProcsInit];
  for(int i=0;i<nProcsInit;++i)
    {
      pids[i]=convert_to_number(argv[i+nLastOptionIndex]); 
      startTimes[i]=~0ULL;
    }

  size_t nProcs;
  {
    PROCTAB* ptp=openproc(PROC_FILLARG|PROC_FILLSTAT);
    if(!ptp)
      {
	fprintf(stderr, "Error: cannot access /proc.\n");
	exit(-1);
      }
    nProcs=init_check_procs(ptp,nProcsInit,pids,startTimes,batch);
    if(!batch)
      {
	if(!nProcs)
	  {
	    fprintf(stderr, "no running process detected, bailing out\n");
	    exit(-2);
	  }
      }
    closeproc(ptp);
  }

  if((nProcsInit>=stopped) && (running<nProcsInit-stopped))
    running=nProcsInit-stopped;
    
  while(nProcs>running)
    {
      PROCTAB* ptp=openproc(PROC_FILLSTAT);
      if(!ptp)
	{
	  fprintf(stderr, "Error: cannot access /proc.\n");
	  exit(-3);
	}
      nProcs=check_procs(ptp,nProcs,pids,startTimes,running,batch);
      closeproc(ptp);
    }

  if(*cmd!='\x0')
    system(cmd);
  return 0;
}
