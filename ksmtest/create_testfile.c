#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <malloc.h>
#include <string.h>

/*
---file example----
50% sharing_potential & distance 1
ffffffff0000... 000000010000...

*/

#define PAGESIZE sysconf(_SC_PAGESIZE)
#define oneGB (1 << 30)

void createpage(uint32_t uint32_index, FILE *fp) {
  int i;
  
  fwrite(&uint32_index, sizeof(uint32_t), 1, fp);
  for (i = 0; i < PAGESIZE - sizeof(uint32_t); ++i) {
    putc('a', fp);
  }

}
int main(int argc, char *argv[])
{
  int i;
  FILE *fp = fopen(argv[4], "w");
  size_t filesize = !strcmp("1GB", argv[1]) ? oneGB: atoi(argv[1]);
  int sharing_potential = atoi(argv[2]) ;
  int distance = atoi(argv[3]);
  int d = 0;
  uint32_t uint32_index = 0;
  uint32_t uint32_max = ~uint32_index;
  int sh = 0;
  int sharable_pagenum = sharing_potential * filesize / 100 / PAGESIZE;
  

  
  if ((distance + 1) * sharing_potential > 100){
    printf("distance or sharing_potential is too learge\n");
    return 1;
  }

  for (i = 0; i < filesize / PAGESIZE; ++i) {
    if (d == 0 && sh++ < sharable_pagenum) 
      createpage(uint32_max, fp);
    else 
      createpage(uint32_index++, fp);

    d = (d == distance) ? 0:(d+1);
  }
  
  return 0;
}
