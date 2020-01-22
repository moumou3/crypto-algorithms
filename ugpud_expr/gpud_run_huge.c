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
#define MADV_EXPR_FLAG 97
#define MADV_EXPR_INPUT 98
#define MADV_EXPR_OUTPUT 99
#define HUGE_SIZE 2 * 1024 * 1024
#define SHSCRIPT \
   "echo 20 | sudo tee /proc/sys/vm/nr_hugepages"

int main(int argc, char *argv[])
{
  unsigned char *hugeapp_mem;
  //system(SHSCRIPT);
  hugeapp_mem= mmap(NULL, HUGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_HUGETLB|MAP_ANONYMOUS, 0, 0);
  hugeapp_mem[0] = 0x11;
  madvise(hugeapp_mem, sizeof(int), MADV_EXPR_RUN); 
  while(1);
  return 0;
}
