#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <malloc.h>


#define PAGESIZE sysconf(_SC_PAGESIZE)

int main(int argc, char *argv[])
{
  size_t addr1_size = 1 << 30; //1GB
  void* addr1 = memalign(PAGESIZE, addr1_size);
  FILE*fp;
  char filestr[20] = argv[1];

  fp = fopen(filestr, "r");
  fread(addr1, addr1_size, 1, fp);

  madvise(addr1, addr1_size, MADV_MERGEABLE);
  return 0;
}
