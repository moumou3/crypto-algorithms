#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#define UGPUD_SVM 96
#define UGPUD_FLAG 95

int main(int argc, char *argv[])
{
  unsigned char *mapped_flag;
  unsigned char* mapped_addr;

  //mapped_flag = aligned_alloc(sysconf(_SC_PAGESIZE), sysconf(_SC_PAGESIZE) *5);
  mapped_flag = mmap(NULL, sysconf(_SC_PAGESIZE)*1, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  madvise(mapped_flag, 1, UGPUD_FLAG);
  printf("mapped_flag1 %p\n", mapped_flag);

  mapped_addr = mmap(NULL, sysconf(_SC_PAGESIZE)*100, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  madvise(mapped_addr, 1, UGPUD_SVM);

 while(1) {
   if (*mapped_flag == 0x1) {
     printf("launching gpu");
     *mapped_flag = 0x2;
     break;
   }
 }
  return 0;
}
