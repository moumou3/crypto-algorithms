#include <cuda.h>
#ifdef __KERNEL__ /* just for measurement */
#include <linux/vmalloc.h>
#include <linux/time.h>
#define printf printk
#define malloc vmalloc
#define free vfree
#define gettimeofday(x, y) do_gettimeofday(x)
#else /* just for measurement */
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

#define PAGESIZE 4096
/* tvsub: ret = x - y. */
static inline void tvsub(struct timeval *x, 
						 struct timeval *y, 
						 struct timeval *ret)
{
	ret->tv_sec = x->tv_sec - y->tv_sec;
	ret->tv_usec = x->tv_usec - y->tv_usec;
	if (ret->tv_usec < 0) {
		ret->tv_sec--;
		ret->tv_usec += 1000000;
	}
}

int cuda_test_hash(unsigned int n, char *path)
{
	int i, j, idx;
	CUresult res;
	CUdevice dev;
	CUcontext ctx;
	CUfunction function;
	CUmodule module;
	CUdeviceptr text1_dev, b_dev, c_dev, pass_dev;
	unsigned char pass = 0x1;
	unsigned char *a = (unsigned char*) malloc (PAGESIZE);
	unsigned int *b = (unsigned int *) malloc (n*n * sizeof(unsigned int));
	unsigned int *c = (unsigned int *) malloc (n*n * sizeof(unsigned int));
	int block_x, block_y, grid_x, grid_y;
	char fname[256];
	struct timeval tv;
	struct timeval tv_total_start, tv_total_end;
	float total;
	struct timeval tv_h2d_start, tv_h2d_end;
	float h2d;
	struct timeval tv_d2h_start, tv_d2h_end;
	float d2h;
	struct timeval tv_exec_start, tv_exec_end;
	struct timeval tv_mem_alloc_start;
	struct timeval tv_data_init_start;
	float data_init;
	struct timeval tv_conf_kern_start;
	struct timeval tv_close_start;
	float mem_alloc;
	float exec;
	float init_gpu;
	float configure_kernel;
	float close_gpu;
	float data_read;

	unsigned int dummy_b, dummy_c;
	

	/* block_x * block_y should not exceed 512. */
	block_x = n < 16 ? n : 16;
	block_y = n < 16 ? n : 16;
	grid_x = n / block_x;
	if (n % block_x != 0)
		grid_x++;
	grid_y = n / block_y;
	if (n % block_y != 0)
		grid_y++;
	printf("block = (%d, %d)\n", block_x, block_y);
	printf("grid = (%d, %d)\n", grid_x, grid_y);

	gettimeofday(&tv_total_start, NULL);

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

	sprintf(fname, "%s/gpu_hash.cubin", path);
	res = cuModuleLoad(&module, fname);
	if (res != CUDA_SUCCESS) {
		printf("cuModuleLoad() failed\n");
		return -1;
	}
	res = cuModuleGetFunction(&function, module, "gpusha1");
	if (res != CUDA_SUCCESS) {
		printf("cuModuleGetFunction() failed\n");
		return -1;
	}
	res = cuFuncSetBlockShape(function, block_x, block_y, 1);
	if (res != CUDA_SUCCESS) {
		printf("cuFuncSetBlockShape() failed\n");
		return -1;
	}

	gettimeofday(&tv_mem_alloc_start, NULL);


	/* a[] */
	res = cuMemAlloc(&text1_dev, PAGESIZE);
	if (res != CUDA_SUCCESS) {
		printf("cuMemAlloc (a) failed\n");
		return -1;
	}

        res = cuMemAlloc(&pass_dev, sizeof(unsigned char));
	if (res != CUDA_SUCCESS) {
		printf("cuMemAlloc (c) failed\n");
		return -1;
	}
	gettimeofday(&tv_data_init_start, NULL);

	/* initialize A[] & B[] */
        memset(a, 0x5, PAGESIZE);
        


	gettimeofday(&tv_h2d_start, NULL);
	/* upload a[] and b[] */
	res = cuMemcpyHtoD(text1_dev, a, PAGESIZE);
	if (res != CUDA_SUCCESS) {
		printf("cuMemcpyHtoD (a) failed: res = %lu\n", (unsigned long)res);
		return -1;
	}
	gettimeofday(&tv_h2d_end, NULL);

	gettimeofday(&tv_conf_kern_start, NULL);

	/* set kernel parameters */
	res = cuParamSeti(function, 0, text1_dev);	
	if (res != CUDA_SUCCESS) {
		printf("cuParamSeti (a) failed: res = %lu\n", (unsigned long)res);
		return -1;
	}
	res = cuParamSeti(function, 4, text1_dev >> 32);
	if (res != CUDA_SUCCESS) {
		printf("cuParamSeti (a) failed: res = %lu\n", (unsigned long)res);
		return -1;
	}
	res = cuParamSeti(function, 8, pass_dev);
	if (res != CUDA_SUCCESS) {
		printf("cuParamSeti (c) failed: res = %lu\n", (unsigned long)res);
		return -1;
	}
	res = cuParamSeti(function, 12, pass_dev >> 32);
	if (res != CUDA_SUCCESS) {
		printf("cuParamSeti (c) failed: res = %lu\n", (unsigned long)res);
		return -1;
	}
	res = cuParamSeti(function, 16, n);
	if (res != CUDA_SUCCESS) {
		printf("cuParamSeti (c) failed: res = %lu\n", (unsigned long)res);
		return -1;
	}
	res = cuParamSetSize(function, 20);
	if (res != CUDA_SUCCESS) {
		printf("cuParamSetSize failed: res = %lu\n", (unsigned long)res);
		return -1;
	}

	gettimeofday(&tv_exec_start, NULL);
	/* launch the kernel */
	res = cuLaunchGrid(function, grid_x, grid_y);
	if (res != CUDA_SUCCESS) {
		printf("cuLaunchGrid failed: res = %lu\n", (unsigned long)res);
		return -1;
	}
	cuCtxSynchronize();
	gettimeofday(&tv_exec_end, NULL);

	gettimeofday(&tv_d2h_start, NULL);
	/* download c[] */
	res = cuMemcpyDtoH(&pass, pass_dev, sizeof(unsigned char));
        printf("pass hash value 0x%x \n", pass);
	if (res != CUDA_SUCCESS) {
		printf("cuMemcpyDtoH (c) failed: res = %lu\n", (unsigned long)res);
		return -1;
	}
	gettimeofday(&tv_d2h_end, NULL);


	gettimeofday(&tv_close_start, NULL);

	res = cuMemFree(text1_dev);
	if (res != CUDA_SUCCESS) {
		printf("cuMemFree (a) failed: res = %lu\n", (unsigned long)res);
		return -1;
	}
	res = cuMemFree(pass_dev);
	if (res != CUDA_SUCCESS) {
		printf("cuMemFree (pass_dev) failed: res = %lu\n", (unsigned long)res);
		return -1;
	}

	res = cuModuleUnload(module);
	if (res != CUDA_SUCCESS) {
		printf("cuModuleUnload failed: res = %lu\n", (unsigned long)res);
		return -1;
	}

	res = cuCtxDestroy(ctx);
	if (res != CUDA_SUCCESS) {
		printf("cuCtxDestroy failed: res = %lu\n", (unsigned long)res);
		return -1;
	}

	free(a);
	free(b);
	free(c);

	gettimeofday(&tv_total_end, NULL);


	tvsub(&tv_mem_alloc_start, &tv_total_start, &tv);
	init_gpu = tv.tv_sec * 1000.0 + (float) tv.tv_usec / 1000.0;

	tvsub(&tv_data_init_start, &tv_mem_alloc_start, &tv);
	mem_alloc = tv.tv_sec * 1000.0 + (float) tv.tv_usec / 1000.0;

	tvsub(&tv_h2d_start, &tv_data_init_start, &tv);
	data_init = tv.tv_sec * 1000.0 + (float) tv.tv_usec / 1000.0;

	tvsub(&tv_h2d_end, &tv_h2d_start, &tv);
	h2d = tv.tv_sec * 1000.0 + (float) tv.tv_usec / 1000.0;

	tvsub(&tv_exec_start, &tv_conf_kern_start, &tv);
	configure_kernel = tv.tv_sec * 1000.0 + (float) tv.tv_usec / 1000.0;

	tvsub(&tv_exec_end, &tv_exec_start, &tv);
	exec = tv.tv_sec * 1000.0 + (float) tv.tv_usec / 1000.0;

	tvsub(&tv_d2h_end, &tv_d2h_start, &tv);
	d2h = tv.tv_sec * 1000.0 + (float) tv.tv_usec / 1000.0;

	tvsub(&tv_close_start, &tv_d2h_end, &tv);
	data_read = tv.tv_sec * 1000.0 + (float) tv.tv_usec / 1000.0;

	tvsub(&tv_total_end, &tv_close_start, &tv);
	close_gpu = tv.tv_sec * 1000.0 + (float) tv.tv_usec / 1000.0;

	tvsub(&tv_total_end, &tv_total_start, &tv);
	total = tv.tv_sec * 1000.0 + (float) tv.tv_usec / 1000.0;

	printf("Init: %f\n", init_gpu);
	printf("MemAlloc: %f\n", mem_alloc);
	printf("DataInit: %f\n", data_init);
	printf("HtoD: %f\n", h2d);
	printf("KernConf: %f\n", configure_kernel);
	printf("Exec: %f\n", exec);
	printf("DtoH: %f\n", d2h);
	printf("DataRead: %f\n", data_read);
	printf("Close: %f\n", close_gpu);
	printf("Total: %f\n", total);

	return 0;
}


