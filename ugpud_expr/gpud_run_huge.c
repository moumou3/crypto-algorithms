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
#define HUGE_SIZE 2 * 1024 * 1024

int main(int argc, char *argv[])
{
  unsigned char *dummy_advise;
  dummy_advise = mmap(NULL, HUGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_HUGETLB|MAP_ANONYMOUS, 0, 0);
  madvise(dummy_advise, HUGE_SIZE, MADV_EXPR_RUN); 
  return 0;
}
