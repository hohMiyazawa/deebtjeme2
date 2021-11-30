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

void print_usage(){
	printf("./motion_vector frame1.png frame2.png output.png block_size max_range sub_pel\n");
}

void motion_vector(uint8_t* first,uint8_t* second,uint8_t* buffer,unsigned width,unsigned height,unsigned block_size,int max_range,unsigned sub_pixel_steps){
	for(size_t y = 0;y<(height + block_size - 1)/block_size;y++){
		for(size_t x = 0;x<(width + block_size - 1)/block_size;x++){
			size_t block_width = block_size;
			if(width - x*block_size < block_size){
				block_width = width % block_size;
			}
			size_t block_height = block_size;
			if(height - y*block_size < block_size){
				block_height = height % block_size;
			}

			double best_error = 0;
			double error = 0;
			for(size_t y2=0;y2<block_height;y2++){
				for(size_t x2=0;x2<block_width;x2++){
					//size_t i=1;
					for(size_t i=0;i<4;i++){
						double diff = first[((block_height*y + y2)*width + x*block_width + x2)*4 + i]
							- second[((block_height*y + y2)*width + x*block_width + x2)*4 + i];
						error += diff*diff;
					}
				}
			}
			error = error/(block_height*block_width);
			best_error = error;
			int best_x_offset = 0;
			int best_y_offset = 0;
			int best_x_offset_sub_pel = 0;
			int best_y_offset_sub_pel = 0;

			for(int y_offset = -max_range;y_offset < max_range;y_offset++){
				for(int x_offset = -max_range;x_offset < max_range;x_offset++){
					for(size_t sub_y = 0;sub_y<sub_pixel_steps;sub_y++){
					for(size_t sub_x = 0;sub_x<sub_pixel_steps;sub_x++){
					if(x_offset == 0 && y_offset == 0 && sub_x == 0 && sub_y == 0){
						continue;
					}
					int signed_sub_x = 0;
					int signed_sub_y = 0;
					if(sub_x){
						signed_sub_x = 1;
					}
					if(sub_y){
						signed_sub_y = 1;
					}
					if(
						x*block_size + x_offset < 0
						|| x*block_size + block_width + x_offset + signed_sub_x > width
						|| y*block_size + y_offset < 0
						|| y*block_size + block_height + y_offset + signed_sub_y > height
					){
						continue;
					}
					error = 0;
					for(size_t y2=0;y2<block_height;y2++){
						for(size_t x2=0;x2<block_width;x2++){
							for(size_t i=0;i<4;i++){
								double diff = first[((block_height*y + y2)*width + x*block_width + x2)*4 + i];
								if(sub_x == 0){
									if(sub_y == 0){
										diff -= second[((block_height*y + y2 + y_offset)*width + x*block_width + x2 + x_offset)*4 + i];
									}
									else{
										diff -= (
											(sub_pixel_steps - sub_y) * second[((block_height*y + y2 + y_offset)*width + x*block_width + x2 + x_offset)*4 + i]
											+ sub_y * second[((block_height*y + y2 + y_offset + 1)*width + x*block_width + x2 + x_offset)*4 + i]
										)/sub_pixel_steps;
									}
								}
								else{
									if(sub_y == 0){
										diff -= (
											(sub_pixel_steps - sub_x) * second[((block_height*y + y2 + y_offset)*width + x*block_width + x2 + x_offset)*4 + i]
											+ sub_x * second[((block_height*y + y2 + y_offset)*width + x*block_width + x2 + x_offset + 1)*4 + i]
										)/sub_pixel_steps;
									}
									else{
										diff -= (
											(2*sub_pixel_steps - sub_x - sub_y) * second[((block_height*y + y2 + y_offset)*width + x*block_width + x2 + x_offset)*4 + i]
											+ (sub_pixel_steps - sub_y + sub_x) * second[((block_height*y + y2 + y_offset)*width + x*block_width + x2 + x_offset + 1)*4 + i]
											+ (sub_pixel_steps - sub_x + sub_y) * second[((block_height*y + y2 + y_offset + 1)*width + x*block_width + x2 + x_offset)*4 + i]
											+ (sub_x + sub_y) * second[((block_height*y + y2 + y_offset + 1)*width + x*block_width + x2 + x_offset + 1)*4 + i]
										)/(4*sub_pixel_steps);
									}
								}
								error += diff*diff;
							}
						}
					}
					error = error/(block_height*block_width);
					if(error < best_error){
						best_x_offset = x_offset;
						best_y_offset = y_offset;
						best_x_offset_sub_pel = sub_x;
						best_y_offset_sub_pel = sub_y;
						best_error = error;
					}
					}
					}
				}
			}

			for(size_t y2=0;y2<block_height;y2++){
				for(size_t x2=0;x2<block_width;x2++){
					for(size_t i=0;i<4;i++){
						size_t coord = ((block_height*y + y2)*width + x*block_width + x2)*4 + i;
						if(best_x_offset_sub_pel == 0){
							if(best_y_offset_sub_pel == 0){
								buffer[coord] = second[((block_height*y + y2 + best_y_offset)*width + x*block_width + x2 + best_x_offset)*4 + i];
							}
							else{
								buffer[coord] = (
									(sub_pixel_steps - best_y_offset_sub_pel) * second[((block_height*y + y2 + best_y_offset)*width + x*block_width + x2 + best_x_offset)*4 + i]
									+ best_y_offset_sub_pel * second[((block_height*y + y2 + best_y_offset + 1)*width + x*block_width + x2 + best_x_offset)*4 + i]
								)/sub_pixel_steps;
							}
						}
						else{
							if(best_y_offset_sub_pel == 0){
								buffer[coord] = (
									(sub_pixel_steps - best_x_offset_sub_pel) * second[((block_height*y + y2 + best_y_offset)*width + x*block_width + x2 + best_x_offset)*4 + i]
									+ best_x_offset_sub_pel * second[((block_height*y + y2 + best_y_offset)*width + x*block_width + x2 + best_x_offset + 1)*4 + i]
								)/sub_pixel_steps;
							}
							else{
								buffer[coord] = (
									(2*sub_pixel_steps - best_x_offset_sub_pel - best_y_offset_sub_pel) * second[((block_height*y + y2 + best_y_offset)*width + x*block_width + x2 + best_x_offset)*4 + i]
									+ (sub_pixel_steps - best_y_offset_sub_pel + best_x_offset_sub_pel) * second[((block_height*y + y2 + best_y_offset)*width + x*block_width + x2 + best_x_offset + 1)*4 + i]
									+ (sub_pixel_steps - best_x_offset_sub_pel + best_y_offset_sub_pel) * second[((block_height*y + y2 + best_y_offset + 1)*width + x*block_width + x2 + best_x_offset)*4 + i]
									+ (best_x_offset_sub_pel + best_y_offset_sub_pel) * second[((block_height*y + y2 + best_y_offset + 1)*width + x*block_width + x2 + best_x_offset + 1)*4 + i]
								)/(4*sub_pixel_steps);
							}
						}
					}
				}
			}
		}
	}
}

