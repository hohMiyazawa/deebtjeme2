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

/* Outline
	1. detect pixel format
		- greyscale detection
		- opaque alpha stripping
	2. palette detection
		- simple strategy: don't bother
	3. consider colour transforms
		- simple strategy: always use subtract greeen
		- more advanced: try several?
	4. filtering
		- simple strategy: always use clampedGrad
	5. stats counting
		- simple strategy: use one for each channel
		- more advanced: divide into 8x8 blocks, sort into groups by median cuts on entropy
	6. try LZ
	7 optimisation loop
		- try 8x8 blocks of filters
		- shuffle entropy groups
		- run slower matchfinders
		- select LZ matches
	8. write data
		- literals
		- lz tokens
	9. write tables
	10. write subimages
	11. write header
	12. write file
*/

#endif
