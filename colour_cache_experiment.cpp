#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include <cmath>
#include <math.h>

#include "file_io.hpp"
#include "2dutils.hpp"
#include "lode_io.hpp"
#include "symbolstats.hpp"
#include "colour_filters.hpp"
#include "entropy_estimation.hpp"

void print_usage(){
	printf("./colour_cache_experiment infile.png cacheSize blockWidth\n");
}

int main(int argc, char *argv[]){
	if(argc < 3){
		printf("not enough arguments\n");
		print_usage();
		return 1;
	}

	unsigned width = 0, height = 0;

	printf("reading png\n");
	uint8_t* decoded = decodeOneStep(argv[1],&width,&height);
	printf("width : %d\n",(int)width);
	printf("height: %d\n",(int)height);

	size_t cache_bits = atoi(argv[2]);
	size_t cache_size = 1 << cache_bits;
	size_t block_size = atoi(argv[3]);
	if(block_size == 0){
		block_size = width;
	}
	size_t num_block = (width + block_size - 1) / block_size;
	uint32_t cache[cache_size*num_block];

	uint8_t* alpha_stripped = new uint8_t[width*height*3];
	for(size_t i=0;i<width*height;i++){
		alpha_stripped[i*3 + 0] = decoded[i*4 + 1];
		alpha_stripped[i*3 + 1] = decoded[i*4 + 0];
		alpha_stripped[i*3 + 2] = decoded[i*4 + 2];
	}

	size_t max_elements = (width*height + 4096)*3;
	uint8_t* out_buf = new uint8_t[max_elements];
	uint8_t* out_end = out_buf + max_elements;
	uint8_t* outPointer = out_end;

	uint8_t* filtered_bytes = colour_filter_all_ffv1_subGreen(alpha_stripped, 256, width, height);
	SymbolStats stats_green;
	SymbolStats stats_red;
	SymbolStats stats_blue;

	for(size_t i=0;i<256;i++){
		stats_green.freqs[i] = 0;
		stats_red.freqs[i] = 0;
		stats_blue.freqs[i] = 0;
	}
	for(size_t i=0;i<width*height*3;i+=3){
		stats_green.freqs[filtered_bytes[i]]++;
		stats_red.freqs[filtered_bytes[i+1]]++;
		stats_blue.freqs[filtered_bytes[i+2]]++;
	}

	double* green_lookup = entropyLookup(stats_green, width*height);
	double*   red_lookup = entropyLookup(stats_red, width*height);
	double*  blue_lookup = entropyLookup(stats_blue, width*height);

	/*for(size_t i=0;i<256;i++){
		printf("%f %f %f\n",green_lookup[i],red_lookup[i],blue_lookup[i]);
	}*/

	uint32_t hitlist[cache_size];
	for(size_t i=0;i<cache_size*num_block;i++){
		cache[i] = 0;
	}
	for(size_t i=0;i<cache_size;i++){
		hitlist[i] = 0;
	}

	size_t hits = 0;

	for(size_t i=0;i<width*height;i++){
		size_t bl = (i % width)/block_size;
		uint32_t argb_pixel = (decoded[i*4] << 24) + (decoded[i*4 + 1] << 16) + (decoded[i*4 + 2] << 8) + decoded[i*4 + 3];
		size_t index = (0x1e35a7bd * argb_pixel) >> (32 - cache_bits);
		if(cache[index + bl*cache_size] == argb_pixel){
			double cost = green_lookup[filtered_bytes[i*3]] + red_lookup[filtered_bytes[i*3+1]] + blue_lookup[filtered_bytes[i*3+2]];
			//printf("%f\n",cost);
			if(cost > cache_bits){
				hits++;
				hitlist[index]++;
			}
		}
		else{
			cache[index + bl*cache_size] = argb_pixel;
		}
	}
	double percentage = (double)hits/(width*height);
	printf("hits: %d, %f%% (%d %d (%d))\n",(int)hits,percentage*100,(int)cache_size,(int)block_size,(int)num_block);

	for(size_t loop=0;loop<10;loop++){
		SymbolStats cache_ind;
		for(size_t i=0;i<cache_size;i++){
			cache_ind.freqs[i] = hitlist[i];
		}
		double* lookup_cost = entropyLookup(cache_ind, hits, cache_size);

		double signalling_cost = -std::log2(percentage);
		for(size_t i=0;i<cache_size*num_block;i++){
			cache[i] = 0;
		}
		for(size_t i=0;i<cache_size;i++){
			hitlist[i] = 0;
		}

		hits = 0;
		double saved = 0;
		double total = 0;

		for(size_t i=0;i<width*height;i++){
			size_t bl = (i % width)/block_size;
			uint32_t argb_pixel = (decoded[i*4] << 24) + (decoded[i*4 + 1] << 16) + (decoded[i*4 + 2] << 8) + decoded[i*4 + 3];
			size_t index = (0x1e35a7bd * argb_pixel) >> (32 - cache_bits);
			double cost = green_lookup[filtered_bytes[i*3]] + red_lookup[filtered_bytes[i*3+1]] + blue_lookup[filtered_bytes[i*3+2]];
			if(cache[index + bl*cache_size] == argb_pixel){
				//printf("%f\n",cost);
				if(cost > /*cache_bits*/lookup_cost[index] + signalling_cost){
					saved += cost - /*cache_bits*/lookup_cost[index] - signalling_cost;
					total += lookup_cost[index] + signalling_cost;
					hits++;
					hitlist[index]++;
				}
				else{
					total += cost;
				}
			}
			else{
				cache[index + bl*cache_size] = argb_pixel;
				total += cost;
			}
		}
		percentage = (double)hits/(width*height);

		printf("hits: %d, %f%% (%f) (%d %d (%d)) %f %f\n",(int)hits,percentage*100,signalling_cost,(int)cache_size,(int)block_size,(int)num_block,saved/8,total/8);

		delete[] lookup_cost;
	}
	for(size_t i=0;i<cache_size;i++){
		printf("%d,",(int)hitlist[i]);
	}
	printf("\n");

	delete[] green_lookup;
	delete[] red_lookup;
	delete[] blue_lookup;
	delete[] alpha_stripped;
	delete[] filtered_bytes;
	delete[] decoded;

	return 0;
}
