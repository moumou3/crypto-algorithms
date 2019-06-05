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
#include <CL/cl.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include  <sys/ipc.h>
#include "../gpu_hashing/cpu/xxhash.h"



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

static int setup_ocl(cl_uint platform, cl_uint device, char* msg, void* inputA, void* inputB, size_t sizeA, size_t sizeB);

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
  size_t memsize;
  size_t outsize;
  unsigned int remapcount = 0;

  size_t local_item_size = 256;
  size_t global_item_size = ((text_num+ local_item_size - 1) / local_item_size) * local_item_size;

  memsize = sysconf(_SC_PAGESIZE) * text_num;
  outsize = ((sizeof(unsigned int) * text_num) / sysconf(_SC_PAGESIZE) + 1) * sysconf(_SC_PAGESIZE);

  mapped_flag = mmap(NULL, sysconf(_SC_PAGESIZE)*1, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  printf("mapped_flag:%llx\n", mapped_flag);



  alloc_start = rdtsc();
  texts = mmap(NULL, memsize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  printf("text:%llx\n", texts);
  outputs = mmap(NULL, outsize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  printf("outputs:%llx\n", outputs);

//  texts = clSVMAlloc(context, CL_MEM_READ_WRITE|CL_MEM_SVM_FINE_GRAIN_BUFFER|CL_MEM_SVM_ATOMICS, memsize + outsize, 0);
 // printf("text:%llx\n", texts);
  //outputs = texts + memsize;
  //outputs = clSVMAlloc(context, CL_MEM_READ_WRITE|CL_MEM_SVM_FINE_GRAIN_BUFFER|CL_MEM_SVM_ATOMICS, outsize, 0);
 // printf("outputs:%llx\n", outputs);

  alloc_end = rdtsc();
  alloc_sub = alloc_end - alloc_start;

#if 0
  madvise(mapped_flag, sizeof(int), MADV_UGPUD_FLAG);
  madvise(texts, memsize, MADV_UGPUD_INPUT); 
  ret = madvise(outputs, outsize, MADV_UGPUD_OUTPUT); 
#endif
  texts[0] = 0x5; 


  if (ret != 0)
    printf("ret %d", ret);
  if (*mapped_flag == 0x0)
    printf("mapped_flag mapped:\n");

  ret = setup_ocl((cl_uint)platform, (cl_uint)device, msg, texts, outputs, memsize, outsize);
  if (ret > 0)
    printf("ret= %d , %s", ret, msg);

 alloc_start= rdtsc();
  //Create buffer
  cl_mem inputobj = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, memsize, texts, &ret);
  cl_mem outputobj = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, outsize, outputs, &ret);
  unsigned char *ptrMapped = (unsigned char*)clEnqueueMapBuffer(Queue, inputobj, CL_TRUE, CL_MAP_WRITE, 0, memsize,  0, NULL, NULL, &ret);
  ptrMapped[0] = 0x5;
  printf("ptrMapped %x", ptrMapped[0]);
  ret = clEnqueueUnmapMemObject(Queue, inputobj, ptrMapped, 0, NULL, NULL);
  alloc_end = rdtsc();
  printf("alloc_sub: %llu\n", alloc_end-alloc_start);
  return 0;
  while (1) {
    if (*mapped_flag == 0x1) {
      remapcount = outputs[0];
      printf("remapcount : %u\n", remapcount);
      printf("gpu calc start:\n");

//        unsigned int a = calc_checksum_xxhash(texts);
 //       printf("aaaaaaaa %d\n\n", a);
      for (int j = 0; j < remapcount; j++) {
        outputs[j] = calc_checksum_xxhash(texts+j*PAGE_SIZE);
      }
      for (int j = 0; j < remapcount; j++) {
        printf("outputs %d\n\nn", outputs[j]);
      }
      *mapped_flag = 0x2;

      while(1);
      //clSetKernelArgSVMPointer(k_vadd, 0, texts);
      //clSetKernelArgSVMPointer(k_vadd, 1, outputs);
      //clSetKernelArg(k_vadd, 2, sizeof(remapcount), &remapcount);
      clSetKernelArg(k_vadd, 0, sizeof(cl_mem), (void*)&inputobj);
      clSetKernelArg(k_vadd, 1, sizeof(cl_mem), (void*)&outputobj);
      clSetKernelArg(k_vadd, 2, sizeof(remapcount), &remapcount);

      ndrange_start = rdtsc();
      clEnqueueNDRangeKernel(Queue, k_vadd, 1, NULL, &global_item_size, &local_item_size, 0, NULL, NULL);
      clFinish(Queue);
      ndrange_end = rdtsc();
      ndrange_sub = ndrange_end - ndrange_start;

      printf("ndrange_sub: %llu\n", ndrange_sub);
      printf("\ngpu calc end:\n");
      *mapped_flag = 0x2;

    }

  }


  /* SVM

  clSetKernelArgSVMPointer(k_vadd, 0, texts);
  clSetKernelArgSVMPointer(k_vadd, 1, hashval);
  clSetKernelArg(k_vadd, 2, sizeof(text_num), &text_num);

  //clSetKernelExecInfo(k_vadd,  CL_KERNEL_EXEC_INFO_SVM_PTRS, text_num * sizeof(unsigned long long), pg_addrs);






  ndrange_start = rdtsc();
  clEnqueueNDRangeKernel(Queue, k_vadd, 1, NULL, &global_item_size, &local_item_size, 0, NULL, NULL);
  clFinish(Queue);
  ndrange_end= rdtsc();
  ndrange_sub = ndrange_end - ndrange_start;

  */


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

static int setup_ocl(cl_uint platform, cl_uint device, char* msg, void* inputA, void* inputB, size_t sizeA, size_t sizeB)
{
  cl_program     program = NULL;
  cl_platform_id platform_id[MAX_PLATFORMS];
  cl_device_id   device_id[MAX_DEVICES];

  FILE *fp;
  char *source_str;
  char str[BUFSIZ];
  size_t source_size, ret_size, size;
  cl_uint num_platforms, num_devices;
  cl_int ret;
  unsigned long long tv_CrContext_start, tv_CrContext_end;
  unsigned long long tv_CrKernel_start, tv_CrKernel_end;

  // alloc
  source_str = (char *)malloc(MAX_SOURCE_SIZE * sizeof(char));

  // platform
  clGetPlatformIDs(MAX_PLATFORMS, platform_id, &num_platforms);
  if (platform >= num_platforms) {
    sprintf(msg, "error : platform = %d (limit = %d)", platform, num_platforms - 1);
    return 1;
  }

  // device
  clGetDeviceIDs(platform_id[platform], CL_DEVICE_TYPE_ALL, MAX_DEVICES, device_id, &num_devices);
  if (device >= num_devices) {
    sprintf(msg, "error : device = %d (limit = %d)", device, num_devices - 1);
    return 1;
  }

  // device name (option)
  clGetDeviceInfo(device_id[device], CL_DEVICE_NAME, sizeof(str), str, &ret_size);
  sprintf(msg, "%s (platform = %d, device = %d)", str, platform, device);
  char version[100];
  clGetPlatformInfo(platform_id[platform], CL_PLATFORM_VERSION, 100, version, &ret_size);
  printf("version, %s", version);

  //svm capability
  cl_device_svm_capabilities caps;
  clGetDeviceInfo(device_id[0], CL_DEVICE_SVM_CAPABILITIES, sizeof(cl_device_svm_capabilities), &caps, NULL);
  int svmCoarse     = 0!=(caps & CL_DEVICE_SVM_COARSE_GRAIN_BUFFER);
  int svmFineBuffer = 0!=(caps & CL_DEVICE_SVM_FINE_GRAIN_BUFFER);
  int svmFineSystem = 0!=(caps & CL_DEVICE_SVM_FINE_GRAIN_SYSTEM);
  int svmAtomics= 0!=(caps & CL_DEVICE_SVM_ATOMICS);
  printf("svmcoarse, %d", svmCoarse);
  printf("svmbuffer, %d", svmFineBuffer);
  printf("svmfinesys, %d", svmFineSystem);
  printf("svmAtomics, %d", svmAtomics);
  printf("svm cap, %d", caps);

  // context
  tv_CrContext_start = rdtsc();
  context = clCreateContext(NULL, 1, &device_id[device], NULL, NULL, &ret);
  tv_CrContext_end = rdtsc();

  // command queue
  Queue = clCreateCommandQueue(context, device_id[device], 0, &ret);



  /*create program*/ 
  char source[20] = "gpuxxhash.cl";
  char kern_name[20] = "gpuxxhash";

  printf("\nkernel name %s\n\n", source);

  if ((fp = fopen(source, "r")) == NULL) {
    sprintf(msg, "kernel source open error");
    return 1;
  }
  source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
  fclose(fp);

  program = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &ret);
  if (ret != CL_SUCCESS) {
    sprintf(msg, "clCreateProgramWithSource() error");
    return 1;
  }

  // build
  ret = clBuildProgram(program, 1, &device_id[device], NULL, NULL, NULL);
  if(ret != CL_SUCCESS) {
    sprintf(msg, "clBuildProgram() error");
    return -ret;
  }

  // kernel
  tv_CrKernel_start = rdtsc();
  k_vadd = clCreateKernel(program, kern_name, &ret);
  tv_CrKernel_end = rdtsc();
  if (ret != CL_SUCCESS) {
    sprintf(msg, "clCreateKernel() error");
    return 1;
  }


  tv_CrContext = tv_CrContext_end - tv_CrContext_start;
  tv_CrKernel = tv_CrKernel_end - tv_CrKernel_start;


  clReleaseProgram(program);
  free(source_str);

  return 0;


}

