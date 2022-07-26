#ifndef COLOUR_FILTERS_HEADER
#define COLOUR_FILTERS_HEADER

#include "numerics.hpp"
#include "image_structs.hpp"
#include "delta_colour.hpp"
#include "filter_utils.hpp"
#include "colour_filter_utils.hpp"

image_3ch_8bit filter_all_3ch_left(image_3ch_8bit& rgb,uint32_t range){
	image_3ch_8bit filtered;
	filtered.header = rgb.header;
	filtered.pixels = new uint8_t[filtered.header.width*filtered.header.height*3];
	uint32_t width = rgb.header.width;

	for(size_t i=1;i<rgb.header.width;i++){
		filtered.pixels[i*3  ] = sub_mod(rgb.pixels[i*3  ],rgb.pixels[(i - 1)*3  ],range);
		filtered.pixels[i*3+1] = sub_mod(rgb.pixels[i*3+1],rgb.pixels[(i - 1)*3+1],range);
		filtered.pixels[i*3+2] = sub_mod(rgb.pixels[i*3+2],rgb.pixels[(i - 1)*3+2],range);
	}
	for(size_t y=1;y<rgb.header.height;y++){
		filtered.pixels[y*width*3    ] = sub_mod(rgb.pixels[(y*width)*3  ],rgb.pixels[((y-1)*width)*3  ],range);
		filtered.pixels[y*width*3 + 1] = sub_mod(rgb.pixels[(y*width)*3+1],rgb.pixels[((y-1)*width)*3+1],range);
		filtered.pixels[y*width*3 + 2] = sub_mod(rgb.pixels[(y*width)*3+2],rgb.pixels[((y-1)*width)*3+2],range);
		for(size_t i=1;i<rgb.header.width;i++){
			filtered.pixels[(i + y*width)*3  ] = sub_mod(rgb.pixels[(i + y*width)*3  ],rgb.pixels[(i - 1 + y*width)*3  ],range);
			filtered.pixels[(i + y*width)*3+1] = sub_mod(rgb.pixels[(i + y*width)*3+1],rgb.pixels[(i - 1 + y*width)*3+1],range);
			filtered.pixels[(i + y*width)*3+2] = sub_mod(rgb.pixels[(i + y*width)*3+2],rgb.pixels[(i - 1 + y*width)*3+2],range);
		}
	}
	return filtered;
}

image_3ch_8bit filter_all_3ch_ffv1(image_3ch_8bit& rgb,uint32_t range){
	image_3ch_8bit filtered;
	filtered.header = rgb.header;
	filtered.pixels = new uint8_t[filtered.header.width*filtered.header.height*3];
	uint32_t width = rgb.header.width;
	//printf("dim filt %d %d\n",(int)rgb.header.width,(int)rgb.header.height);

	for(size_t i=1;i<width;i++){
		filtered.pixels[i*3  ] = sub_mod(rgb.pixels[i*3  ],rgb.pixels[(i - 1)*3  ],range);
		filtered.pixels[i*3+1] = sub_mod(rgb.pixels[i*3+1],rgb.pixels[(i - 1)*3+1],range);
		filtered.pixels[i*3+2] = sub_mod(rgb.pixels[i*3+2],rgb.pixels[(i - 1)*3+2],range);
	}
	for(size_t y=1;y<rgb.header.height;y++){
		filtered.pixels[y*width*3    ] = sub_mod(rgb.pixels[(y*width)*3  ],rgb.pixels[((y-1)*width)*3  ],range);
		filtered.pixels[y*width*3 + 1] = sub_mod(rgb.pixels[(y*width)*3+1],rgb.pixels[((y-1)*width)*3+1],range);
		filtered.pixels[y*width*3 + 2] = sub_mod(rgb.pixels[(y*width)*3+2],rgb.pixels[((y-1)*width)*3+2],range);
		for(size_t i=1;i<rgb.header.width;i++){
			uint8_t L  = rgb.pixels[(i - 1 + y*width)*3  ];
			uint8_t T  = rgb.pixels[(i + (y-1)*width)*3  ];
			uint8_t TL = rgb.pixels[(i - 1 + (y-1)*width)*3  ];
			filtered.pixels[(i + y*width)*3  ] = sub_mod(rgb.pixels[(i + y*width)*3  ],ffv1(L,T,TL),range);
			L  = rgb.pixels[(i - 1 + y*width)*3  +1];
			T  = rgb.pixels[(i + (y-1)*width)*3  +1];
			TL = rgb.pixels[(i - 1 + (y-1)*width)*3 +1];
			filtered.pixels[(i + y*width)*3+1] = sub_mod(rgb.pixels[(i + y*width)*3+1],ffv1(L,T,TL),range);
			L  = rgb.pixels[(i - 1 + y*width)*3  +2];
			T  = rgb.pixels[(i + (y-1)*width)*3  +2];
			TL = rgb.pixels[(i - 1 + (y-1)*width)*3 +2];
			filtered.pixels[(i + y*width)*3+2] = sub_mod(rgb.pixels[(i + y*width)*3+2],ffv1(L,T,TL),range);
		}
	}
	//printf("Done filtering\n");
	return filtered;
}

