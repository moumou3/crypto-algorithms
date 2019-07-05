typedef unsigned char uint8_t;             // 8-bit byte
typedef unsigned int  uint32_t;             // 32-bit word, change to "long" for 16-bit machines


#define PAGE_SIZE 4096
#define xxh_rotl32(x, r) ((x << r) | (x >> (32 - r)))

__constant uint32_t PRIME32_1 = 2654435761U;
__constant uint32_t PRIME32_2 = 2246822519U;
__constant uint32_t PRIME32_3 = 3266489917U;
__constant uint32_t PRIME32_4 =  668265263U;
__constant uint32_t PRIME32_5 =  374761393U;

uint32_t get_unaligned_le32(const uint8_t *p)
{
	return p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24;
}

uint32_t xxh32_round(uint32_t seed, const uint32_t input)
{
	seed += input * PRIME32_2;
	seed = xxh_rotl32(seed, 13);
	seed *= PRIME32_1;
	return seed;
}

uint32_t xxh32(const void *input, const size_t len, const uint32_t seed)
{
	const uint8_t *p = (const uint8_t *)input;
	const uint8_t *b_end = p + len;
	uint32_t h32;

	if (len >= 16) {
		const uint8_t *const limit = b_end - 16;
		uint32_t v1 = seed + PRIME32_1 + PRIME32_2;
		uint32_t v2 = seed + PRIME32_2;
		uint32_t v3 = seed + 0;
		uint32_t v4 = seed - PRIME32_1;

		do {
			v1 = xxh32_round(v1, get_unaligned_le32(p));
			p += 4;
			v2 = xxh32_round(v2, get_unaligned_le32(p));
			p += 4;
			v3 = xxh32_round(v3, get_unaligned_le32(p));
			p += 4;
			v4 = xxh32_round(v4, get_unaligned_le32(p));
			p += 4;
		} while (p <= limit);

		h32 = xxh_rotl32(v1, 1) + xxh_rotl32(v2, 7) +
			xxh_rotl32(v3, 12) + xxh_rotl32(v4, 18);
	} else {
		h32 = seed + PRIME32_5;
	}

	h32 += (uint32_t)len;

	while (p + 4 <= b_end) {
		h32 += get_unaligned_le32(p) * PRIME32_3;
		h32 = xxh_rotl32(h32, 17) * PRIME32_4;
		p += 4;
	}

	while (p < b_end) {
		h32 += (*p) * PRIME32_5;
		h32 = xxh_rotl32(h32, 11) * PRIME32_1;
		p++;
	}

	h32 ^= h32 >> 15;
	h32 *= PRIME32_2;
	h32 ^= h32 >> 13;
	h32 *= PRIME32_3;
	h32 ^= h32 >> 16;

	return h32;
}

__kernel void gpuxxhash(__global unsigned char *texts, __global uint32_t* hashval, int text_num) {
	int thx = get_global_id(0);
        unsigned char text_dev[PAGE_SIZE];
        uint32_t hashval_dev;
        int i;

        if (thx < text_num  ) {

          for (i = 0; i < PAGE_SIZE; i++) {
              text_dev[i] = texts[i + thx * PAGE_SIZE];
          }
	  hashval_dev = xxh32(text_dev, PAGE_SIZE, 0);
          hashval[thx] = hashval_dev;
	}
}


