#include <proc/readproc.h>
#include <proc/sysinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <search.h>
#include <limits.h>
#include <errno.h>
#include <getopt.h>
#include <string.h>

// gcc -g -lproc-3.2.6 barrier.c -std=c99 -o barrier

// blocked, non blocked
// wait for process, socket ...
// verbose, terse

// put exe option with the command to execute
// make sure that the command is not run if no match is found ever
// have a silent flag to not dump the initial commands at startup
// rewrite using bsd process monitoring

// make wake-up period configurable
//   alternatively babysit, nurse , pnurse, pward

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

static
int init_check_procs(PROCTAB* ptp, pid_t* pids, unsigned long long* startTimes, size_t nProcs, _Bool batch)
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
	  pid_t tmp=*p;
	  *p=*pids;
	  *pids=tmp;

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

// this stops checking after a single positive is found
// hence: sometimes an overestimate might be returned in nProc

static
int check_procs(PROCTAB* ptp, pid_t* pids, unsigned long long* startTimes, size_t nProcs)
{
  proc_t buf;
  pid_t* pidsstart=pids;

  while(readproc(ptp,&buf))
    {
      pid_t* p=
	(pid_t*)lfind(&buf.tgid,pids,&nProcs,sizeof(pid_t),cmppid);
      if(p!=NULL)
	{
	  if(startTimes[p-pids]==buf.start_time)
	    {
	      cleanup_proc(&buf);
	      return nProcs;
	    }
	  else
	    {
	      printf("detected reused process id %d\n",buf.tgid);
	      printf("%d %llu %s\n",buf.tgid,buf.start_time,buf.cmd);
	      cleanup_proc(&buf);
	      // ignore restarted pids in future runs, by moving them to the end of the array
	      --nProcs;
	      *p=pids[nProcs];
	      startTimes[p-pids]=startTimes[nProcs];
	    }
	}
      cleanup_proc(&buf);
    }
  return 0;
}

int main(int argc,const char* argv[])
{
  _Bool verbose=0;
  _Bool batch=0;

  int running=0;
  int stopped=1;
  char cmd[MAX_CMD_LENGTH]="\0";

  while(1)
    {
      int option_index = 0;
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
    
      int c = getopt_long (argc,(char* const*)argv, "vhfr::s::e:",
		       long_options, &option_index);
      if(c==-1)
	break;

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
	  if(optarg!=NULL)
	    stopped=convert_to_number(optarg);
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

  size_t nProcs=argc-1;
  pid_t pids[nProcs];
  unsigned long long startTimes[nProcs];
  for(int i=0;i<nProcs;++i)
    {
      pids[i]=convert_to_number(argv[i+1]);
      // startTimes[i]=~0ULL;
    }

  {
    PROCTAB* ptp=openproc(PROC_FILLARG|PROC_FILLSTAT);
    if(!ptp)
      {
	fprintf(stderr, "Error: cannot access /proc.\n");
	exit(-1);
      }
    size_t nProcs=init_check_procs(ptp,pids,startTimes,nProcs, batch);
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
    
  while(nProcs>running)
    {
      PROCTAB* ptp=ptp=openproc(PROC_FILLSTAT);
      if(!ptp)
	{
	  fprintf(stderr, "Error: cannot access /proc.\n");
	  exit(1);
	}
      nProcs=check_procs(ptp,pids,startTimes,nProcs);
      closeproc(ptp);
    }
  return 0;
}