image_1ch_8bit filter_all_1ch_ffv1(image_1ch_8bit& grey,uint32_t range){
	image_1ch_8bit filtered;
	filtered.header = grey.header;
	filtered.pixels = new uint8_t[filtered.header.width*filtered.header.height];
	uint32_t width = grey.header.width;

	for(size_t i=1;i<width;i++){
		filtered.pixels[i] = sub_mod(grey.pixels[i],grey.pixels[i - 1],range);
	}
	for(size_t y=1;y<grey.header.height;y++){
		filtered.pixels[y*width] = sub_mod(grey.pixels[y*width],grey.pixels[(y-1)*width],range);
		for(size_t i=1;i<grey.header.width;i++){
			uint8_t L  = grey.pixels[i - 1 + y*width];
			uint8_t T  = grey.pixels[i + (y-1)*width];
			uint8_t TL = grey.pixels[i - 1 + (y-1)*width];
			filtered.pixels[i + y*width] = sub_mod(grey.pixels[i + y*width],ffv1(L,T,TL),range);
		}
	}
	//printf("Done filtering\n");
	return filtered;
}

uint8_t* colour_filter_all_left(uint8_t* in_bytes, uint32_t range, uint32_t width, uint32_t height){
	uint8_t* filtered = new uint8_t[width * height * 3];

	filtered[0] = in_bytes[0];
	filtered[1] = in_bytes[1];
	filtered[2] = in_bytes[2];

	for(size_t i=1;i<width;i++){
		filtered[i*3] = sub_mod(in_bytes[i*3],in_bytes[(i - 1)*3],range);
		filtered[i*3 + 1] = sub_mod(in_bytes[i*3 + 1],in_bytes[(i - 1)*3 + 1],range);
		filtered[i*3 + 2] = sub_mod(in_bytes[i*3 + 2],in_bytes[(i - 1)*3 + 2],range);
	}

	for(size_t y=1;y<height;y++){
		filtered[y*width*3] = sub_mod(in_bytes[y*width*3],in_bytes[(y - 1)*width*3],range);
		filtered[y*width*3 + 1] = sub_mod(in_bytes[y*width*3 + 1],in_bytes[(y - 1)*width*3 + 1],range);
		filtered[y*width*3 + 2] = sub_mod(in_bytes[y*width*3 + 2],in_bytes[(y - 1)*width*3 + 2],range);
		for(size_t i=1;i<width;i++){
			uint8_t L  = in_bytes[(y * width + i - 1)*3];
			filtered[((y * width) + i)*3] = sub_mod(
				in_bytes[((y * width) + i)*3],
				L,
				range
			);
			L  = in_bytes[(y * width + i - 1)*3 + 1];
			filtered[((y * width) + i)*3 + 1] = sub_mod(
				in_bytes[((y * width) + i)*3 + 1],
				L,
				range
			);
			L  = in_bytes[(y * width + i - 1)*3 + 2];
			filtered[((y * width) + i)*3 + 2] = sub_mod(
				in_bytes[((y * width) + i)*3 + 2],
				L,
				range
			);
		}
	}

	return filtered;
}

