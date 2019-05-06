#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <malloc.h>
#include <string.h>
#include <time.h>


#define PAGESIZE sysconf(_SC_PAGESIZE)
#define oneGB (1 << 30)

void create_rand_page(FILE *fp) {
  int i;
  int random;
  

  for (i = 0; i < PAGESIZE; ++i) {
    random = rand() % 256;
    putc(0x0 + random, fp);
  }

}
int main(int argc, char *argv[])
{
  int i;
  size_t filesize = !strcmp("1GB", argv[1]) ? oneGB: atoi(argv[1]);
  FILE *fp;
  char basename[30] =  "testfile_random";

  printf("Usage: ./createfile_random size\n");
  printf("create about %d page\n", filesize / PAGESIZE); 
  printf("over 4M is expected for same page probability\n");

  fp = fopen(strcat(basename, argv[1]), "w");

  srand((unsigned)time(NULL));

  for (i = 0; i < filesize / PAGESIZE; ++i) {
      create_rand_page(fp);
  }
  return 0;
}
