# Makefile
TARGET	= user_test
CC	= gcc
NVCC	= nvcc -arch sm_30 -cubin
LIBS	= -lcuda
CFLAGS	= -g 
all:
	$(NVCC) -g -G -o madd_gpu.cubin madd_gpu.cu
	gcc -o $(TARGET) $(CFLAGS) main.c madd.c $(LIBS)

clean:
	rm -f $(TARGET) *.cubin ./*~
