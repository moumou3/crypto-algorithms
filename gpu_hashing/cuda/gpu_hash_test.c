#include <cuda.h>
#ifdef __KERNEL__ /* just for measurement */
#include <linux/vmalloc.h>
#include <linux/time.h>
#define printf printk
#define malloc vmalloc
#define free vfree
#else /* just for measurement */
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

#define PAGESIZE 4096
#define SHA1_BLOCK_SIZE 20
/* tvsub: ret = x - y. */

static inline unsigned long long rdtsc() {
  unsigned long long ret;

  __asm__ volatile ("rdtsc" : "=A" (ret));

  return ret;
}

unsigned char anshash[SHA1_BLOCK_SIZE] = {0x73, 0x2f, 0x20, 0x71, 0x22, 0x21, 0x18, 0x5f, 0x27, 0xd, 0xcd, 0xef, 0x18, 0x7b, 0x1b, 0xae, 0x53, 0x72, 0x15, 0x71};

int cuda_test_hash(unsigned int n, char *path)
{
	int i, j, idx;
	CUresult res;
	CUdevice dev;
	CUcontext ctx;
	CUfunction function;
	CUmodule module;
	CUdeviceptr text_dev, hashval_dev;
	unsigned char *text_host = (unsigned char*) malloc (PAGESIZE*n);
	unsigned char *hashval_host = (unsigned char*) malloc (SHA1_BLOCK_SIZE*n);
	int block_x, grid_x;
	char fname[256];
	unsigned long long tv_total_start, tv_total_end;
	unsigned long long total;
	unsigned long long tv_h2d_start, tv_h2d_end;
	unsigned long long h2d;
	unsigned long long tv_d2h_start, tv_d2h_end;
	unsigned long long d2h;
	unsigned long long tv_exec_start, tv_exec_end;
	unsigned long long tv_mem_alloc_start;
	unsigned long long tv_data_init_start;
	unsigned long long data_init;
	unsigned long long tv_conf_kern_start;
	unsigned long long tv_close_start;
	unsigned long long mem_alloc;
	unsigned long long exec;
	unsigned long long init_gpu;
	unsigned long long configure_kernel;
	unsigned long long close_gpu;
	unsigned long long data_read;

	unsigned int dummy_b, dummy_c;
	

	/* block_x  should not exceed 512. */
	block_x = n < 16 ? n : 16;
	grid_x = n / block_x;
	if (n % block_x != 0)
		grid_x++;
	printf("block = (%d)\n", block_x);
	printf("grid = (%d)\n", grid_x);

	tv_total_start = rdtsc();

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
	res = cuFuncSetBlockShape(function, block_x, 1, 1);
	if (res != CUDA_SUCCESS) {
		printf("cuFuncSetBlockShape() failed\n");
		return -1;
	}

	tv_mem_alloc_start = rdtsc();


	/* text_dev[] */
	res = cuMemAlloc(&text_dev, PAGESIZE*n);
	if (res != CUDA_SUCCESS) {
		printf("cuMemAlloc (a) failed\n");
		return -1;
	}

        res = cuMemAlloc(&hashval_dev, SHA1_BLOCK_SIZE * n);
	if (res != CUDA_SUCCESS) {
		printf("cuMemAlloc (c) failed\n");
		return -1;
	}
	tv_data_init_start = rdtsc();

	/* initialize hash value that is maybe uncontinuous*/
        memset(text_host, 0x5, PAGESIZE*n);


	tv_h2d_start = rdtsc();
	/* upload hashes */
	res = cuMemcpyHtoD(text_dev, text_host, PAGESIZE*n);
	if (res != CUDA_SUCCESS) {
		printf("cuMemcpyHtoD (a) failed: res = %lu\n", (unsigned long)res);
		return -1;
	}
	tv_h2d_end = rdtsc();

	tv_conf_kern_start = rdtsc();

	/* set kernel parameters */
	res = cuParamSeti(function, 0, text_dev);
	if (res != CUDA_SUCCESS) {
		printf("cuParamSeti (a) failed: res = %lu\n", (unsigned long)res);
		return -1;
	}
	res = cuParamSeti(function, 4, text_dev >> 32);
	if (res != CUDA_SUCCESS) {
		printf("cuParamSeti (a) failed: res = %lu\n", (unsigned long)res);
		return -1;
	}
	res = cuParamSeti(function, 8, hashval_dev);
	if (res != CUDA_SUCCESS) {
		printf("cuParamSeti (c) failed: res = %lu\n", (unsigned long)res);
		return -1;
	}
	res = cuParamSeti(function, 12, hashval_dev >> 32);
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

	tv_exec_start = rdtsc();
	/* launch the kernel */
	res = cuLaunchGrid(function, grid_x, grid_y);
	if (res != CUDA_SUCCESS) {
		printf("cuLaunchGrid failed: res = %lu\n", (unsigned long)res);
		return -1;
	}
	cuCtxSynchronize();
	tv_exec_end = rdtsc();

	tv_d2h_start = rdtsc();
	/* download c[] */
	res = cuMemcpyDtoH(hashval_host, hashval_dev, SHA1_BLOCK_SIZE * n);
	if (res != CUDA_SUCCESS) {
		printf("cuMemcpyDtoH (c) failed: res = %lu\n", (unsigned long)res);
		return -1;
	}
	tv_d2h_end = rdtsc();


	tv_close_start = rdtsc();

	res = cuMemFree(text_dev);
	if (res != CUDA_SUCCESS) {
		printf("cuMemFree (a) failed: res = %lu\n", (unsigned long)res);
		return -1;
	}
	res = cuMemFree(hashval_dev);
	if (res != CUDA_SUCCESS) {
		printf("cuMemFree (hashval_dev) failed: res = %lu\n", (unsigned long)res);
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

	free(text_host);

	tv_total_end = rdtsc();


	init_gpu = tv_mem_alloc_start - tv_total_start;
	mem_alloc = tv_data_init_start - tv_mem_alloc_start;
	data_init = tv_h2d_start - tv_data_init_start;
	h2d = tv_h2d_end - tv_h2d_start;
	configure_kernel = tv_exec_start - tv_conf_kern_start;
	exec = tv_exec_end - tv_exec_start;
	d2h = tv_d2h_end - tv_d2h_start;
	data_read = tv_close_start - tv_d2h_end;
	close_gpu = tv_total_end - tv_close_start;
	total = tv_total_end - tv_total_start;

	printf("Init: %llu\n", init_gpu);
	printf("MemAlloc: %llu\n", mem_alloc);
	printf("DataInit: %llu\n", data_init);
	printf("HtoD: %llu\n", h2d);
	printf("KernConf: %llu\n", configure_kernel);
	printf("Exec: %llu\n", exec);
	printf("DtoH: %llu\n", d2h);
	printf("DataRead: %llu\n", data_read);
	printf("Close: %llu\n", close_gpu);
	printf("Total: %llu\n", total);

	for (i = 0; i < n; ++i) {
          for (int j = 0; j < SHA1_BLOCK_SIZE; j++) {

            if (anshash[j] != hashval_host[j + i*SHA1_BLOCK_SIZE]) {
              printf("invalid hash value ");
            }

          }

	}

	return 0;
}


