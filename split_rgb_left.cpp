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
#include "colour_filters.hpp"

void print_usage(){
	printf("./split_rgb infile.png\n");
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

	image_3ch_8bit proto = lode_to_rgb(decoded,width,height);
	image_3ch_8bit rgb   = filter_all_3ch_left(proto, 256);
	delete[] decoded;

	std::vector<unsigned char> image;
	image.resize(width * height * 4);
	for(size_t i=0;i<width*height;i++){
		image[i*4]   = rgb.pixels[i*3];
		image[i*4+1] = rgb.pixels[i*3];
		image[i*4+2] = rgb.pixels[i*3];
		image[i*4+3] = 255;
	}
	char filename[sizeof(argv[1]) + 7];
	strcpy(filename, argv[1]);
	strcat(filename, "_R.png");

	encodeOneStep(filename, image, width, height);

	size_t sim_count = 0;
	size_t zero_count = 0;
	size_t one_count = 0;
	size_t max_count = 0;

	for(size_t i=0;i<width*height;i++){
		image[i*4]   = rgb.pixels[i*3+1];
		image[i*4+1] = rgb.pixels[i*3+1];
		image[i*4+2] = rgb.pixels[i*3+1];
		if(rgb.pixels[i*3+1] == 0){
			zero_count++;
		}
		if(rgb.pixels[i*3+1] == 1){
			one_count++;
		}
		if(rgb.pixels[i*3+1] == 255){
			max_count++;
		}
		if(i && rgb.pixels[i*3+1] != 0 && rgb.pixels[i*3+1] != 1 && rgb.pixels[i*3+1] != 255 && rgb.pixels[i*3+1] == rgb.pixels[(i-1)*3+1]){
			sim_count++;
		}
	}
	printf("sim:%d zero:%d one:%d max:%d\n",(int)sim_count,(int)zero_count,(int)one_count,(int)max_count);
	strcpy(filename, argv[1]);
	strcat(filename, "_G.png");
	encodeOneStep(filename, image, width, height);
	for(size_t i=0;i<width*height;i++){
		image[i*4]   = rgb.pixels[i*3+2];
		image[i*4+1] = rgb.pixels[i*3+2];
		image[i*4+2] = rgb.pixels[i*3+2];
	}
	strcpy(filename, argv[1]);
	strcat(filename, "_B.png");
	encodeOneStep(filename, image, width, height);


	return 0;
}
