#define _GNU_SOURCE
#include "proc_impl.h"

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

static void
print_usage()
{
   printf(
      PACKAGE": [OPTION]... [PID]... \n"
      "  -r, --running=n: stop if no more than n processes are still running (default 0)\n"
      "  -s, --stopped=n: stop if at least n processes have stopped (default 1)\n"
      "  -e, --exec=\"cmd\": command to execute when treshold is met\n"
      "  -i, --interval=n: polling interval in seconds (default 1)\n"
      "  -v, --verbose: summarize monitored processes at startup\n"
      "  -b, --batch: no sanity checks on input (useful when scripting)\n"
      "  -h, --help: print this help message\n\n"
      "  [PID]: space separated list of process-ids to be monitored.\n\n"
      "default behaviour: run until all specified processes have died\n"
      "pward-version: "PACKAGE_VERSION" \n");
}

static inline size_t
checked_strtoul(const char* arg, bool* success)
{
   errno=0;
   char* endptr;
   long signed_result=strtol(arg,&endptr,10);
   *success= (errno != ERANGE) &&(errno != EINVAL)
      && (endptr!=arg) && (*endptr=='\0')
      &&(signed_result>=0);
   return (size_t)signed_result;
}

int
main(int argc,const char* argv[])
{
   bool verbose=false;
   bool batch=false;
   const char* cmd="";
   size_t running=0;

   bool stopCondition=false;
   size_t stopped=1;
   int nInterval=1; /* seconds */

   const struct option long_options[]=
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

   while(1)
   {
      int c = getopt_long(argc,(char* const*)argv, "vhr::s::e:bi:",
			  long_options,NULL);
      if(c==-1)
         break;

      switch(c)
      {
         case 'v':
            verbose=1;
            break;
         case 'h':
            print_usage();
            return -1;
         case 'r':
            if(optarg!=NULL)
	    {
               bool success;
               running=checked_strtoul(optarg,&success);
               if(!success)
               {
		  fprintf(stderr,"invalid parameter: '%s' "
			  "as number of running processes\n",optarg);
		  return -1;
               }
	    }
            break;
         case 's':
            stopCondition=1;
            if(optarg!=NULL)
	    {
               bool success;
               stopped=checked_strtoul(optarg,&success);
               if(!success)
               {
		  fprintf(stderr,"invalid parameter: '%s' "
			  "as number as stopped processes\n",optarg);
		  return -1;
               }
	    }
            break;
         case 'e':
            cmd=strdupa(optarg);
            break;
         case 'b':
            batch=1;
            break;
         case 'i':
         {
	    bool success;
	    stopped=checked_strtoul(optarg,&success);
	    if(!success)
            {
               fprintf(stderr,"invalid parameter: '%s' "
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

   if(!nProcsInit && !batch)
   {
      fprintf(stderr,"WARNING: no processes are monitored\n\n");
   }
   /* recalculate 'stop' in terms of number of running processes */
   if(stopCondition)
   {
      if(nProcsInit<stopped)
      {
         if(!batch)
	    fprintf(stderr,"WARNING: requesting more stopped processes (%zu) "
                    "than the number of processes at startup (%zu)\n",stopped,nProcsInit);
      }
      else if(running<nProcsInit-stopped)
         running=nProcsInit-stopped;
   }

   pid_t pids[nProcsInit];
   for(int i=0;i<nProcsInit;++i)
   {
      bool success;
      pids[i]=checked_strtoul(argv[nLastOptionIndex+i],&success);
      if(!success)
      {
         fprintf(stderr,"invalid parameter: '%s' "
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
         fprintf(stderr,"command '%s' failed to spawn\n",cmd);
   }
   return 0;
}
