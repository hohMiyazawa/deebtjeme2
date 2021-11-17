#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <argp.h>

#include "panic.hpp"
#include "hoh_header.hpp"
#include "file_io.hpp"

const char *argp_program_version = "dhoh 0.0.1";
static char doc[] = "./dhoh infile.hoh -o outfile.png\n";
static char args_doc[] = "input.hoh";
static struct argp_option options[] = { 
	{ "output", 'o', "FILE.png", 0, "Output file path"},
	{ "raw", 'r', 0, 0, "Output raw image data instead of PNG"},
	{ 0 } 
};

struct arguments{
	char* args[1];
	char* outputPath;
	bool hasOutput;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
	struct arguments *arguments =  static_cast<struct arguments*>(state->input);
	switch (key) {
		case 'o':
			arguments->hasOutput = true;
			arguments->outputPath = arg;
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
		default: return ARGP_ERR_UNKNOWN;
	}   
	return 0;
}

/*
uint8_t* readImage(uint32_t*& fileIndex, size_t range,uint32_t width,uint32_t height){
	return image;
}*/

void validateInfile(uint8_t* in_bytes, int in_size){
	if(in_size >= 8){
		if(
			in_bytes[0] == 0x89
			&& in_bytes[1] == 0x50
			&& in_bytes[2] == 0x4E
			&& in_bytes[3] == 0x47
			&& in_bytes[4] == 0x0D
			&& in_bytes[5] == 0x0A
			&& in_bytes[6] == 0x1A
			&& in_bytes[7] == 0x0A
		){
			panic("Input file is a PNG, not a hoh file!");
		}
	}
	if(in_size < 16 || in_size % 4 != 0){
		panic("Not a hoh file, or incomplete!");
	}
	if(
		in_bytes[0] == 0x89
		&& in_bytes[1] == 'H'
		&& in_bytes[2] == 'O'
		&& in_bytes[3] == 'H'
	){
		panic("Not a hoh file!");
	}
}

static struct argp argp = { options, parse_opt, args_doc, doc, 0, 0, 0 };

int main(int argc, char *argv[]){
	struct arguments arguments;
	arguments.hasOutput = false;

	argp_parse(&argp, argc, argv, 0, 0, &arguments);

	if(!arguments.hasOutput){
		printf("No output file specified. Decompression will be performed, but no output file written\n");
	}

	size_t in_size;
	uint8_t* in_bytes = read_file(arguments.args[0], &in_size);
	printf("read %d bytes\n",(int)in_size);

	validateInfile(in_bytes, in_size);

	delete[] in_bytes;

	uint32_t* fileIndex = (uint32_t*)in_bytes;

	header header = parseHeader(fileIndex);
/*
	fileIndex++;

	uint32_t width =  (fileIndex++) + 1;
	uint32_t height = (fileIndex++) + 1;
	printf("width : %d\n",(int)(width));
	printf("height: %d\n",(int)(height));

	uint8_t* normal = readImage(fileIndex, 256, width, height);
	delete[] in_bytes;

	std::vector<unsigned char> image;
	image.resize(width * height * 4);
	for(size_t i=0;i<width*height;i++){
		image[i*4]   = normal[i*3+1];
		image[i*4+1] = normal[i*3];
		image[i*4+2] = normal[i*3+2];
		image[i*4+3] = 255;
	}
	delete[] normal;


	encodeOneStep(argv[2], image, width, height);*/

	return 0;
}
