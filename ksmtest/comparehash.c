#include <stdio.h>

0 1 2 3 4 5 

int main(int argc, char *argv[])
{
  void* hashptrs[];
  char* hashptr;
  int textnum = 100;
  int i, j;
  hashptrs = malloc(sizeof(void*) * textnum); 
  hashptr = malloc(SHA1_BLOCK_SIZE * textnum);
  memcpy(hashptr, 0x5, SHA1_BLOCK_SIZE * textnum);
  for (i = 0; i < textnum; ++i) {
    hashptrs[i] = hashptr + SHA1_BLOCK_SIZE * i;
  }

  for (i = 0; i < textnum; ++i) {
    for (j = 0; j < i; ++j) {
      if (hash[j] == hash[i])
        bitmap[j] = bitmap[i] = 1;
    }
  }

  return 0;
}
