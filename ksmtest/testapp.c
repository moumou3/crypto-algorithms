#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>


#define PAGESIZE sysconf(_SC_PAGESIZE)
#define oneGB (1 << 30)

int main(int argc, char *argv[])
{
  size_t addr1_size = !strcmp("1GB", argv[2]) ? oneGB: atoi(argv[2]);
  void* addr1 = memalign(PAGESIZE, addr1_size);
  FILE*fp;

  fp = fopen(argv[1], "r");
  fread(addr1, addr1_size, 1, fp);

  madvise(addr1, addr1_size, MADV_MERGEABLE);
  for(;;);
  return 0;
}
