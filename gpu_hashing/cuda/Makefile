# Makefile
TARGET	= crypto_test
CC	= gcc
NVCC	= nvcc -arch sm_30 -cubin -g -G
LIBS	= -lcuda 
CFLAGS	= -g

all:
	$(NVCC) -o gpu_hash.cubin gpusha1cuda_test.cu
	$(CC) -o $(TARGET) $(CFLAGS) main.c gpu_hash_test.c $(LIBS)

clean:
	rm -f $(TARGET) *.cubin ./*~
