#ifndef IMAGE_STRUCTS_HEADER
#define IMAGE_STRUCTS_HEADER

#include "hoh_header.hpp"

struct image_1ch_8bit{
	HEADER header;
	uint8_t* pixels;
	~image_1ch_8bit();
};
image_1ch_8bit::~image_1ch_8bit(){delete[] pixels;}

struct image_1ch_16bit{
	HEADER header;
	uint16_t* pixels;
	~image_1ch_16bit();
};
image_1ch_16bit::~image_1ch_16bit(){delete[] pixels;}

struct image_3ch_8bit{
	HEADER header;
	uint8_t* pixels;
	~image_3ch_8bit();
};
image_3ch_8bit::~image_3ch_8bit(){delete[] pixels;}

struct image_3ch_16bit{
	HEADER header;
	uint16_t* pixels;
	~image_3ch_16bit();
};
image_3ch_16bit::~image_3ch_16bit(){delete[] pixels;}

#endif //IMAGE_STRUCTS
