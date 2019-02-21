#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <malloc.h>


#define PAGESIZE sysconf(_SC_PAGESIZE)

int main(int argc, char *argv[])
{
  size_t addr1_size = PAGESIZE;
  void* addr1 = memalign(PAGESIZE, addr1_size);
  size_t addr2_size = 2 * PAGESIZE;
  void* addr2 = memalign(PAGESIZE, addr2_size);
  madvise(addr1, addr1_size, MADV_MERGEABLE);
  madvise(addr2, addr2_size, MADV_MERGEABLE);
  memset(addr1, 0x5, addr1_size);
  memset(addr2, 0x5, addr2_size);
  return 0;
}
