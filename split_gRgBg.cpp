#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include <cmath>
#include <vector>
#include <math.h>

#include "file_io.hpp"
#include "lode_io.hpp"
#include "colour_transform.hpp"

void print_usage(){
	printf("./split_gRgBg infile.png\n");
}

int main(int argc, char *argv[]){
	if(argc < 2){
		printf("not enough arguments\n");
		print_usage();
		return 1;
	}

	unsigned width = 0, height = 0;
	printf("reading png\n");
	uint8_t* decoded = decodeOneStep(argv[1],&width,&height);
	printf("width : %d\n",(int)width);
	printf("height: %d\n",(int)height);

	image_3ch_8bit rgb = lode_to_gRgBg(decoded,width,height);
	delete[] decoded;

	std::vector<unsigned char> image;
	image.resize(width * height * 4);
	for(size_t i=0;i<width*height;i++){
		image[i*4]   = rgb.pixels[i*3];
		image[i*4+1] = rgb.pixels[i*3];
		image[i*4+2] = rgb.pixels[i*3];
		image[i*4+3] = 255;
	}
	char filename[sizeof(argv[1]) + 6];
	strcpy(filename, argv[1]);
	strcat(filename, "_G.png");

	encodeOneStep(filename, image, width, height);

	for(size_t i=0;i<width*height;i++){
		image[i*4]   = rgb.pixels[i*3+1];
		image[i*4+1] = rgb.pixels[i*3+1];
		image[i*4+2] = rgb.pixels[i*3+1];
	}
	char filename2[sizeof(argv[1]) + 7];
	strcpy(filename2, argv[1]);
	strcat(filename2, "_Rg.png");
	encodeOneStep(filename2, image, width, height);
	for(size_t i=0;i<width*height;i++){
		image[i*4]   = rgb.pixels[i*3+2];
		image[i*4+1] = rgb.pixels[i*3+2];
		image[i*4+2] = rgb.pixels[i*3+2];
	}
	strcpy(filename2, argv[1]);
	strcat(filename2, "_Bg.png");
	encodeOneStep(filename2, image, width, height);


	return 0;
}
