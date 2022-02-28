#pragma once

#include <cmath>
#include "image_structs.hpp"
#include "backref_table.hpp"
#include "matchlen_table.hpp"
#include "offset_table.hpp"

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
	image_3ch_8bit*& image,
	size_t hash_bits;
	double*& literal_costs,
	double*& backref_costs,
	double*& matchlen_costs,
	double*& offset_costs,
	lz_match* matches
){
	uint32_t* hashMap = new uint32_t[1 << hash_bits];
	for(size_t i=0;i<(1 << hash_bits);i++){
		hashMap[i] = 0xFFFFFFFF;
	}
	size_t img_size = image.header.width*image.header.height;
	size_t since_last = 1;
	size_t match_count = 0;
	for(size_t i=0;i<img_size;i++){
		int hash = mulHash(
			(image.pixels[i*3] << 16) + (image.pixels[i*3 + 1] << 8) + image.pixels[i*3 + 2],
			32 - hash_bits
		);
		if(hashMap[hash] != 0xFFFFFFFF){
			size_t location = hashMap[hash];
			double saved = 0;
			size_t len = 0;
			for(;len + i < img_size;len++){
				if(
					!(
						image.pixels[(i+len)*3] == image.pixels[(location+len)*3]
						&& image.pixels[(i+len)*3 + 1] == image.pixels[(location+len)*3 + 1]
						&& image.pixels[(i+len)*3 + 2] == image.pixels[(location+len)*3 + 2]
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
					//TODO: hash in values for the matched stretch
				}
			}
		}
		hashMap[hash] = i;
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