uint8_t* colour_filter_all_ffv1_double(uint8_t* in_bytes, uint32_t range, uint32_t width, uint32_t height){
	uint16_t* filtered = new uint16_t[width * height * 3];

	filtered[0] = in_bytes[0] + range;
	filtered[1] = in_bytes[1] + range;
	filtered[2] = in_bytes[2] + range;

	for(size_t i=1;i<width;i++){
		filtered[i*3] = in_bytes[i*3] + range - in_bytes[(i - 1)*3];
		filtered[i*3 + 1] = in_bytes[i*3 + 1] + range - in_bytes[(i - 1)*3 + 1];
		filtered[i*3 + 2] = in_bytes[i*3 + 2] + range - in_bytes[(i - 1)*3 + 2];
	}

	for(size_t y=1;y<height;y++){
		filtered[y*width*3] = in_bytes[y*width*3] + range - in_bytes[(y - 1)*width*3];
		filtered[y*width*3 + 1] = in_bytes[y*width*3 + 1] + range - in_bytes[(y - 1)*width*3 + 1];
		filtered[y*width*3 + 2] = in_bytes[y*width*3 + 2] + range - in_bytes[(y - 1)*width*3 + 2];
		for(size_t i=1;i<width;i++){
			uint8_t L  = in_bytes[(y * width + i - 1)*3];
			uint8_t T  = in_bytes[((y-1) * width + i)*3];
			uint8_t TL = in_bytes[((y-1) * width + i - 1)*3];
			filtered[((y * width) + i)*3] = range + in_bytes[((y * width) + i)*3] - ffv1(
					L,
					T,
					TL
				);
			L  = in_bytes[(y * width + i - 1)*3 + 1];
			T  = in_bytes[((y-1) * width + i)*3 + 1];
			TL = in_bytes[((y-1) * width + i - 1)*3 + 1];
			filtered[((y * width) + i)*3 + 1] = range + in_bytes[((y * width) + i)*3 + 1] - ffv1(
					L,
					T,
					TL
				);
			L  = in_bytes[(y * width + i - 1)*3 + 2];
			T  = in_bytes[((y-1) * width + i)*3 + 2];
			TL = in_bytes[((y-1) * width + i - 1)*3 + 2];
			filtered[((y * width) + i)*3 + 2] = range + in_bytes[((y * width) + i)*3 + 2] - ffv1(
					L,
					T,
					TL
				);
		}
	}

	uint8_t* filtered2 = new uint8_t[width * height * 3];

	filtered2[0] = filtered[0] % range;
	filtered2[1] = filtered[1] % range;
	filtered2[2] = filtered[2] % range;

	for(size_t i=1;i<width;i++){
		filtered2[i*3] = (filtered[i*3] + 2*range - filtered[(i - 1)*3]) % range;
		filtered2[i*3 + 1] = (filtered[i*3 + 1] + 2*range - filtered[(i - 1)*3 + 1]) % range;
		filtered2[i*3 + 2] = (filtered[i*3 + 2] + 2*range - filtered[(i - 1)*3 + 2]) % range;
	}

	for(size_t y=1;y<height;y++){
		filtered2[y*width*3] = (filtered[y*width*3] + 2*range - filtered[(y - 1)*width*3]) % range;
		filtered2[y*width*3 + 1] = (filtered[y*width*3 + 1] + 2*range - filtered[(y - 1)*width*3 + 1]) % range;
		filtered2[y*width*3 + 2] = (filtered[y*width*3 + 2] + 2*range - filtered[(y - 1)*width*3 + 2]) % range;
		for(size_t i=1;i<width;i++){
			uint16_t L  = filtered[(y * width + i - 1)*3];
			uint16_t T  = filtered[((y-1) * width + i)*3];
			uint16_t TL = filtered[((y-1) * width + i - 1)*3];
/*
			filtered2[((y * width) + i)*3] = (2*range + filtered[((y * width) + i)*3] - ffv1(
					L,
					T,
					TL
				)) % range;
*/
			filtered2[((y * width) + i)*3] = (2*range + filtered[((y * width) + i)*3] - L) % range;
			L  = filtered[(y * width + i - 1)*3 + 1];
			T  = filtered[((y-1) * width + i)*3 + 1];
			TL = filtered[((y-1) * width + i - 1)*3 + 1];
/*			filtered2[((y * width) + i)*3 + 1] = (2*range + filtered[((y * width) + i)*3 + 1] - ffv1(
					L,
					T,
					TL
				)) % range;*/
			filtered2[((y * width) + i)*3 + 1] = (2*range + filtered[((y * width) + i)*3 + 1] - L) % range;
			L  = filtered[(y * width + i - 1)*3 + 2];
			T  = filtered[((y-1) * width + i)*3 + 2];
			TL = filtered[((y-1) * width + i - 1)*3 + 2];
/*			filtered2[((y * width) + i)*3 + 2] = (2*range + filtered[((y * width) + i)*3 + 2] - ffv1(
					L,
					T,
					TL
				)) % range;*/
			filtered2[((y * width) + i)*3 + 2] = (2*range + filtered[((y * width) + i)*3 + 2] - L) % range;
		}
	}
	delete[] filtered;

	return filtered2;
}

