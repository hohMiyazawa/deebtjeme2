#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <argp.h>

#include "panic.hpp"
#include "hoh_header.hpp"
#include "file_io.hpp"
#include "lode_io.hpp"
#include "image_structs.hpp"
#include "colour_transform.hpp"
#include "entropy_coding.hpp"
#include "encode.hpp"

const char *argp_program_version = "choh 0.0.1";
static char doc[] = "./choh infile.png -o outfile.hoh\n";
static char args_doc[] = "input.png";
static struct argp_option options[] = { 
	{ "output", 'o', "FILE.hoh", 0, "Output file path"},
	{ "speed", 's', "0-9", 0, "Speed, 0-9, higher is slower but better compression"},
	{ "length", 'l', 0, 0, "add length to header"},
	{ "checksum", 'c', 0, 0, "add checksum to header"},
	{ 0 } 
};

struct arguments{
	char* args[1];
	char* outputPath;
	bool hasOutput;
	bool hasChecksum;
	bool hasLength;
	int speed;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state){
	struct arguments *arguments =  static_cast<struct arguments*>(state->input);
	switch(key){
		case 'o':
			arguments->hasOutput = true;
			arguments->outputPath = arg;
			break;
		case 's':
			arguments->speed = atoi(arg);
			break;
		case 'l':
			arguments->hasLength = true;
			break;
		case 'c':
			arguments->hasChecksum = true;
			break;
		case ARGP_KEY_ARG:
			if(state->arg_num > 1){
				argp_usage(state);
			}
			arguments->args[state->arg_num] = arg;
			break;
		case ARGP_KEY_END:
			if(state->arg_num < 1){
				argp_usage(state);
			}
			break;
	}   
	return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc, 0, 0, 0 };

int main(int argc, char *argv[]){

	struct arguments arguments;
	arguments.hasOutput = false;
	arguments.speed = 3;

	argp_parse(&argp, argc, argv, 0, 0, &arguments);

	if(!arguments.hasOutput){
		printf("No output file specified.\n");
		return 10;
	}

	unsigned width = 0, height = 0;

	printf("reading png %s\n", arguments.args[0]);
	uint8_t* decoded = decodeOneStep(arguments.args[0],&width,&height);
	printf("width : %d\n",(int)width);
	printf("height: %d\n",(int)height);

	image_3ch_8bit rgb = lode_to_rgb(decoded,width,height);
	delete[] decoded;

	ransInfo rans;
	rans.prob_bits = 24;
	rans.prob_scale = (1 << rans.prob_bits);
	rans.buffer = 0;
	rans.buffer_size = 0;

	HEADER header;
	header.width = width;
	header.height = height;
	header.depth = 8;
	header.joined = false;
	header.hasAlpha = false;
	header.hasJFIF = false;
	header.hasFallback = false;
	header.hasProgressive = false;
	header.hasLength = false;
	header.hasChecksum = false;
	header.hasTileOffsets = false;
	header.mode = header.RGB;

	size_t header_size = headerSize(header);
	size_t pixel_bits = width*height*channelNumber(header) * header.depth;
	if(arguments.speed == 0){
		size_t max_elements = (pixel_bits + 31)/32 + header_size + 10;
		printf("pixel_bits: %d max_elements: %d\n",(int)pixel_bits,(int)max_elements);	
		uint32_t* out_buf = new uint32_t[max_elements];
		uint32_t* out_end = out_buf + max_elements;
		uint32_t* outPointer = out_end;
		rans.data = outPointer;

		printf("starting encoding\n");
		printf("encoding %d pixels\n",(int)rgb.header.width*rgb.header.height);
		//replace with encode raw later
		for(size_t i = rgb.header.width*rgb.header.height;i--;){
			writeBits(8,rgb.pixels[i*3 + 2],rans);
			writeBits(8,rgb.pixels[i*3 + 1],rans);
			writeBits(8,rgb.pixels[i*3    ],rans);
		}

		//printf("writing header\n");
		//writeHeader(header, rans.data);
		printf("writing file (%d bytes)\n",(int)((out_end - rans.data)*4));
		write_file(arguments.outputPath, (uint8_t*)rans.data, (out_end - rans.data)*4);
		delete[] out_buf;
	}
	else if(arguments.speed == 1){
		size_t max_elements = 2*((pixel_bits + 31)/32) + header_size + 10;
		printf("pixel_bits: %d max_elements: %d\n",(int)pixel_bits,(int)max_elements);	
		uint32_t* out_buf = new uint32_t[max_elements];
		uint32_t* out_end = out_buf + max_elements;
		uint32_t* outPointer = out_end;
		rans.data = outPointer;

		printf("starting encoding\n");
		printf("encoding %d pixels\n",(int)rgb.header.width*rgb.header.height);

		SymbolStats2 stats_red;
		SymbolStats2 stats_green;
		SymbolStats2 stats_blue;
		stats_red.init(256);
		stats_green.init(256);
		stats_blue.init(256);
		for(size_t i = rgb.header.width*rgb.header.height;i--;){
			stats_red.freqs[rgb.pixels[i*3]]++;
			stats_green.freqs[rgb.pixels[i*3 +1 ]]++;
			stats_blue.freqs[rgb.pixels[i*3  +2 ]]++;
		}
		Rans64EncSymbol* esyms_red = createEncodeTable_strat1(
			stats_red,
			rans
		);
		Rans64EncSymbol* esyms_green = createEncodeTable_strat1(
			stats_green,
			rans
		);
		Rans64EncSymbol* esyms_blue = createEncodeTable_strat1(
			stats_blue,
			rans
		);
		for(size_t i = rgb.header.width*rgb.header.height;i--;){
			Rans64EncPutSymbol(&rans.rans_state, &rans.data, &esyms_blue[ rgb.pixels[i*3  +2]], rans.prob_bits);
			Rans64EncPutSymbol(&rans.rans_state, &rans.data, &esyms_red[  rgb.pixels[i*3  +1]], rans.prob_bits);
			Rans64EncPutSymbol(&rans.rans_state, &rans.data, &esyms_green[rgb.pixels[i*3    ]], rans.prob_bits);
		}
		delete[] esyms_red;
		delete[] esyms_green;
		delete[] esyms_blue;
		//write tables

		//printf("writing header\n");
		//writeHeader(header, rans.data);
		printf("writing file (%d bytes)\n",(int)((out_end - rans.data)*4));
		write_file(arguments.outputPath, (uint8_t*)rans.data, (out_end - rans.data)*4);
		delete[] out_buf;
	}
	return 0;
}
