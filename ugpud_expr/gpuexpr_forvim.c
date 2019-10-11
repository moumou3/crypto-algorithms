typedef unsigned char uint8_t;             // 8-bit byte
typedef unsigned int  uint32_t;             // 32-bit word, change to "long" for 16-bit machines


#define PAGE_SIZE 4096
#define HUGE_SIZE 2 * 1024 * 1024




__kernel void gpuexpr(__global unsigned char *page_addr,  int page_num) {
	int thx = get_global_id(0);
        int i;


        if (thx < page_num * HUGE_SIZE) {
          page_addr[thx] = 0;
	}
}


