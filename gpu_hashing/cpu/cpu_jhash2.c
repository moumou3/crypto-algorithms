#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>

uint32_t calc_checksum(char* pgaddr) {
  uint32_t checksum;
  checksum = jhash2(pgaddr, sysconf(_SC_PAGESIZE) / 4, 17);
  return checksum;
}

int main(int argc, char *argv[])
{
  void* pgaddr;
  uint32_t checksum;

  pgaddr = malloc(sysconf(_SC_PAGESIZE));
  checksum = calc_checksum(pgaddr);
  printf("checksum %d\n", checksum);
  return 0;
}
