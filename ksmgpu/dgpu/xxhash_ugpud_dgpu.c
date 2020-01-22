#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<fcntl.h>
#include<string.h>
#include<unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <inttypes.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include  <sys/ipc.h>
#include <cuda.h>
#include "../../gpu_hashing/cpu/xxhash.h"



#define MAX_PLATFORMS (10)
#define MAX_DEVICES (10)
#define MAX_SOURCE_SIZE (100000)
#define PAGE_SIZE 4096
#define MADV_UGPUD_FLAG 95
#define MADV_UGPUD_INPUT 96
#define MADV_UGPUD_OUTPUT 97

static inline unsigned long long rdtsc() {
  unsigned long long ret;

  __asm__ volatile ("rdtsc" : "=A" (ret));

  return ret;
}

uint32_t calc_checksum_xxhash(void *pgaddr) {
  uint32_t checksum;
  checksum = xxh32(pgaddr, PAGE_SIZE, 0);
  return checksum;
}

unsigned long long tv_CrContext, tv_CrKernel;
unsigned int anshash = 1474019464U;

static int setup_cuda(int argc, char **argv, CUfunction *pfunction);

cl_command_queue Queue;
cl_kernel k_vadd;
cl_context     context = NULL;

size_t memsize;
int result;
int *test;
int lnum;

