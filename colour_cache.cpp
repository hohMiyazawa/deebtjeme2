#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include <cmath>
#include <math.h>

#include "file_io.hpp"
#include "lode_io.hpp"

void print_usage(){
	printf("./colour_cache infile.png cacheSize blockWidth\n");
}

int main(int argc, char *argv[]){
	if(argc < 4){
		printf("not enough arguments\n");
		print_usage();
		return 1;
	}

	unsigned width = 0, height = 0;

	uint8_t* decoded = decodeOneStep(argv[1],&width,&height);

	size_t cache_bits = atoi(argv[2]);
	size_t cache_size = 1 << cache_bits;
	size_t block_size = atoi(argv[3]);
	if(block_size == 0){
		block_size = width;
	}
	size_t num_block = (width + block_size - 1) / block_size;
	uint32_t cache[cache_size*num_block];
	uint32_t hitlist[cache_size];
	for(size_t i=0;i<cache_size*num_block;i++){
		cache[i] = 0;
	}
	for(size_t i=0;i<cache_size;i++){
		hitlist[i] = 0;
	}
	uint8_t defaults[64] = {
		0,0,0,255,
		255,255,255,255,
		255,0,0,255,
		0,255,0,255,
		0,0,255,255,
		255,255,0,255,
		255,0,255,255,
		0,255,255,255
	};
	for(size_t bl=0;bl<num_block;bl++){
		for(size_t i=0;i<8;i++){
			uint32_t argb_pixel = (defaults[i*4] << 24) + (defaults[i*4 + 1] << 16) + (defaults[i*4 + 2] << 8) + defaults[i*4 + 3];
			size_t index = (0x1e35a7bd * argb_pixel) >> (32 - cache_bits);
			cache[index + bl*cache_size] = argb_pixel;
		}
	}

	size_t hits = 0;

	for(size_t i=0;i<width*height;i++){
		size_t bl = (i % width)/block_size;
		uint32_t argb_pixel = (decoded[i*4] << 24) + (decoded[i*4 + 1] << 16) + (decoded[i*4 + 2] << 8) + decoded[i*4 + 3];
		size_t index = (0x1e35a7bd * argb_pixel) >> (32 - cache_bits);
		if(cache[index + bl*cache_size] == argb_pixel){
			hits++;
			hitlist[index]++;
		}
		else{
			cache[index + bl*cache_size] = argb_pixel;
		}
	}
	double percentage = (double)hits*100/(width*height);
	printf("hits: %d, %f%% (%d %d (%d))\n",(int)hits,percentage,(int)cache_size,(int)block_size,(int)num_block);
	for(size_t i=0;i<cache_size;i++){
		printf("%d,",(int)hitlist[i]);
	}
	delete[] decoded;

	return 0;
}