uint8_t* colour_filter_all_ffv1(uint8_t* in_bytes, uint32_t range, uint32_t width, uint32_t height){
	uint8_t* filtered = new uint8_t[width * height * 3];

	filtered[0] = in_bytes[0];
	filtered[1] = in_bytes[1];
	filtered[2] = in_bytes[2];

	for(size_t i=1;i<width;i++){
		filtered[i*3] = sub_mod(in_bytes[i*3],in_bytes[(i - 1)*3],range);
		filtered[i*3 + 1] = sub_mod(in_bytes[i*3 + 1],in_bytes[(i - 1)*3 + 1],range);
		filtered[i*3 + 2] = sub_mod(in_bytes[i*3 + 2],in_bytes[(i - 1)*3 + 2],range);
	}

	for(size_t y=1;y<height;y++){
		filtered[y*width*3] = sub_mod(in_bytes[y*width*3],in_bytes[(y - 1)*width*3],range);
		filtered[y*width*3 + 1] = sub_mod(in_bytes[y*width*3 + 1],in_bytes[(y - 1)*width*3 + 1],range);
		filtered[y*width*3 + 2] = sub_mod(in_bytes[y*width*3 + 2],in_bytes[(y - 1)*width*3 + 2],range);
		for(size_t i=1;i<width;i++){
			uint8_t L  = in_bytes[(y * width + i - 1)*3];
			uint8_t T  = in_bytes[((y-1) * width + i)*3];
			uint8_t TL = in_bytes[((y-1) * width + i - 1)*3];
			filtered[((y * width) + i)*3] = sub_mod(
				in_bytes[((y * width) + i)*3],
				ffv1(
					L,
					T,
					TL
				),
				range
			);
			L  = in_bytes[(y * width + i - 1)*3 + 1];
			T  = in_bytes[((y-1) * width + i)*3 + 1];
			TL = in_bytes[((y-1) * width + i - 1)*3 + 1];
			filtered[((y * width) + i)*3 + 1] = sub_mod(
				in_bytes[((y * width) + i)*3 + 1],
				ffv1(
					L,
					T,
					TL
				),
				range
			);
			L  = in_bytes[(y * width + i - 1)*3 + 2];
			T  = in_bytes[((y-1) * width + i)*3 + 2];
			TL = in_bytes[((y-1) * width + i - 1)*3 + 2];
			filtered[((y * width) + i)*3 + 2] = sub_mod(
				in_bytes[((y * width) + i)*3 + 2],
				ffv1(
					L,
					T,
					TL
				),
				range
			);
		}
	}

	return filtered;
}

uint8_t* hbd_filter_all_ffv1(uint16_t* in_bytes, uint32_t range, uint32_t width, uint32_t height){
	uint8_t* filtered = new uint8_t[width * height];

	filtered[0] = in_bytes[0];

	for(size_t i=1;i<width;i++){
		filtered[i] = sub_mod(in_bytes[i],in_bytes[i - 1],range);
	}

	for(size_t y=1;y<height;y++){
		filtered[y*width] = sub_mod(in_bytes[y*width],in_bytes[(y - 1)*width],range);
		for(size_t i=1;i<width;i++){
			uint16_t L  = in_bytes[(y * width + i - 1)];
			uint16_t T  = in_bytes[((y-1) * width + i)];
			uint16_t TL = in_bytes[((y-1) * width + i - 1)];
			filtered[(y * width) + i] = sub_mod(
				in_bytes[(y * width) + i],
				ffv1(
					L,
					T,
					TL
				),
				range
			);
		}
	}

	return filtered;
}