int main(int argc, char *argv[]) {
  int ret, fd;
  int platform = 0;
  int device = 0;
  char msg[BUFSIZ];
  unsigned int *hashval;
  unsigned long long* pg_addrs;
  int i;
  unsigned char *mapped_flag;
  

  unsigned char *texts;
  unsigned int *outputs;
  int text_num = atoi(argv[1]);
  int count = 0;

  unsigned long long ndrange_start, ndrange_end, ndrange_sub;
  unsigned long long alloc_start, alloc_end, alloc_sub;
  unsigned long long write_start, write_end, write_sub;
  unsigned long long read_start, read_end, read_sub;
  size_t memsize;
  size_t outsize;
  cl_mem inputobj;
  cl_mem outputobj;
  unsigned int remapcount = 0;

  size_t local_item_size = 256;
  size_t global_item_size = ((text_num+ local_item_size - 1) / local_item_size) * local_item_size;

  CUfunction function;
  CUdeviceptr a_dev, b_dev, c_dev;

  memsize = sysconf(_SC_PAGESIZE) * text_num;
  outsize = ((sizeof(unsigned int) * text_num) / sysconf(_SC_PAGESIZE) + 1) * sysconf(_SC_PAGESIZE);

  mapped_flag = mmap(NULL, sysconf(_SC_PAGESIZE)*1, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  printf("mapped_flag:%llx\n", mapped_flag);


  ret = setup_cuda(0,NULL,&function);
  if (ret > 0)
    printf("ret= %d , %s", ret, msg);

  alloc_start = rdtsc();
  texts = mmap(NULL, memsize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  printf("text:%llx\n", texts);
  outputs = mmap(NULL, outsize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  printf("outputs:%llx\n", outputs);



  alloc_end = rdtsc();
  alloc_sub = alloc_end - alloc_start;

  madvise(mapped_flag, sizeof(int), MADV_UGPUD_FLAG);
  madvise(texts, memsize, MADV_UGPUD_INPUT); 
  madvise(outputs, outsize, MADV_UGPUD_OUTPUT); 
  if (*mapped_flag == 0x0)
    printf("mapped_flag mapped:\n");

  res = cuMemAlloc(&input_dev, memsize);
  res = cuMemAlloc(&output_dev, outsize);
  if (res != CUDA_SUCCESS) {
    printf("cuMemAlloc (a) failed\n");
    return -1;
  }



  while (1) {
    if (*mapped_flag == 0x1) {
      remapcount = outputs[0];
      printf("remapcount : %u\n", remapcount);
      printf("gpu calc start:\n");

      res = cuMemcpyHtoD(input_dev, texts, memsize);
	res = cuParamSeti(function, 0, input_dev);	
	res = cuParamSeti(function, 1, output_dev);	
	res = cuParamSeti(function, 2, remapcount);	
      clSetKernelArg(k_vadd, 0, sizeof(inputobj), &inputobj);
      clSetKernelArg(k_vadd, 1, sizeof(outputobj), &outputobj);
      clSetKernelArg(k_vadd, 2, sizeof(remapcount), &remapcount);

      ndrange_start = rdtsc();
      clEnqueueNDRangeKernel(Queue, k_vadd, 1, NULL, &global_item_size, &local_item_size, 0, NULL, NULL);
      clFinish(Queue);
      ndrange_end = rdtsc();


      ndrange_sub = ndrange_end - ndrange_start;
      /*
      printf("ndrange_sub: %llu\n", ndrange_sub);
      printf("write_sub: %llu\n", write_sub);
      printf("read_sub: %llu\n", read_sub);
      printf("\ngpu calc end:\n");
      */

   mapped_input = (unsigned char*) clEnqueueMapBuffer(Queue, inputobj, CL_TRUE, CL_MAP_READ | CL_MAP_WRITE, 0, memsize, 0, NULL, NULL, &ret);
   mapped_output = (unsigned int*) clEnqueueMapBuffer(Queue, outputobj, CL_TRUE, CL_MAP_READ | CL_MAP_WRITE, 0, outsize, 0, NULL, NULL, &ret);
  if (ret > 0)
    printf("mapped ret= %d , %s", ret, msg);
  printf("mapped_input:%llx\n", mapped_input);
  printf("mapped_output:%llx\n", mapped_output);

      *mapped_flag = 0x2;

    }

  }




  //printf("tv_CrContext: %llu\n", tv_CrContext);
  //printf("tv_CrKernel: %llu\n", tv_CrKernel);
  //printf("ndrange_sub: %llu\n", ndrange_sub);
  printf("alloc_sub: %llu\n", alloc_sub);

  /*
  for (i = 0; i < text_num; ++i) {

    if (anshash != hashval) {
      printf("invalid hash value %d, %u, %u\n", i, anshash, hashval[i]);
    }


  }
  */



  /*
  clReleaseKernel(k_vadd);
  clReleaseCommandQueue(Queue);
  clReleaseContext(context);
  */



  printf("End of the program\n");
  return 0;
}

static int setup_cuda(int argc, char **argv, CUfunction *pfunction)
{

  CUresult res;
  CUdevice dev;
  CUcontext ctx;
  CUfunction function;
  CUmodule module;
  unsigned long long tv_CrContext_start, tv_CrContext_end;
  unsigned long long tv_CrKernel_start, tv_CrKernel_end;
  char ptxfname[20] = "gpuxxhash_dgpu.ptx";
  char kern_name[20] = "gpuxxhash_dgpu";

  res = cuInit(0);
  if (res != CUDA_SUCCESS) {
    printf("cuInit failed: res = %lu\n", (unsigned long)res);
    return -1;
  }
  res = cuDeviceGet(&dev, 0);
  if (res != CUDA_SUCCESS) {
    printf("cuDeviceGet failed: res = %lu\n", (unsigned long)res);
    return -1;
  }
  res = cuCtxCreate(&ctx, 0, dev);
  if (res != CUDA_SUCCESS) {
    printf("cuCtxCreate failed: res = %lu\n", (unsigned long)res);
    return -1;
  }
  res = cuModuleLoad(&module, ptxfname);
  if (res != CUDA_SUCCESS) {
    printf("cuModuleLoad() failed\n");
    return -1;
  }
  res = cuModuleGetFunction(&function, module, "_Z3addPjS_S_j");
  if (res != CUDA_SUCCESS) {
    printf("cuModuleGetFunction() failed\n");
    return -1;
  }
  printf("\nkernel name %s\n\n", source);



  return 0;


}

