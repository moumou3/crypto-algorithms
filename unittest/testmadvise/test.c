#include <stdio.h>
#include <sys/mman.h>


int main(int argc, char *argv[])
{
  unsigned char mapped_flag = 0x0;
  madvise(&mapped_flag, 1, MADV_UGPUD_FLAG);
 while(1) {
   if (mapped_flag == 0x3)
     printf("mapped");
 }
  return 0;
}
