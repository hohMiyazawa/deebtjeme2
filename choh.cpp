#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <argp.h>

#include "panic.hpp"
#include "hoh_header.hpp"
#include "lode_io.hpp"

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
		printf("No output file specified. Compression will be performed, but no output file written\n");
	}

	unsigned width = 0, height = 0;

	printf("reading png %s\n", arguments.args[0]);
	uint8_t* decoded = decodeOneStep(arguments.args[0],&width,&height);
	printf("width : %d\n",(int)width);
	printf("height: %d\n",(int)height);

	header header;
	if(arguments.speed == 0){
		int header_size = headerSize(header);
		int pixel_bits = width*height*channelNumber(header) * header.depth;
		size_t max_elements = ((pixel_bits + 31)/32 + header_size*4 + 3)/4;
		uint32_t* out_buf = new uint32_t[max_elements];
		uint32_t* out_end = out_buf + max_elements;
		uint32_t* outPointer = out_end;

		delete[] out_buf;
	}

	delete[] decoded;

	return 0;
}
