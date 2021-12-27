#ifndef IMAGE_STRUCTS_HEADER
#define IMAGE_STRUCTS_HEADER

#include "hoh_header.hpp"

struct image_1ch_8bit{
	header header;
	uint8_t* pixels;
}

struct image_1ch_16bit{
	header header;
	uint16_t* pixels;
}

struct image_3ch_8bit{
	header header;
	uint8_t* pixels;
}

struct image_3ch_16bit{
	header header;
	uint16_t* pixels;
}

#endif //IMAGE_STRUCTS
