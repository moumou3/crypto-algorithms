#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include "jhash.h"
#include "xxhash.h"

#define PAGESIZE sysconf(_SC_PAGESIZE)

static inline unsigned long long rdtsc() {
  unsigned long long ret;

  __asm__ volatile ("rdtsc" : "=A" (ret));

  return ret;
}


uint32_t calc_checksum_jhash2(void *pgaddr) {
  uint32_t checksum;
  checksum = jhash2(pgaddr, PAGESIZE / 4, 17);
  return checksum;
}
uint32_t calc_checksum_xxhash(void *pgaddr) {
  uint32_t checksum;
  checksum = xxh32(pgaddr, PAGESIZE, 0);
  return checksum;
}

int main(int argc, char *argv[])
{
  void* pgaddr;
  uint32_t checksum;
  int pgnum = atoi(argv[1]);

  pgaddr = malloc(PAGESIZE*pgnum);
  for (int j = 0; j < pgnum; ++j) {
    for (int i = 0; i < PAGESIZE; ++i) {
      ((char*)(pgaddr+j*PAGESIZE))[i] = i+j*(PAGESIZE+1);
    }
    checksum = calc_checksum_xxhash(pgaddr+j*PAGESIZE);
    printf("checksum %u\n", checksum);
  }
  memset(pgaddr, 0x5, PAGESIZE);

    checksum = calc_checksum_xxhash(pgaddr);
    printf("aachecksum %u\n", checksum);
    ((char*)pgaddr)[0]+=3; 
    ((char*)pgaddr)[1]+=3; 
    ((char*)pgaddr)[5]-=3; 
    ((char*)pgaddr)[6]-=3; 
    checksum = calc_checksum_xxhash(pgaddr);
    printf("aachecksum %u\n", checksum);


  return 0;
}
