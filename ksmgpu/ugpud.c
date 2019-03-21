#include <stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<fcntl.h>
#include<string.h>
#include <sys/ioctl.h>
#include<unistd.h>
#include <CL/cl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/syscall.h>
#include  <sys/ipc.h>

static inline unsigned long long rdtsc() {

  uint32_t hi, lo;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  return ( (uint64_t)lo)|( ((uint64_t)hi)<<32 );

}

#define MAX_PLATFORMS (10)
#define MAX_DEVICES (10)
#define MAX_SOURCE_SIZE (100000)
#define SHA1_BLOCK_SIZE 20              // SHA1 outputs a 20 byte digest

int fd;
cl_command_queue Queue;
cl_kernel k_vadd;
cl_context     context = NULL;
size_t memsize;
int ret;

// ans hash for all 0x5 page
unsigned char anshash[SHA1_BLOCK_SIZE] = {0x73, 0x2f, 0x20, 0x71, 0x22, 0x21, 0x18, 0x5f, 0x27, 0xd, 0xcd, 0xef, 0x18, 0x7b, 0x1b, 0xae, 0x53, 0x72, 0x15, 0x71};

static int setup_ocl(cl_uint platform, cl_uint device, char* msg);

int main(int argc, char *argv[])
{
  char *mapped_input;
  char msg[100];
  int packet_num = 3;
  int *sum;
  size_t local_item_size = 256;
  size_t global_item_size = 1;
  long pgsize = sysconf(_SC_PAGESIZE); 
  size_t hashsize;
  int batchnum = 64;

  ret = setup_ocl(0, 0, msg);
  memsize = pgsize * batchnum;
  hashsize = SHA1_BLOCK_SIZE * batchnum;
  mapped_input = clSVMAlloc(context, CL_MEM_READ_WRITE|CL_MEM_SVM_FINE_GRAIN_BUFFER | CL_MEM_SVM_ATOMICS, memsize + 1, 0);
  memset(mapped_input, 0x5, memsize);

  hashval = clSVMAlloc(context, CL_MEM_READ_WRITE|CL_MEM_SVM_FINE_GRAIN_BUFFER | CL_MEM_SVM_ATOMICS, hashsize, 0);
  //madvise(mapped_input, memsize, MADV_UGPUD); 

  //memsize byte means the flag of mapping complete
  mapped_input[memsize] = 0;
  //expr start by 0x1
  mapped_input[memsize] = 0x1;

  clSetKernelArgSVMPointer(k_vadd, 0, mapped_input);
  clSetKernelArgSVMPointer(k_vadd, 1, hashval);
  clSetKernelArg(k_vadd, 2, sizeof(batchnum), &batchnum);
  int count;
  while (1) {
    if (count == 10) {
      break;
    }
    if (mapped_input[memsize] == 0x1) {
      //remapping by kernel complete or processing
      //now debugging instead of gpu computing
      printf("mapped_input[0]%d", mapped_input[0]);
      clEnqueueNDRangeKernel(Queue, k_vadd, 1, NULL, &global_item_size, &local_item_size, 0, NULL, NULL);
      clFinish(Queue);
      break;
      
    }

    count++;
  }

  printf("gpu complete %x", mapped_input[memsize]); 
  printf("hash result 0x%x",hashval[0]); 

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
  char source[10] = "madd.cl";
  char kern_name[10] = "madd";
  char *source_str;
  char str[BUFSIZ];
  size_t source_size, ret_size, size;
  cl_uint num_platforms, num_devices;
  cl_int ret;
  uint64_t rdt1;
  uint64_t rdt2;
  char version[100];
  cl_device_svm_capabilities caps;
  int svmCoarse, svmFineBuffer, svmFineSystem, svmAtomics;

  source_str = (char*)malloc(MAX_SOURCE_SIZE*sizeof(char));

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
  clGetPlatformInfo(platform_id[platform], CL_PLATFORM_VERSION, 100, version, &ret_size);
  printf("version, %s", version);


  // context
  context = clCreateContext(NULL, 1, &device_id[device], NULL, NULL, &ret);

  // command queue
  Queue = clCreateCommandQueue(context, device_id[device], 0, &ret);

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
  if (clBuildProgram(program, 1, &device_id[device], NULL, NULL, NULL) != CL_SUCCESS) {
    sprintf(msg, "clBuildProgram() error");
    return 1;
  }


  // kernel
  k_vadd = clCreateKernel(program, kern_name, &ret);
  if (ret != CL_SUCCESS) {
    sprintf(msg, "clCreateKernel() error");
    return 1;
  }


  clReleaseProgram(program);
  free(source_str);

  return 0;


}
