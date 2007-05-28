#define _GNU_SOURCE
#include "proc_impl.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>

#include <string.h>
#include <limits.h>
#include <sys/types.h>

/* TODO */

/* wait for process, socket ... */
/* verbose, terse */

/* TODO */
/* explore se-linux for process tracking */
/* explore ptrace for process tracking */

/* tried but failed */
/* attempted BSD process accounting but turned out ugly */

static void
print_usage(const char* name)
{
  printf(
"%1$s: version "PACKAGE_VERSION" \n"
"usage: %1$s -hv [-f] | [-r running_procs | -s stopped_procs]\n"
"\t\t-e cmd_at_exit -i interval_time\n"
"\trunning_procs: stop if only n running processes are: (default 0)\n"
"\tstopped_procs: stop if n processes are stopped: (default 1)\n"
"\tcmd_at_exit: command to be executed if treshold met\n"
"\tinterval_time: polling interval\n",name);
}

#define NOT_NUMBER -1

static 
int convert_to_number(const char* arg,void* number)
{
  errno = 0;
  char* endptr;
  long result=
  result=strtol(arg,&endptr, 10);
  
  if (((errno == ERANGE) && (result == LONG_MAX || result == LONG_MIN))
      ||((errno != 0) && (result == 0))
      ||(*endptr!='\0'))
    return NOT_NUMBER;
  *((long*)number)=result;
  return 0;
}

int main(int argc,const char* argv[])
{
  _Bool verbose=0;
  _Bool batch=0;
  char* cmd="";
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
	    if(convert_to_number(optarg,&running))
	      {
		fprintf(stderr,"non numeric parameter as number of running processes");
		return -1;
	      }
	  break;
	case 's':
	  stopCondition=1;
	  if(optarg!=NULL)
	    {
	      if(convert_to_number(optarg,&stopped))
		{
		  fprintf(stderr,"non numeric parameter as number as stopped processes\n");
		  return -1;
		}
	    }
	  else
	    stopped=1;
	  break;
	case 'e':
	  if(optarg!=NULL) 
	    cmd=strdupa(optarg);
	  else
	    {
	      printf("missing argument to command option\n");
	      return -1;
	    }
	  break;
	case 'b':
	  batch=1;
	  break;
	case 'i':
	  if(convert_to_number(optarg,&nInterval))
	    {
	      fprintf(stderr,"non numeric parameter as interval time\n");
	      return -1;
	    }
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
      if(convert_to_number(argv[nLastOptionIndex+i],pids+i))
	{
	  fprintf(stderr,"non numeric parameter at process id\n");
	  return -1;
	}
    }
  
  int result=proc_observe_processes(nProcsInit,pids,running,batch,verbose,nInterval);
  if(result)
    exit(result);

  if(*cmd!='\x0')
    system(cmd);
  return 0;
}
