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
#include "colour_filters.hpp"
#include "entropy_coding.hpp"
#include "encode.hpp"
#include "lz_matchFinder.hpp"

#include "backref_table.hpp"
#include "matchlen_table.hpp"
#include "offset_table.hpp"
#include "prefix_coding.hpp"

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
	header.mode = COLOURTYPE::RGB;

	size_t header_size = headerSize(header);

	bool isGreyscale = detectGreyscale(rgb);

	if(isGreyscale){
		printf("greyscale image\n");
		header.mode = COLOURTYPE::GREY;
		size_t pixel_bits = width*height*channelNumber(header) * header.depth;

		image_1ch_8bit grey = rgb_to_grey(rgb);

		size_t max_elements = 2*((pixel_bits + 31)/32) + header_size + 10;
		printf("pixel_bits: %d max_elements: %d\n",(int)pixel_bits,(int)max_elements);	
		uint32_t* out_buf = new uint32_t[max_elements];
		uint32_t* out_end = out_buf + max_elements;
		uint32_t* outPointer = out_end;
		rans.data = outPointer;

		printf("starting encoding\n");
		printf("encoding %d pixels\n",(int)grey.header.width*grey.header.height);

		printf("filtering (ffv1)\n");
		image_1ch_8bit filtered = filter_all_1ch_ffv1(grey,256);

		printf("stats\n");
		SymbolStats2 stats;
		stats.init(256);
		for(size_t i = filtered.header.width*filtered.header.height;i--;){
			stats.freqs[filtered.pixels[i]]++;
		}

		treesymbol* entropy_test = tree_builder(stats);

		printf("ent_test: %f\n",entropy_test->cost);

		double* cost = stats.cost_table();
		double* costs = new double[filtered.header.width*filtered.header.height];
		double costSum = 0;
		for(size_t i = filtered.header.width*filtered.header.height;i--;){
			costSum += cost[filtered.pixels[i]];
			costs[i] = cost[filtered.pixels[i]];
		}

		printf("cost sum %f\n",costSum);

		image_1ch_8bit filtered2 = filter_all_1ch_general(
			grey,
			256,
			4,
			4,
			-1,
			2,
			0,
			-2,
			3,
			-1,
			2
		);

//		LZ here

		lz_match* matches;
		image_1ch_8bit* pointy = &filtered;
		size_t match_count =  lz_matchFinder(
			pointy,12,8,costs,
			backref_default,
			matchlen_default,
			offset_default,
			matches
		);
		for(size_t loop = 0;loop<20;loop++){
			SymbolStats2 backref_stats;
			SymbolStats2 matchlen_stats;
			SymbolStats2 offset_stats;
			backref_stats.init(256);
			matchlen_stats.init(256);
			offset_stats.init(256);
			for(size_t i=0;i<match_count;i++){
				uint8_t backref_prefix = inverse_prefix(matches[i].backref);
				backref_stats.freqs[backref_prefix]++;
				uint8_t matchlen_prefix = inverse_prefix(matches[i].matchlen);
				matchlen_stats.freqs[matchlen_prefix]++;
				uint8_t offset_prefix = inverse_prefix(matches[i].offset);
				offset_stats.freqs[offset_prefix]++;
			}
			double* backref_cost = backref_stats.cost_table();
			double backref_cost_flat[256];
			double* matchlen_cost = matchlen_stats.cost_table();
			double matchlen_cost_flat[256];
			double* offset_cost = offset_stats.cost_table();
			double offset_cost_flat[256];
			for(size_t i=0;i<256;i++){
				uint8_t prefix = inverse_prefix(i);
				uint8_t extra = extrabits_from_prefix(prefix);
				backref_cost_flat[i] = backref_cost[prefix] + extra;
				matchlen_cost_flat[i] = matchlen_cost[prefix] + extra;
				offset_cost_flat[i] = offset_cost[prefix] + extra;
			}
			delete[] backref_cost;
			delete[] matchlen_cost;
			delete[] offset_cost;

			delete[] matches;
			match_count =  lz_matchFinder(
				pointy,12,8+loop,costs,
				backref_cost_flat,
				matchlen_cost_flat,
				offset_cost_flat,
				matches
			);
		}
		

		delete[] costs;
		delete[] cost;
		Rans64EncSymbol* esyms = createEncodeTable_strat1(
			stats,
			rans
		);

		for(size_t i = filtered.header.width*filtered.header.height;i--;){
			Rans64EncPutSymbol(&rans.rans_state, &rans.data, &esyms[filtered.pixels[i]], rans.prob_bits);
		}
		delete[] esyms;
		//write tables

		//printf("writing header\n");
		//writeHeader(header, rans.data);
		printf("writing file (%d bytes)\n",(int)((out_end - rans.data)*4));

		write_file(arguments.outputPath, (uint8_t*)rans.data, (out_end - rans.data)*4);

		delete[] filtered;
		delete[] filtered2;
		//delete[] out_buf;
	}
	else{
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
		else if(arguments.speed == 2){
			size_t max_elements = 2*((pixel_bits + 31)/32) + header_size + 10;
			printf("pixel_bits: %d max_elements: %d\n",(int)pixel_bits,(int)max_elements);	
			uint32_t* out_buf = new uint32_t[max_elements];
			uint32_t* out_end = out_buf + max_elements;
			uint32_t* outPointer = out_end;
			rans.data = outPointer;

			printf("starting encoding\n");
			printf("encoding %d pixels\n",(int)rgb.header.width*rgb.header.height);

			printf("colour transform\n");
			rgb_to_gRgBg(rgb);
			printf("filtering (ffv1)\n");
			image_3ch_8bit filtered = filter_all_3ch_ffv1(rgb,256);
			printf("stats\n");

			SymbolStats2 stats_red;
			SymbolStats2 stats_green;
			SymbolStats2 stats_blue;
			stats_red.init(256);
			stats_green.init(256);
			stats_blue.init(256);
			for(size_t i = filtered.header.width*filtered.header.height;i--;){
				stats_red.freqs[filtered.pixels[i*3]]++;
				stats_green.freqs[filtered.pixels[i*3 +1 ]]++;
				stats_blue.freqs[filtered.pixels[i*3  +2 ]]++;
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

			for(size_t i = filtered.header.width*filtered.header.height;i--;){
				Rans64EncPutSymbol(&rans.rans_state, &rans.data, &esyms_blue[ filtered.pixels[i*3  +2]], rans.prob_bits);
				Rans64EncPutSymbol(&rans.rans_state, &rans.data, &esyms_red[  filtered.pixels[i*3  +1]], rans.prob_bits);
				Rans64EncPutSymbol(&rans.rans_state, &rans.data, &esyms_green[filtered.pixels[i*3    ]], rans.prob_bits);
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
		else if(arguments.speed == 3){
			size_t max_elements = 2*((pixel_bits + 31)/32) + header_size + 10;
			printf("pixel_bits: %d max_elements: %d\n",(int)pixel_bits,(int)max_elements);	
			uint32_t* out_buf = new uint32_t[max_elements];
			uint32_t* out_end = out_buf + max_elements;
			uint32_t* outPointer = out_end;
			rans.data = outPointer;

			printf("starting encoding\n");
			printf("encoding %d pixels\n",(int)rgb.header.width*rgb.header.height);

			printf("colour transform\n");
			rgb_to_gRgBg(rgb);
			printf("filtering (ffv1)\n");
			image_3ch_8bit filtered = filter_all_3ch_ffv1(rgb,256);
			printf("stats\n");
			SymbolStats2 stats_red;
			SymbolStats2 stats_green;
			SymbolStats2 stats_blue;
			stats_red.init(256);
			stats_green.init(256);
			stats_blue.init(256);
			for(size_t i = filtered.header.width*filtered.header.height;i--;){
				stats_red.freqs[filtered.pixels[i*3]]++;
				stats_green.freqs[filtered.pixels[i*3 +1 ]]++;
				stats_blue.freqs[filtered.pixels[i*3  +2 ]]++;
			}

			treesymbol* entropy_test1 = tree_builder(stats_red);
			treesymbol* entropy_test2 = tree_builder(stats_green);
			treesymbol* entropy_test3 = tree_builder(stats_blue);

			printf("ent_test: %f\n",entropy_test1->cost);

			double* red_cost = stats_red.cost_table();
			double* green_cost = stats_green.cost_table();
			double* blue_cost = stats_blue.cost_table();
			double* costs = new double[filtered.header.width*filtered.header.height];
			for(size_t i = filtered.header.width*filtered.header.height;i--;){
				costs[i] = red_cost[filtered.pixels[i*3]]
					+ green_cost[filtered.pixels[i*3+1]]
					+ blue_cost[filtered.pixels[i*3+2]];
			}

	//		LZ here

			lz_match* matches;
			image_3ch_8bit* pointy = &filtered;
			size_t match_count =  lz_matchFinder(
				pointy,12,1,costs,backref_default,matchlen_default,offset_default,matches
			);
	//
			delete[] costs;
			delete[] red_cost;
			delete[] green_cost;
			delete[] blue_cost;
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

			for(size_t i = filtered.header.width*filtered.header.height;i--;){
				Rans64EncPutSymbol(&rans.rans_state, &rans.data, &esyms_blue[ filtered.pixels[i*3  +2]], rans.prob_bits);
				Rans64EncPutSymbol(&rans.rans_state, &rans.data, &esyms_red[  filtered.pixels[i*3  +1]], rans.prob_bits);
				Rans64EncPutSymbol(&rans.rans_state, &rans.data, &esyms_green[filtered.pixels[i*3    ]], rans.prob_bits);
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
		else{
			printf("Unsupported speed setting: %d\n",(int)(arguments.speed));
		}
	}
	return 0;
}