uint8_t* colour_filter_all_ffv1_subColour(uint8_t* in_bytes, uint16_t* rsub, uint16_t* bsub, uint32_t range, uint32_t width, uint32_t height){
	uint8_t* filtered = new uint8_t[width * height * 3];

	filtered[0] = in_bytes[0];
	filtered[1] = rsub[0] % range;
	filtered[2] = bsub[0] % range;


	for(size_t i=0;i<width*height;i++){
		filtered[i*3] = 0;
		filtered[i*3+1] = 0;
		filtered[i*3+2] = 0;
	}

	for(size_t i=1;i<width;i++){
		filtered[i*3] = sub_mod(in_bytes[i*3],in_bytes[(i - 1)*3],range);
		filtered[i*3 + 1] = sub_mod(rsub[i],rsub[i-1],range);
		filtered[i*3 + 2] = sub_mod(bsub[i],bsub[i-1],range);
	}


	for(size_t y=1;y<height;y++){
		filtered[y*width*3] = sub_mod(in_bytes[y*width*3],in_bytes[(y - 1)*width*3],range);
		filtered[y*width*3 + 1] = sub_mod(rsub[y*width],rsub[(y-1)*width],range);
		filtered[y*width*3 + 2] = sub_mod(bsub[y*width],bsub[(y-1)*width],range);
		for(size_t i=1;i<width;i++){
			uint8_t gL  = in_bytes[(y * width + i - 1)*3];
			uint8_t gT  = in_bytes[((y-1) * width + i)*3];
			uint8_t gTL = in_bytes[((y-1) * width + i - 1)*3];
			filtered[((y * width) + i)*3] = sub_mod(
				in_bytes[((y * width) + i)*3],
				ffv1(
					gL,
					gT,
					gTL
				),
				range
			);
			uint16_t L  = rsub[y * width + i - 1];
			uint16_t T  = rsub[(y-1) * width + i];
			uint16_t TL = rsub[(y-1) * width + i - 1];
			filtered[((y * width) + i)*3 + 1] = sub_mod(
				rsub[(y * width) + i],
				ffv1(
					L,
					T,
					TL
				),
				range
			);
			L  = bsub[y * width + i - 1];
			T  = bsub[(y-1) * width + i];
			TL = bsub[(y-1) * width + i - 1];
			filtered[((y * width) + i)*3 + 2] = sub_mod(
				bsub[(y * width) + i],
				ffv1(
					L,
					T,
					TL
				),
				range
			);
		}
	}
	return filtered;
}

uint8_t* colour_filter_all_ffv1_subGreen(uint8_t* in_bytes, uint32_t range, uint32_t width, uint32_t height){
	uint8_t* filtered = new uint8_t[width * height * 3];

	uint16_t* rsub = new uint16_t[width * height];
	uint16_t* bsub = new uint16_t[width * height];

	for(size_t i=0;i<width*height;i++){
		rsub[i] = range   + in_bytes[i*3 + 1] - delta(255,in_bytes[i*3]);
		bsub[i] = 2*range + in_bytes[i*3 + 2] - delta(255,in_bytes[i*3]);//no need to subtract red
	}

	filtered[0] = in_bytes[0];
	filtered[1] = rsub[0] % range;
	filtered[2] = bsub[0] % range;

	for(size_t i=1;i<width;i++){
		filtered[i*3] = sub_mod(in_bytes[i*3],in_bytes[(i - 1)*3],range);
		filtered[i*3 + 1] = sub_mod(rsub[i] % range,rsub[i-1] % range,range);
		filtered[i*3 + 2] = sub_mod(bsub[i] % range,bsub[i-1] % range,range);
	}

	for(size_t y=1;y<height;y++){
		filtered[y*width*3] = sub_mod(in_bytes[y*width*3],in_bytes[(y - 1)*width*3],range);
		filtered[y*width*3 + 1] = sub_mod(rsub[y*width] % range,rsub[(y-1)*width] % range,range);
		filtered[y*width*3 + 2] = sub_mod(bsub[y*width] % range,bsub[(y-1)*width] % range,range);
		for(size_t i=1;i<width;i++){
			uint8_t gL  = in_bytes[(y * width + i - 1)*3];
			uint8_t gT  = in_bytes[((y-1) * width + i)*3];
			uint8_t gTL = in_bytes[((y-1) * width + i - 1)*3];
			filtered[((y * width) + i)*3] = sub_mod(
				in_bytes[((y * width) + i)*3],
				ffv1(
					gL,
					gT,
					gTL
				),
				range
			);
			uint16_t L  = rsub[y * width + i - 1];
			uint16_t T  = rsub[(y-1) * width + i];
			uint16_t TL = rsub[(y-1) * width + i - 1];
			filtered[((y * width) + i)*3 + 1] = sub_mod(
				rsub[(y * width) + i] % range,
				ffv1(
					L,
					T,
					TL
				) % range,
				range
			);
			L  = bsub[y * width + i - 1];
			T  = bsub[(y-1) * width + i];
			TL = bsub[(y-1) * width + i - 1];
			filtered[((y * width) + i)*3 + 2] = sub_mod(
				bsub[(y * width) + i] % range,
				ffv1(
					L,
					T,
					TL
				) % range,
				range
			);
		}
	}
	delete[] rsub;
	delete[] bsub;
	return filtered;
}

