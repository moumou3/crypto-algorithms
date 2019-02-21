#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

/*
---file example----
50% sharing_potential & distance 1
ffffffff0000... 000000010000...

*/

#define PAGESIZE 4096

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
  FILE *fp = fopen("testfile.txt", "w");
  size_t filesize = atoi(argv[1]);
  int sharing_potential = atoi(argv[2]) ;
  int distance = atoi(argv[3]);
  int d = 0;
  uint32_t uint32_index = 0;
  uint32_t uint32_max = ~uint32_index;
  char buffer[PAGESIZE*10 + 1] = {0};
  int sh = 0;
  int sharable_pagenum = sharing_potential * filesize / 100 / PAGESIZE;
  
  if ((distance + 1) * sharing_potential > 100)
    printf("distance or sharing_potential is too learge\n");

  for (i = 0; i < filesize / PAGESIZE; ++i) {
    if (d == 0 && sh++ < sharable_pagenum) 
      createpage(uint32_max, fp);
    else 
      createpage(uint32_index++, fp);

    d = (d == distance) ? 0:(d+1);
  }
  fp = fopen("testfile.txt", "r");
  fread(buffer, PAGESIZE*10, 1, fp);
  printf("buffer:%s", buffer);
  
  return 0;
}
