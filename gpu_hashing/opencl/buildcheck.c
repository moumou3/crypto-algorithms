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


cl_program     program = NULL;
void buildcheck(cl_device_id device_id, const char* options)
{

  // build


  cl_int ret_val = clBuildProgram(program, 1, device_id, options, NULL, NULL);



  // avoid abortion due to CL_BILD_PROGRAM_FAILURE

  if (ret_val != CL_SUCCESS && ret_val != CL_BUILD_PROGRAM_FAILURE)
    CL_CHECK_ERROR(ret_val);



  cl_build_status build_status;

  CL_CHECK_ERROR(clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_STATUS, sizeof(cl_build_status), &build_status, NULL));

  if (build_status == CL_SUCCESS)
    return;

  char *build_log;

  size_t ret_val_size;

  CL_CHECK_ERROR(clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &ret_val_size));

  build_log = (char*) malloc(ret_val_size+1);

  CL_CHECK_ERROR(clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, ret_val_size, build_log, NULL));

  // to be carefully, terminate with \0

  // there's no information in the reference whether the string is 0 terminated or not

  build_log[ret_val_size] = '\0';




  printf("Build log %s", build_log);



}



int main(int argc, char *argv[])
{
  char source[10] = "gpusha1.cl";
  char kern_name[10] = "gpusha1";
  cl_program     program = NULL;
  cl_platform_id platform_id[MAX_PLATFORMS];
  cl_device_id   device_id[MAX_DEVICES];
  int device = 0;
  int platform = 0;
  cl_command_queue Queue;
  cl_kernel k_vadd;
  cl_context     context = NULL;
  FILE *fp;
  char *source_str;
  char str[BUFSIZ];
  size_t source_size, ret_size, size;
  cl_uint num_platforms, num_devices;
  cl_int ret;

  source_str = (char *)malloc(MAX_SOURCE_SIZE * sizeof(char));

  if ((fp = fopen(source, "r")) == NULL) {
    printf("kernel source open error");
    return 1;
  }
  source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
  fclose(fp);

  clGetPlatformIDs(MAX_PLATFORMS, platform_id, &num_platforms);
  clGetDeviceIDs(platform_id[platform], CL_DEVICE_TYPE_ALL, MAX_DEVICES, device_id, &num_devices);
  // program

  context = clCreateContext(NULL, 1, &device_id[device], NULL, NULL, &ret);
  Queue = clCreateCommandQueue(context, device_id[device], 0, &ret);
  program = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &ret);
  program = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &ret);
  buildcheck(device_id, "");
  return 0;
}