uint8_t* colour_filter_all_ffv1_subColour(uint8_t* in_bytes, uint32_t range, uint32_t width, uint32_t height, uint8_t rg, uint8_t bg, uint8_t br){
	uint8_t* filtered = new uint8_t[width * height * 3];

	uint16_t* rsub = new uint16_t[width * height];
	uint16_t* bsub = new uint16_t[width * height];

	for(size_t i=0;i<width*height;i++){
		rsub[i] = range   + in_bytes[i*3 + 1] - delta(rg,in_bytes[i*3]);
		bsub[i] = 2*range + in_bytes[i*3 + 2] - delta(bg,in_bytes[i*3]) - delta(br,in_bytes[i*3+1]);
	}

	filtered[0] = in_bytes[0];
	filtered[1] = rsub[0] % range;
	filtered[2] = bsub[0] % range;

	for(size_t i=1;i<width;i++){
		filtered[i*3] = sub_mod(in_bytes[i*3],in_bytes[(i - 1)*3],range);
		filtered[i*3 + 1] = sub_mod(rsub[i] % range,rsub[i-1] % range,range);
		filtered[i*3 + 2] = sub_mod(bsub[i] % range,bsub[i-1] % range,range);
	}

	for(size_t y=1;y<height;y++){
		filtered[y*width*3] = sub_mod(in_bytes[y*width*3],in_bytes[(y - 1)*width*3],range);
		filtered[y*width*3 + 1] = sub_mod(rsub[y*width] % range,rsub[(y-1)*width] % range,range);
		filtered[y*width*3 + 2] = sub_mod(bsub[y*width] % range,bsub[(y-1)*width] % range,range);
		for(size_t i=1;i<width;i++){
			uint8_t gL  = in_bytes[(y * width + i - 1)*3];
			uint8_t gT  = in_bytes[((y-1) * width + i)*3];
			uint8_t gTL = in_bytes[((y-1) * width + i - 1)*3];
			filtered[((y * width) + i)*3] = sub_mod(
				in_bytes[((y * width) + i)*3],
				ffv1(
					gL,
					gT,
					gTL
				),
				range
			);
			uint16_t L  = rsub[y * width + i - 1];
			uint16_t T  = rsub[(y-1) * width + i];
			uint16_t TL = rsub[(y-1) * width + i - 1];
			filtered[((y * width) + i)*3 + 1] = sub_mod(
				rsub[(y * width) + i] % range,
				ffv1(
					L,
					T,
					TL
				) % range,
				range
			);
			L  = bsub[y * width + i - 1];
			T  = bsub[(y-1) * width + i];
			TL = bsub[(y-1) * width + i - 1];
			filtered[((y * width) + i)*3 + 2] = sub_mod(
				bsub[(y * width) + i] % range,
				ffv1(
					L,
					T,
					TL
				) % range,
				range
			);
		}
	}
	delete[] rsub;
	delete[] bsub;
	return filtered;
}


uint8_t* colour_filter_all_subGreen(uint8_t* in_bytes, uint32_t range, uint32_t width, uint32_t height, uint16_t predictor){
	if(predictor == 0){
		return colour_filter_all_ffv1_subGreen(in_bytes, range, width, height);
	}
	int a = (predictor & 0b1111000000000000) >> 12;
	int b = (predictor & 0b0000111100000000) >> 8;
	int c = (int)((predictor & 0b0000000011110000) >> 4) - 13;
	int d = (predictor & 0b0000000000001111);
	int sum = a + b + c + d;
	int halfsum = sum >> 1;

	uint8_t* filtered = new uint8_t[width * height * 3];

	uint16_t* rsub = new uint16_t[width * height];
	uint16_t* bsub = new uint16_t[width * height];

	for(size_t i=0;i<width*height;i++){
		rsub[i] = range   + in_bytes[i*3 + 1] - delta(255,in_bytes[i*3]);
		bsub[i] = 2*range + in_bytes[i*3 + 2] - delta(255,in_bytes[i*3]);//no need to subtract red
	}

	filtered[0] = in_bytes[0];
	filtered[1] = rsub[0] % range;
	filtered[2] = bsub[0] % range;

	for(size_t i=1;i<width;i++){
		filtered[i*3] = sub_mod(in_bytes[i*3],in_bytes[(i - 1)*3],range);
		filtered[i*3 + 1] = sub_mod(rsub[i] % range,rsub[i-1] % range,range);
		filtered[i*3 + 2] = sub_mod(bsub[i] % range,bsub[i-1] % range,range);
	}

	for(size_t y=1;y<height;y++){
		filtered[y*width*3] = sub_mod(in_bytes[y*width*3],in_bytes[(y - 1)*width*3],range);
		filtered[y*width*3 + 1] = sub_mod(rsub[y*width] % range,rsub[(y-1)*width] % range,range);
		filtered[y*width*3 + 2] = sub_mod(bsub[y*width] % range,bsub[(y-1)*width] % range,range);
		for(size_t i=1;i<width;i++){
			uint8_t gL  = in_bytes[(y * width + i - 1)*3];
			uint8_t gT  = in_bytes[((y-1) * width + i)*3];
			uint8_t gTL = in_bytes[((y-1) * width + i - 1)*3];
			uint8_t gTR = in_bytes[((y-1) * width + i + 1)*3];
			filtered[((y * width) + i)*3] = sub_mod(
				in_bytes[((y * width) + i)*3],
				clamp(
					(
						a*gL + b*gT + c*gTL + d*gTR + halfsum
					)/sum,
					range
				),
				range
			);
			uint16_t L  = rsub[y * width + i - 1];
			uint16_t T  = rsub[(y-1) * width + i];
			uint16_t TL = rsub[(y-1) * width + i - 1];
			uint16_t TR = rsub[(y-1) * width + i + 1];
			filtered[((y * width) + i)*3 + 1] = sub_mod(
				rsub[(y * width) + i] % range,
				i_clamp(
					(
						a*L + b*T + c*TL + d*TR + halfsum
					)/sum,
					0,
					2*range
				) % range,
				range
			);
			L  = bsub[y * width + i - 1];
			T  = bsub[(y-1) * width + i];
			TL = bsub[(y-1) * width + i - 1];
			TR = bsub[(y-1) * width + i + 1];
			filtered[((y * width) + i)*3 + 2] = sub_mod(
				bsub[(y * width) + i] % range,
				i_clamp(
					(
						a*L + b*T + c*TL + d*TR + halfsum
					)/sum,
					0,
					3*range
				) % range,
				range
			);
		}
	}
	delete[] rsub;
	delete[] bsub;
	return filtered;
}

