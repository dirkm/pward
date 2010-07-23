#define _GNU_SOURCE
#include "proc_impl.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>

#include <string.h>
#include <limits.h>
#include <sys/types.h>

/* TODO */
/* added functionality */
/*   wait for process, socket ... */
/*   add more efficient implementations*/
/*   explore ptrace for process tracking */

/* tried but failed */
/*   attempted BSD process accounting but turned out ugly,
     because of huge file that monitors every process in the system +
     required root-access to start monitoring */

/* cleanup printing to actually get tabbing right */

static void
print_usage(const char* name)
{
   printf(
      "%1$s: version "PACKAGE_VERSION" \n"
      "usage: %1$s [-h] | { pids } [-v] [-b] [-r(running_procs) | -s(stopped_procs)]\n"
      "\t\t[-e\"cmd_at_exit\"] [-iinterval_time]\n"
      "\trunning_procs: stop if no more than n processes are left (default 0)\n"
      "\tstopped_procs: stop if at least n processes are stopped (default 1)\n"
      "\tcmd_at_exit: command to be executed if treshold met\n"
      "\tinterval_time: polling interval in seconds (default 1)\n"
      "\t-v: be verbose about monitored processes\n"
      "\t-b: disable some sanity checks for scripting\n"
      "\t-h: print this help message\n"
      "\n"
      "\tdefault: run until all specified processes ended\n"
      ,name);
}

#define CHECKED_STRTOX(func,success,result,arg)         \
   do                                                   \
   {                                                    \
      errno=0;                                          \
      char* endptr;                                     \
      result=func(arg,&endptr,10);                      \
      if ((errno == ERANGE) || (errno == EINVAL)        \
          ||(endptr==arg)                               \
          ||(*endptr!='\0'))                            \
         success=0;                                     \
      else                                              \
         success=1;                                     \
   }                                                    \
   while(0)

int main(int argc,const char* argv[])
{
   bool verbose=false;
   bool batch=false;
   const char* cmd="";
   size_t running=0;

   bool stopCondition=false;
   size_t stopped=1;
   int nInterval=1; // seconds

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

      switch(c)
      {
         case 'v':
            verbose=1;
            break;
         case 'h':
            print_usage(argv[0]);
            return -1;
         case 'r':
            if(optarg!=NULL)
	    {
               bool success;
               CHECKED_STRTOX(strtoul,success,running,optarg);
               if(!success)
               {
		  fprintf(stderr,"non numeric parameter: '%s'"
			  "as number of running processes",optarg);
		  return -1;
               }
	    }
            break;
         case 's':
            stopCondition=1;
            if(optarg!=NULL)
	    {
               bool success;
               CHECKED_STRTOX(strtoul,success,stopped,optarg);
               if(!success)
               {
		  fprintf(stderr,"non numeric parameter: '%s'"
			  "as number as stopped processes\n",optarg);
		  return -1;
               }
	    }
            break;
         case 'e':
            if(optarg!=NULL) 
               cmd=strdupa(optarg);
            else
	    {
               fprintf(stderr,"missing argument to option 'exec'\n");
               return -1;
	    }
            break;
         case 'b':
            batch=1;
            break;
         case 'i':
         {
	    bool success;
	    CHECKED_STRTOX(strtoul,success,stopped,optarg);
	    if(!success)
            {
               fprintf(stderr,"non numeric parameter: '%s'"
                       "as interval time\n",optarg);
               return -1;
            }
	    break;
         }
         case '?':
            return -1; /* missing opt argument */
      }
   }

   int nLastOptionIndex=optind;
   size_t nProcsInit=argc-nLastOptionIndex;

   /* recalculate 'stop' in terms of number of running processes */
   if(stopCondition)
   {
      if(nProcsInit<stopped)
      {
         if(!batch)
	    fprintf(stderr, 
		    "warning: requesting more stopped processes (%zu)"
		    "than the number of processes at startup (%zu)\n",stopped,nProcsInit);
      }
      else if(running<nProcsInit-stopped)
         running=nProcsInit-stopped;
   }

   pid_t pids[nProcsInit];
   for(int i=0;i<nProcsInit;++i)
   {
      bool success;
      CHECKED_STRTOX(strtoul,success,pids[i],argv[nLastOptionIndex+i]);
      if(!success)
      {
         fprintf(stderr,"non numeric parameter: '%s'"
                 "as process id\n",argv[nLastOptionIndex+i]);
         return -1;
      }
   }
  
   int result=proc_observe_processes
      (nProcsInit,pids,running,batch,verbose,nInterval);
   if(result)
      return result;
  
   if(verbose)
      printf("condition met: %zu processes left\n", running);

   if(*cmd!='\x0')
   {
      if(verbose)
         printf("executing command: '%s'\n",cmd);
      if(!system(cmd))
         fprintf(stderr,"could not run command '%s'",cmd);
   }
   return 0;
}
