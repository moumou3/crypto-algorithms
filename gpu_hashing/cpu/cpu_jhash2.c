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
  unsigned long long calc_checksum_start, calc_checksum_end;

  pgaddr = malloc(PAGESIZE*pgnum);
  memset(pgaddr, 0x5, PAGESIZE*pgnum);

  calc_checksum_start = rdtsc();
  for (int j = 0; j < pgnum; ++j) {
    checksum = calc_checksum_xxhash(pgaddr+j*PAGESIZE);
  }
  calc_checksum_end = rdtsc();



  printf("calc_checksum : %llu\n", calc_checksum_end - calc_checksum_start);
  printf("checksum %u\n", checksum);
  return 0;
}
