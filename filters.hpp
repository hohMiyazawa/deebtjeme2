#ifndef FILTERS_HEADER
#define FILTERS_HEADER

struct image_grey_8bit{
	header header;
	uint8_t* pixels;
}

struct image_grey_16bit{
	header header;
	uint16_t* pixels;
}

struct image_rgb_8bit{
	header header;
	uint8_t* pixels;
}

struct image_rgb_16bit{
	header header;
	uint16_t* pixels;
}

#endif //FILTERS
