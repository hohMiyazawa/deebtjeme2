#ifndef ENCODE_HEADER
#define ENCODE_HEADER

#include "entropy_coding.hpp"
#include "image_structs.hpp"

void encode_raw_rgb(image_3ch_8bit rgb, ransInfo& rans){
	printf("encoding %d pixels\n",(int)rgb.header.width*rgb.header.height);
	for(size_t i=rgb.header.width*rgb.header.height;i--;){
		writeBits(8,rgb.pixels[i*3 + 2],rans);
		writeBits(8,rgb.pixels[i*3 + 1],rans);
		writeBits(8,rgb.pixels[i*3    ],rans);
	}
};

#endif