uint8_t* colour_filter_all_subColour(uint8_t* in_bytes, uint16_t* rsub, uint16_t* bsub, uint32_t range, uint32_t width, uint32_t height, uint16_t predictor){
	if(predictor == 0){
		return colour_filter_all_ffv1_subColour(in_bytes, rsub, bsub, range, width, height);
	}
	int a = (predictor & 0b1111000000000000) >> 12;
	int b = (predictor & 0b0000111100000000) >> 8;
	int c = (int)((predictor & 0b0000000011110000) >> 4) - 13;
	int d = (predictor & 0b0000000000001111);
	int sum = a + b + c + d;
	int halfsum = sum >> 1;

	uint8_t* filtered = new uint8_t[width * height * 3];

	filtered[0] = in_bytes[0];
	filtered[1] = rsub[0] % range;
	filtered[2] = bsub[0] % range;

	for(size_t i=1;i<width;i++){
		filtered[i*3] = sub_mod(in_bytes[i*3],in_bytes[(i - 1)*3],range);
		filtered[i*3 + 1] = sub_mod(rsub[i] % range,rsub[i-1] % range,range);
		filtered[i*3 + 2] = sub_mod(bsub[i] % range,bsub[i-1] % range,range);
	}

	for(size_t y=1;y<height;y++){
		filtered[y*width*3] = sub_mod(in_bytes[y*width*3],in_bytes[(y - 1)*width*3],range);
		filtered[y*width*3 + 1] = sub_mod(rsub[y*width] % range,rsub[(y-1)*width] % range,range);
		filtered[y*width*3 + 2] = sub_mod(bsub[y*width] % range,bsub[(y-1)*width] % range,range);
		for(size_t i=1;i<width;i++){
			uint8_t gL  = in_bytes[(y * width + i - 1)*3];
			uint8_t gT  = in_bytes[((y-1) * width + i)*3];
			uint8_t gTL = in_bytes[((y-1) * width + i - 1)*3];
			uint8_t gTR = in_bytes[((y-1) * width + i + 1)*3];
			filtered[((y * width) + i)*3] = sub_mod(
				in_bytes[((y * width) + i)*3],
				clamp(
					(
						a*gL + b*gT + c*gTL + d*gTR + halfsum
					)/sum,
					range
				),
				range
			);
			uint16_t L  = rsub[y * width + i - 1];
			uint16_t T  = rsub[(y-1) * width + i];
			uint16_t TL = rsub[(y-1) * width + i - 1];
			uint16_t TR = rsub[(y-1) * width + i + 1];
			filtered[((y * width) + i)*3 + 1] = sub_mod(
				rsub[(y * width) + i] % range,
				i_clamp(
					(
						a*L + b*T + c*TL + d*TR + halfsum
					)/sum,
					0,
					2*range
				) % range,
				range
			);
			L  = bsub[y * width + i - 1];
			T  = bsub[(y-1) * width + i];
			TL = bsub[(y-1) * width + i - 1];
			TR = bsub[(y-1) * width + i + 1];
			filtered[((y * width) + i)*3 + 2] = sub_mod(
				bsub[(y * width) + i] % range,
				i_clamp(
					(
						a*L + b*T + c*TL + d*TR + halfsum
					)/sum,
					0,
					3*range
				) % range,
				range
			);
		}
	}
	return filtered;
}

