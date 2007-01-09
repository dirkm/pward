#include <sys/acct.h>

#define __USE_XOPEN
#include <stdio.h> 

#include <sys/types.h>
#include <sys/stat.h>

#include <stdlib.h>

#include <fcntl.h>
#include <unistd.h>

int
acct_observe_processes(size_t nProcsInit, pid_t* pids,size_t running,_Bool batch,_Bool verbose)
{
  char* tmpPipe=tempnam(NULL,"pward");
  if(mkfifo(tmpPipe, 0600))
    fprintf(stderr,"failed to create fifo: %s\n",tmpPipe);

  int HND=open(tmpPipe, O_RDONLY);
  free(tmpPipe);
  if(HND==-1)
    fprintf(stderr,"failed to open fifo: %s\n",tmpPipe);
  const char buf[1024];
  while (read(HND,(void*)buf,sizeof(buf))>0)
    {
      fprintf(stdout,"read: %s",buf);
    }
  return 0;
}
