# Makefile
TARGET	= user_test
CC	= gcc
NVCC	= nvcc -arch sm_20 -cubin
LIBS	= -lucuda -lgdev
CFLAGS	= -L /usr/local/gdev/lib64 -I /usr/local/gdev/include

all:
	$(NVCC) -o gpu_hash.cubin gpusha1cuda_test.cu
	gcc -o $(TARGET) $(CFLAGS) main.c gpu_hash_test.c $(LIBS)

clean:
	rm -f $(TARGET) *.cubin ./*~
