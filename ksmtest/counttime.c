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
  char buffer2[20];
  char buffer3[20];
  FILE *fp;
  FILE *fp2;
  FILE *fp3;
  FILE *fp4;
  FILE *fp5;

  fp = fopen("/sys/kernel/mm/ksm/pages_sharing", "r");
  fp2 = fopen("/sys/kernel/mm/ksm/pages_shared", "r");
  fp3 = fopen("/sys/kernel/mm/ksm/full_scans", "r");
  fp4 = fopen("/sys/kernel/mm/ksm/pages_to_scan", "r");
  fp5 = fopen("/sys/kernel/mm/ksm/sleep_millisecs", "r");
  fp6 = fopen("/sys/kernel/mm/ksm/run", "r");
  fread(buffer, 10, 1, fp4);
  printf("pages_to_scan:%s", buffer);
  fread(buffer, 10, 1, fp5);
  printf("sleep_millisecs:%s", buffer);
  fread(buffer, 10, 1, fp6);
  printf("run:%s\n", buffer);

  gettimeofday(&start,  NULL);  
  gettimeofday(&before,  NULL);  
  memset(buffer, 0x0, 20);
  memset(buffer2, 0x0, 20);
  memset(buffer3, 0x0, 20);

  while(1) {
    gettimeofday(&now, NULL);
    if (tv_sub(before, now) > 1.) {
      fseek(fp, 0, SEEK_SET);
      fseek(fp2, 0, SEEK_SET);
      fseek(fp3, 0, SEEK_SET);
      fread(buffer, 10, 1, fp);
      fread(buffer2, 10, 1, fp2);
      fread(buffer3, 10, 1, fp3);
      printf("time %fs:\n sharing:%s shared:%s full_scans:%s", tv_sub(start, now), buffer, buffer2, buffer3);
      gettimeofday(&before, NULL);
    }
    usleep(500000);
  }
  
  return 0;
}
