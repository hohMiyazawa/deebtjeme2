#pragma once

#include <cmath>
#include "image_structs.hpp"
#include "hash.hpp"
#include "prefix_coding.hpp"

double code_len_cost(double* exact,size_t value){
	//use tabled values for value < 256, use log2(value) above
	//large values can't really be tabled
	if(value < 256){
		return exact[value];
	}
	return log2(value);
}

typedef struct {
	uint32_t backref;
	uint32_t matchlen;
	uint32_t offset;
} lz_match;

size_t lz_matchFinder(
	image_1ch_8bit*& image,
	size_t hash_bits,
	size_t hash_buckets,
	double*& literal_costs,
	double* backref_costs,
	double* matchlen_costs,
	double* offset_costs,
	lz_match*& matches
){
	printf("LZ_1ch\n");
	size_t hash_size = (1 << hash_bits) * hash_buckets;
	uint32_t* hashMap = new uint32_t[hash_size];
	for(size_t i=0;i<hash_size;i++){
		hashMap[i] = 0xFFFFFFFF;
	}
	size_t img_size = image->header.width*image->header.height;
	matches = new lz_match[img_size];
	size_t since_last = 1;
	size_t match_count = 0;
	double total = 0;
	size_t finds = 0;
	for(size_t i=0;i<img_size;i++){
		int hash = mulHash(
			image->pixels[i],
			32 - hash_bits
		);
		for(size_t off = 0;off<hash_buckets;off++){
			bool found = false;
			if(hashMap[hash * hash_buckets + off] != 0xFFFFFFFF){
				size_t location = hashMap[hash * hash_buckets + off];
				double saved = 0;
				size_t len = 0;
				for(;len + i < img_size;len++){
					if(
						!(
							image->pixels[i+len] == image->pixels[location+len]
						)
					){
						break;
					}
					else{
						saved += literal_costs[i+len];
					}
				}
				if(len){
					finds++;
					double net = saved
						- code_len_cost(backref_costs,i - location - 1)
						- code_len_cost(matchlen_costs,len - 1)
						- code_len_cost(offset_costs,since_last - 1);
					if(net > 0){
						total += net;
						found = true;
						matches[match_count].backref = i - location - 1;
						matches[match_count].matchlen = len - 1;
						matches[match_count].offset = since_last - 1;
						match_count++;

						for(size_t j=1;j<len;j++){
							int inter_hash = mulHash(
								image->pixels[i+j],
								32 - hash_bits
							);
							for(size_t off = hash_buckets - 1;off--;){
								hashMap[inter_hash * hash_buckets + off + 1] = hashMap[inter_hash * hash_buckets + off];
							}
							hashMap[inter_hash * hash_buckets] = i+j;
						}
						i += len - 1;
					}
				}
			}
			if(found){
				break;
			}
		}
		for(size_t off = hash_buckets - 1;off--;){
			hashMap[hash * hash_buckets + off + 1] = hashMap[hash * hash_buckets + off];
		}
		hashMap[hash * hash_buckets] = i;
	}
	printf("LZ_1ch total %f %d %d\n",total,(int)finds,(int)match_count);
	delete hashMap;
	return match_count;
}

size_t lz_matchFinder(
	image_3ch_8bit*& image,
	size_t hash_bits,
	size_t hash_buckets,
	double*& literal_costs,
	double* backref_costs,
	double* matchlen_costs,
	double* offset_costs,
	lz_match*& matches
){
	size_t hash_size = (1 << hash_bits) * hash_buckets;
	uint32_t* hashMap = new uint32_t[hash_size];
	for(size_t i=0;i<hash_size;i++){
		hashMap[i] = 0xFFFFFFFF;
	}
	size_t img_size = image->header.width*image->header.height;
	size_t since_last = 1;
	size_t match_count = 0;
	for(size_t i=0;i<img_size;i++){
		int hash = mulHash(
			(image->pixels[i*3] << 16) + (image->pixels[i*3 + 1] << 8) + image->pixels[i*3 + 2],
			32 - hash_bits
		);
		if(hashMap[hash * hash_buckets] != 0xFFFFFFFF){
			size_t location = hashMap[hash * hash_buckets];
			double saved = 0;
			size_t len = 0;
			for(;len + i < img_size;len++){
				if(
					!(
						image->pixels[(i+len)*3] == image->pixels[(location+len)*3]
						&& image->pixels[(i+len)*3 + 1] == image->pixels[(location+len)*3 + 1]
						&& image->pixels[(i+len)*3 + 2] == image->pixels[(location+len)*3 + 2]
					)
				){
					break;
				}
				else{
					saved += literal_costs[i+len];
				}
			}
			if(len){
				double net = saved
					- code_len_cost(backref_costs,i - location - 1)
					- code_len_cost(matchlen_costs,len - 1)
					- code_len_cost(offset_costs,since_last - 1);
				if(net > 0){
					matches[match_count].backref = i - location - 1;
					matches[match_count].matchlen = len - 1;
					matches[match_count].offset = since_last - 1;
					match_count++;
					for(size_t j=1;j<len;j++){
						int inter_hash = mulHash(
							(image->pixels[(i+j)*3] << 16) + (image->pixels[(i+j)*3 + 1] << 8) + image->pixels[(i+j)*3 + 2],
							32 - hash_bits
						);
						hashMap[inter_hash * hash_buckets] = i+j;
					}
				}
			}
		}
		hashMap[hash * hash_buckets] = i;
	}
	delete hashMap;
	return match_count;
}

/*merging theory

Assumptions:
- longer backrefs are more expensive
	(this is not strictly true, but often the case)
- We always want the longest possible match

Thus, a list can be constructed for each location, sorted by increasing backref and increasing matchlength.

Case 0: isolated match

	....aaaaaa....

	Keep if |lit| - |back| - |len| - |off| + |off_0| - |(off_0 - len - off)|

Case 1: contained match

	....aaaaaaa....
	......bb.......

	only makes sense to keep one. Calculate net win separately according to case 0, and then keep either or none.

Case 2: overlapping match

	...aaaaaaa.....
	......bbbbbb...

	Find a split point? Prefix/suffix gain?

	Possible ways to code:
		...............

		...aaaaaaa.....

		......bbbbbb...

		...aaa.........
		......bbbbbb...

		...aaaaaaa.....
		..........bb...

		middle split?
		moving the middle split:
			increases a_len by one
			decreases b_len by one

		in general, the gradient for large values is less than for small values, so lengths should be evened out
			
*/