uint8_t* colour_filter_all_subColour(uint8_t* in_bytes, uint32_t range, uint32_t width, uint32_t height, uint16_t predictor, uint8_t rg, uint8_t bg, uint8_t br){
	if(predictor == 0){
		return colour_filter_all_ffv1_subColour(in_bytes, range, width, height, rg, bg, br);
	}
	int a = (predictor & 0b1111000000000000) >> 12;
	int b = (predictor & 0b0000111100000000) >> 8;
	int c = (int)((predictor & 0b0000000011110000) >> 4) - 13;
	int d = (predictor & 0b0000000000001111);
	int sum = a + b + c + d;
	int halfsum = sum >> 1;

	uint8_t* filtered = new uint8_t[width * height * 3];

	uint16_t* rsub = new uint16_t[width * height];
	uint16_t* bsub = new uint16_t[width * height];

	for(size_t i=0;i<width*height;i++){
		rsub[i] = range   + in_bytes[i*3 + 1] - delta(rg,in_bytes[i*3]);
		bsub[i] = 2*range + in_bytes[i*3 + 2] - delta(bg,in_bytes[i*3]) - delta(br,in_bytes[i*3+1]);
	}

	filtered[0] = in_bytes[0];
	filtered[1] = rsub[0] % range;
	filtered[2] = bsub[0] % range;

	for(size_t i=1;i<width;i++){
		filtered[i*3] = sub_mod(in_bytes[i*3],in_bytes[(i - 1)*3],range);
		filtered[i*3 + 1] = sub_mod(rsub[i] % range,rsub[i-1] % range,range);
		filtered[i*3 + 2] = sub_mod(bsub[i] % range,bsub[i-1] % range,range);
	}

	for(size_t y=1;y<height;y++){
		filtered[y*width*3] = sub_mod(in_bytes[y*width*3],in_bytes[(y - 1)*width*3],range);
		filtered[y*width*3 + 1] = sub_mod(rsub[y*width] % range,rsub[(y-1)*width] % range,range);
		filtered[y*width*3 + 2] = sub_mod(bsub[y*width] % range,bsub[(y-1)*width] % range,range);
		for(size_t i=1;i<width;i++){
			uint8_t gL  = in_bytes[(y * width + i - 1)*3];
			uint8_t gT  = in_bytes[((y-1) * width + i)*3];
			uint8_t gTL = in_bytes[((y-1) * width + i - 1)*3];
			uint8_t gTR = in_bytes[((y-1) * width + i + 1)*3];
			filtered[((y * width) + i)*3] = sub_mod(
				in_bytes[((y * width) + i)*3],
				clamp(
					(
						a*gL + b*gT + c*gTL + d*gTR + halfsum
					)/sum,
					range
				),
				range
			);
			uint16_t L  = rsub[y * width + i - 1];
			uint16_t T  = rsub[(y-1) * width + i];
			uint16_t TL = rsub[(y-1) * width + i - 1];
			uint16_t TR = rsub[(y-1) * width + i + 1];
			filtered[((y * width) + i)*3 + 1] = sub_mod(
				rsub[(y * width) + i] % range,
				i_clamp(
					(
						a*L + b*T + c*TL + d*TR + halfsum
					)/sum,
					0,
					2*range
				) % range,
				range
			);
			L  = bsub[y * width + i - 1];
			T  = bsub[(y-1) * width + i];
			TL = bsub[(y-1) * width + i - 1];
			TR = bsub[(y-1) * width + i + 1];
			filtered[((y * width) + i)*3 + 2] = sub_mod(
				bsub[(y * width) + i] % range,
				i_clamp(
					(
						a*L + b*T + c*TL + d*TR + halfsum
					)/sum,
					0,
					3*range
				) % range,
				range
			);
		}
	}
	delete[] rsub;
	delete[] bsub;
	return filtered;
}

#endif //COLOUR_FILTERS_HEADER
