#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<fcntl.h>
#include<string.h>
#include<unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <inttypes.h>
#include <CL/cl.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include  <sys/ipc.h>

#define MADV_EXPR_RUN 94

int main(int argc, char *argv[])
{
  unsigned char *dummy_advise;
  dummy_advise = mmap(NULL, sysconf(_SC_PAGESIZE)*1, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  madvise(dummy_advise, sizeof(int), MADV_EXPR_RUN); 
  return 0;
}
