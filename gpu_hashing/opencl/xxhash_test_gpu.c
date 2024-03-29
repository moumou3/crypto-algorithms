#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<fcntl.h>
#include<string.h>
#include<unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <CL/cl.h>
#include <sys/time.h>
#include <time.h>
 


#define MAX_PLATFORMS (10)
#define MAX_DEVICES (10)
#define MAX_SOURCE_SIZE (100000)
#define PAGE_SIZE 4096

static inline unsigned long long rdtsc() {
  unsigned long long ret;

  __asm__ volatile ("rdtsc" : "=A" (ret));

  return ret;
}

unsigned long long tv_CrContext, tv_CrKernel;
uint32_t anshash = 1474019464U;

static int setup_ocl(cl_uint, cl_uint, char*);

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
  uint32_t *hashval;
  unsigned long long* pg_addrs;
  int i;
  

  unsigned char *texts;
  int text_num = atoi(argv[1]);

  unsigned long long ndrange_start, ndrange_end, ndrange_sub;
  unsigned long long alloc_start, alloc_end, alloc_sub;

  size_t local_item_size = 256;
  size_t global_item_size = ((text_num+ local_item_size - 1) / local_item_size) * local_item_size;

  


  ret = setup_ocl((cl_uint)platform, (cl_uint)device, msg);
  if (ret > 0)
    printf("ret= %d , %s", ret, msg);

  alloc_start = rdtsc();
  
  //--- original pages of text
  texts = clSVMAlloc(context, CL_MEM_READ_WRITE|CL_MEM_SVM_FINE_GRAIN_BUFFER|CL_MEM_SVM_ATOMICS, PAGE_SIZE * text_num, 0);
  memset(texts, 0x5, PAGE_SIZE * text_num);
  hashval = clSVMAlloc(context, CL_MEM_READ_WRITE|CL_MEM_SVM_FINE_GRAIN_BUFFER|CL_MEM_SVM_ATOMICS, sizeof(uint32_t)* text_num, 0);
  //-----

  pg_addrs = clSVMAlloc(context, CL_MEM_READ_WRITE|CL_MEM_SVM_FINE_GRAIN_BUFFER|CL_MEM_SVM_ATOMICS, sizeof(unsigned long long) * text_num, 0);
  alloc_end = rdtsc();
  alloc_sub = alloc_end - alloc_start;

  for (i = 0; i < text_num; ++i) {
    pg_addrs[i] = (unsigned long long)(texts + i* PAGE_SIZE);
  }
  //memset(pg_addrs, 0xff, sizeof(unsigned long long)*text_num);
  
  //printf("pg_addrs[0]:%llx,[1]:%llx, pgaddr 0x%llx, 0x%llx\n", pg_addrs[0], pg_addrs[1], &pg_addrs[0], &pg_addrs[1]);

  clSetKernelArgSVMPointer(k_vadd, 0, pg_addrs);
  clSetKernelArgSVMPointer(k_vadd, 1, hashval);
  clSetKernelArg(k_vadd, 2, sizeof(text_num), &text_num);

  //clSetKernelExecInfo(k_vadd,  CL_KERNEL_EXEC_INFO_SVM_PTRS, text_num * sizeof(unsigned long long), pg_addrs);






  ndrange_start = rdtsc();
  clEnqueueNDRangeKernel(Queue, k_vadd, 1, NULL, &global_item_size, &local_item_size, 0, NULL, NULL);
  clFinish(Queue);
  ndrange_end= rdtsc();
  ndrange_sub = ndrange_end - ndrange_start;



  printf("tv_CrContext: %llu\n", tv_CrContext);
  printf("tv_CrKernel: %llu\n", tv_CrKernel);
  printf("ndrange_sub: %llu\n", ndrange_sub);
  printf("alloc_sub: %llu\n", alloc_sub);

  for (i = 0; i < text_num; ++i) {

    if (anshash != hashval[i]) {
      printf("invalid hash value %d, %u, %u\n", i, anshash, hashval[i]);
    }


  }



  clReleaseKernel(k_vadd);
  clReleaseCommandQueue(Queue);
  clReleaseContext(context);



  printf("End of the program\n");
  return 0;
}

static int setup_ocl(cl_uint platform, cl_uint device, char* msg)
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
  char source[20] = "gpuxxhash.cl";
  char kern_name[20] = "gpuxxhash";

  printf("\nkernel name %s\n\n", source);

  if ((fp = fopen(source, "r")) == NULL) {
    sprintf(msg, "kernel source open error");
    return 1;
  }
  source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
  fclose(fp);

  // program
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

