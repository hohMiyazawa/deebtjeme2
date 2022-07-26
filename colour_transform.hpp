#ifndef COLOUR_TRANSFORM_HEADER
#define COLOUR_TRANSFORM_HEADER


#include "image_structs.hpp"


void rgb_to_grb(image_3ch_8bit rgb){
	for(size_t i=0;i<rgb.header.width * rgb.header.height;i++){
		uint8_t R = rgb.pixels[i*3];
		uint8_t G = rgb.pixels[i*3 + 1];
		uint8_t B = rgb.pixels[i*3 + 2];
		rgb.pixels[i*3] = G;
		rgb.pixels[i*3 + 1] = R;
		rgb.pixels[i*3 + 2] = B;
	}
}

image_3ch_8bit rgb_to_grb_new(image_3ch_8bit rgb){
	image_3ch_8bit grb;
	grb.header = rgb.header;
	grb.pixels = new uint8_t[grb.header.width*grb.header.height*3];
	for(size_t i=0;i<rgb.header.width * rgb.header.height;i++){
		uint8_t R = rgb.pixels[i*3];
		uint8_t G = rgb.pixels[i*3 + 1];
		uint8_t B = rgb.pixels[i*3 + 2];
		grb.pixels[i*3] = G;
		grb.pixels[i*3 + 1] = R;
		grb.pixels[i*3 + 2] = B;
	}
	return grb;
}

image_1ch_8bit rgb_to_grey(image_3ch_8bit rgb){
	image_1ch_8bit grey;
	grey.header = rgb.header;
	grey.pixels = new uint8_t[grey.header.width*grey.header.height];
	for(size_t i=0;i<grey.header.width * grey.header.height;i++){
		grey.pixels[i] = rgb.pixels[i*3];
	}
	return grey;
}


image_3ch_8bit lode_to_rgb(uint8_t* decoded ,unsigned width, unsigned height){
	image_3ch_8bit rgb;
	HEADER header;
	rgb.header = header;
	rgb.header.width = width;
	rgb.header.height = height;
	rgb.pixels = new uint8_t[rgb.header.width*rgb.header.height*3];
	for(size_t i=0;i<rgb.header.width * rgb.header.height;i++){
		uint8_t R = decoded[i*4];
		uint8_t G = decoded[i*4 + 1];
		uint8_t B = decoded[i*4 + 2];
		rgb.pixels[i*3] = R;
		rgb.pixels[i*3 + 1] = G;
		rgb.pixels[i*3 + 2] = B;
	}
	return rgb;
}

image_3ch_8bit lode_to_gRgBg(uint8_t* decoded ,unsigned width, unsigned height){
	image_3ch_8bit rgb;
	rgb.header.width = width;
	rgb.header.height = height;
	rgb.pixels = new uint8_t[rgb.header.width*rgb.header.height*3];
	for(size_t i=0;i<rgb.header.width * rgb.header.height;i++){
		uint8_t R = decoded[i*4];
		uint8_t G = decoded[i*4 + 1];
		uint8_t B = decoded[i*4 + 2];
		rgb.pixels[i*3] = G;
		rgb.pixels[i*3 + 1] = R - G + 128;
		rgb.pixels[i*3 + 2] = B - G + 128;
	}
	return rgb;
}

void rgb_to_gRgBg(image_3ch_8bit& rgb){
	for(size_t i=0;i<rgb.header.width * rgb.header.height;i++){
		uint8_t R = rgb.pixels[i*3];
		uint8_t G = rgb.pixels[i*3 + 1];
		uint8_t B = rgb.pixels[i*3 + 2];
		rgb.pixels[i*3] = G;
		rgb.pixels[i*3 + 1] = R - G + 128;
		rgb.pixels[i*3 + 2] = B - G + 128;
	}
}

bool detectGreyscale(image_3ch_8bit& rgb){
	bool isGrey = true;
	for(size_t i=0;i<rgb.header.width * rgb.header.height;i++){
		uint8_t R = rgb.pixels[i*3];
		uint8_t G = rgb.pixels[i*3 + 1];
		uint8_t B = rgb.pixels[i*3 + 2];
		if(R != G || G != B){
			isGrey = false;
			break;
		}
	}
	return isGrey;
}

#endif //COLOUR_TRANSFORM_HEADER
