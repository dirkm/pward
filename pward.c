#include "proc_impl.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>

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

static void
print_usage(const char* name)
{
  printf(
"\t%1$s: version "VERSION" \n"
"usage: %1$s -hv [-f] | [-r running_procs | -s stopped_procs]\n"
"\t\t-e cmd_at_exit -i interval_time\n"
"\trunning_procs: stop if only n processes are running : (default 0)\n"
"\tstopped_procs: stop if n processes are stopped : (default 1)\n"
"\tcmd_at_exit: command to be executed if treshold met\n"
"\tinterval_time: polling interval\n",name);
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
      static const struct option long_options[]=
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
	      printf("command longer than max allowed length '%d'\n", 
		     MAX_CMD_LENGTH);
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
	    fprintf(stderr, 
		    "warning: requesting more stopped processes "
		    "than the number of processes at startup\n");
	}
      else if(running<nProcsInit-stopped)
	running=nProcsInit-stopped;
    }

  pid_t pids[nProcsInit];
  for(int i=0;i<nProcsInit;++i)
    {
      pids[i]=convert_to_number(argv[nLastOptionIndex+i]); 
    }
  
  int result=proc_observe_processes(pids,running,batch,verbose,nInterval);
  if(result)
    exit(result);

  if(*cmd!='\x0')
    system(cmd);
  return 0;
}
