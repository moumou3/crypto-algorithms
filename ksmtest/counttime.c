#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

int main(int argc, char *argv[])
{
  struct timeval start, before, now;
  char buffer[20];
  FILE *fp;
  fp = fopen("/sys/kernel/mm/ksm/pages_sharing", "r");
  gettimeofday(&start,  NULL);  
  gettimeofday(&before,  NULL);  
  memset(buffer, 0x0, 20);

  while(1) {
    gettimeofday(&now, NULL);
    if (now.tv_sec - before.tv_sec > 1) {
      fseek(fp, 0, SEEK_SET);
      fread(buffer, 10, 1, fp);
      printf("%d: %s", now.tv_sec - start.tv_sec, buffer);
      gettimeofday(&before, NULL);
    }
    usleep(800000);
  }
  
  return 0;
}