int main(int argc, char *argv[]){
	if(argc < 6){
		printf("not enough arguments\n");
		print_usage();
		return 1;
	}

	unsigned width = 0, height = 0;
	printf("reading png\n");
	uint8_t* decoded = decodeOneStep(argv[1],&width,&height);
	printf("width : %d\n",(int)width);
	printf("height: %d\n",(int)height);

	unsigned width2 = 0, height2 = 0;
	printf("reading png\n");
	uint8_t* decoded2 = decodeOneStep(argv[2],&width2,&height2);
	if(width2 != width || height2 != height){
		printf("the frames must have the same dimensions\n");
		return 1;
	}

	uint8_t* buffer = new uint8_t[width*height*4];
	for(size_t i=0;i<width*height;i++){
		buffer[i*4+0] = decoded[i*4+0];
		buffer[i*4+1] = decoded[i*4+1];
		buffer[i*4+2] = decoded[i*4+2];
		buffer[i*4+3] = decoded[i*4+3];
	}

	delete[] decoded;
	delete[] decoded2;

	int block_size = atoi(argv[4]);
	int max_range = atoi(argv[5]);
	int sub_pel = atoi(argv[6]);

	motion_vector(decoded2,decoded,buffer,width,height,block_size,max_range,sub_pel);

	std::vector<unsigned char> image;
	image.resize(width * height * 4);
	for(size_t i=0;i<width*height;i++){
		image[i*4]   = buffer[i*4];
		image[i*4+1] = buffer[i*4+1];
		image[i*4+2] = buffer[i*4+2];
		image[i*4+3] = buffer[i*4+3];
	}
	delete[] buffer;


	encodeOneStep(argv[3], image, width, height);

	return 0;
}
