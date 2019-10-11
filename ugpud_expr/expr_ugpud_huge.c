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


#define HUGE_SIZE 2 * 1024 * 1024
#define MAX_PLATFORMS (10)
#define MAX_DEVICES (10)
#define MAX_SOURCE_SIZE (100000)
#define PAGE_SIZE 4096
#define MADV_EXPR_RUN 94
#define MADV_EXPR_FLAG 97
#define MADV_EXPR_INPUT 98
#define MADV_EXPR_OUTPUT 99

#define SHSCRIPT \
   "echo 20 | sudo tee /proc/sys/vm/nr_hugepages"

static inline unsigned long long rdtsc() {
  unsigned long long ret;

  __asm__ volatile ("rdtsc" : "=A" (ret));

  return ret;
}


unsigned long long tv_CrContext, tv_CrKernel;

static int setup_ocl(cl_uint, cl_uint, char*);

cl_command_queue Queue;
cl_kernel k_vadd;
cl_context     context = NULL;

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
  unsigned int *mapped_pagenum;
  unsigned char *dummy_advise;

  unsigned char *inputs;
  unsigned int *outputs;
  int page_num = atoi(argv[1]);
  int count = 0;

  unsigned long long ndrange_start, ndrange_end, ndrange_sub;
  unsigned long long enqueuemap_start, enqueuemap_end, enqueuemap_sub;
  unsigned long long alloc_start, alloc_end, alloc_sub;
  unsigned long long write_start, write_end, write_sub;
  unsigned long long read_start, read_end, read_sub;
  unsigned long long testmemset_start, testmemset_end, testmemset_sub;
  unsigned long long memmap_start, memmap_end, memmap_sub;
  size_t inputsize;
  size_t outsize;
  cl_mem inputobj;
  cl_mem outputobj;
  unsigned int remapcount = 0;

  size_t local_item_size = 256;
  size_t global_item_size = ((page_num+ local_item_size - 1) / local_item_size) * local_item_size;

  inputsize=  HUGE_SIZE * page_num;

  system(SHSCRIPT);

  mapped_flag = mmap(NULL, sysconf(_SC_PAGESIZE)*1, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  mapped_pagenum = (void*)mapped_flag + sizeof(unsigned char);
  dummy_advise = mmap(NULL, sysconf(_SC_PAGESIZE)*1, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  printf("mapped_flag:%llx\n", mapped_flag);


  ret = setup_ocl((cl_uint)platform, (cl_uint)device, msg);
  if (ret > 0)
    printf("ret= %d , %s", ret, msg);

  inputs = mmap(NULL, inputsize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_HUGETLB|MAP_ANONYMOUS, 0, 0);
  printf("text:%llx\n", inputs);




  madvise(mapped_flag, sizeof(int), MADV_EXPR_FLAG);
  *mapped_pagenum = page_num;
  madvise(inputs, inputsize, MADV_EXPR_INPUT); 
  if (*mapped_flag == 0x0)
    printf("mapped_flag mapped:\n");

  inputobj = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, inputsize, inputs, &ret);

  unsigned char* mapped_input = (unsigned char*) clEnqueueMapBuffer(Queue, inputobj, CL_TRUE, CL_MAP_READ | CL_MAP_WRITE, 0, inputsize, 0, NULL, NULL, &ret);
  if (ret > 0)
    printf("mapped ret= %d , %s", ret, msg);

  printf("mapped_input:%llx\n", mapped_input);

  while (1) {
    if (*mapped_flag == 0x1) {
//      printf("gpu calc start:\n");

      memmap_start = rdtsc();
      clEnqueueUnmapMemObject(Queue, inputobj, mapped_input, 0, NULL, NULL);
      //-clEnqueueUnmapMemObject(Queue, outputobj, mapped_output, 0, NULL, NULL);
      clSetKernelArg(k_vadd, 0, sizeof(inputobj), &inputobj);
      clSetKernelArg(k_vadd, 1, sizeof(page_num), &page_num);
      memmap_end = rdtsc();

      ndrange_start = rdtsc();
      clEnqueueNDRangeKernel(Queue, k_vadd, 1, NULL, &global_item_size, &local_item_size, 0, NULL, NULL);
      clFinish(Queue);
      ndrange_end = rdtsc();




      enqueuemap_start = rdtsc();
   mapped_input = (unsigned char*) clEnqueueMapBuffer(Queue, inputobj, CL_TRUE, CL_MAP_READ | CL_MAP_WRITE, 0, inputsize, 0, NULL, NULL, &ret);
      enqueuemap_end = rdtsc();

   /*
   for (int r = 0; r < inputsize; ++r) {
     mapped_input[r] = 0;
   }
   */

   //-mapped_output = (unsigned int*) clEnqueueMapBuffer(Queue, outputobj, CL_TRUE, CL_MAP_READ | CL_MAP_WRITE, 0, outsize, 0, NULL, NULL, &ret);
 // if (ret > 0)
  //  printf("mapped ret= %d , %s", ret, msg);
 // printf("mapped_input:%llx\n", mapped_input);
  //-printf("mapped_output:%llx\n", mapped_output);

      *mapped_flag = 0x2;
      ndrange_sub = ndrange_end - ndrange_start;
      enqueuemap_sub = enqueuemap_end - enqueuemap_start;
      memmap_sub = memmap_end - memmap_start;
      printf("ndrange_sub: %llu\n", ndrange_sub);
      printf("enqueuemap_sub: %llu\n", enqueuemap_sub);
      printf("memmap_sub: %llu\n", memmap_sub);

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
  char source[20] = "gpuexpr.cl";
  char kern_name[20] = "gpuexpr";

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

