#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

float tv_sub(struct timeval before, struct timeval now){
 return now.tv_sec - before.tv_sec + (float)(now.tv_usec - before.tv_usec) / 1000000;


}
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
    if (tv_sub(before, now) > 1.) {
      fseek(fp, 0, SEEK_SET);
      fread(buffer, 10, 1, fp);
      printf("%f: %s", tv_sub(start, now), buffer);
      gettimeofday(&before, NULL);
    }
    usleep(500000);
  }
  
  return 0;
}
