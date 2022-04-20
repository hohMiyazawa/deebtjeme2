#ifndef ENTROPY_CODING_HEADER
#define ENTROPY_CODING_HEADER

#include "rans64.h"
#include "symbolstats.hpp"
#include "numerics.hpp"
/////
typedef struct {
	Rans64DecSymbol decoded;
	uint16_t value;
	uint8_t extra_bits;
	uint32_t cum;
} decoded_symbol;

typedef struct {
	Rans64EncSymbol encoded;
	uint16_t value;
	uint8_t extra_bits;
} encoded_symbol;

typedef struct enode_tree enode_tree;

struct enode_tree{
	bool leaf;
	encoded_symbol* enode;
	enode_tree* left;
	enode_tree* right;
	uint16_t thresh;
};

typedef struct symbolTable symbolTable;

struct symbolTable{
	uint8_t mode;
//modes:
// 0: always 0 (return value, do not rans
// 1: raw bits
// 2: fully coded (return decoded symbol, ignore extra bits)
// 3: extra_bits
// 4: extra_bits with order 0
	decoded_symbol* nodes;
	encoded_symbol* enodes;
	enode_tree* etree;
	symbolTable* orderZero;
	size_t orderZero_size;
	size_t size;
	size_t valSize;
	bool mirrored;
	bool codeCodes;
};

typedef struct{
	uint32_t* data;
	Rans64State rans_state;
	uint32_t prob_bits;
	uint32_t prob_scale;
	uint64_t buffer;
	uint8_t buffer_size;
}ransInfo;

uint16_t readBits(
	uint8_t bits,
	ransInfo& rans
){
	if(bits == 0){
		return 0;
	}
	if(bits > rans.buffer_size){
		rans.buffer += (*(rans.data++)) << rans.buffer_size;
		rans.buffer_size += 32;
	}
	uint16_t value = rans.buffer % (1 << bits);
	rans.buffer >> bits;
	rans.buffer_size -= bits;
	return value;
}

uint16_t readValue(
	symbolTable table,
	ransInfo& rans
){
	if(table.mode == 0){
		return 0;
	}
	else if(table.mode == 1){
		return readBits(log2_plus(table.size - 1) + 1,rans);//check if this is right
	}
	else{//partition search
		uint32_t s = Rans64DecGet(&(rans.rans_state), rans.prob_bits);
		uint16_t pivot1 = 0;
		uint16_t pivot2 = table.size;
		while(pivot2 - pivot1 > 1){
			uint16_t pivot3 = (pivot1 + pivot2)/2;
			if(table.nodes[pivot3].cum > s){
				pivot2 = pivot3;
			}
			else{
				pivot1 = pivot3;
			}
		}
		Rans64DecAdvanceSymbol(&(rans.rans_state), &(rans.data), &(table.nodes[pivot1].decoded), rans.prob_bits);
		if(table.mode == 2){
			return pivot1;
		}
		else{
			uint16_t base = table.nodes[pivot1].value;
			if(table.mode == 3 || table.orderZero_size > table.nodes[pivot1].extra_bits){
				return base + readBits(table.nodes[pivot1].extra_bits,rans);
			}
			else{
				uint16_t orderZ = readValue(*(table.orderZero),rans);
				return base + orderZ + (readBits(table.nodes[pivot1].extra_bits - table.orderZero_size,rans) << table.orderZero_size);
			}
		}
	}
}

symbolTable readSymbolTable(
	size_t table_size,
	ransInfo& rans
){
	uint8_t encodeMode = readBits(3,rans);
	symbolTable table;
	table.valSize = table_size;
	if(encodeMode == 0){//zero
		table.mode = 0;
	}
	else if(encodeMode == 1){//raw
		table.mode = 1;
		table.size = table_size;
	}
	else if(encodeMode == 2){//flat
		table.mode = 2;
		table.size = table_size;
		SymbolStats2 stats;
		stats.init(table.size);
		for(size_t i=0;i<table.size;i++){
			stats.freqs[i] = 1;
		}
		stats.normalize_freqs(rans.prob_scale);
		table.nodes = new decoded_symbol[table.size];
		for(size_t i=0;i<table.size;i++){
			table.nodes[i].cum = stats.cum_freqs[i];
			Rans64DecSymbolInit(&(table.nodes[i].decoded), stats.cum_freqs[i], stats.freqs[i]);
		}
	}
	else if(encodeMode == 3){//laplace
		table.mode = 2;
		table.size = table_size;
		//TODO
	}
	else if(encodeMode == 4){//fully weighted
		table.mode = 2;
		bool mirrored = readBits(1,rans);
		bool codeCodes = readBits(1,rans);
		table.mirrored = mirrored;
		table.codeCodes = codeCodes;
		SymbolStats2 stats;
		stats.init(table.size);
		if(codeCodes){
			//TODO
		}
		else{
			if(mirrored){
				//TODO
			}
			else{
/*
0	0    0
1	1    1
2	2    10
3	4    100
4	8    1x00
5	16   1x000
6	32   1xx000
7	64   1xx0000
...
15
*/
				for(size_t i=0;i<table.size;i++){
					stats.freqs[i] = 0;
					size_t expoCode = readBits(4,rans);
					if(expoCode){
						stats.freqs[i] = 1 << (expoCode - 1);
						if(expoCode > 3){
							size_t extraBits = (expoCode - 2)/2;
							size_t shift_dist = (expoCode + 1)/2;
							stats.freqs[i] += readBits(extraBits,rans) << shift_dist;
						}
					}
				}
			}
		}
		stats.normalize_freqs(rans.prob_scale);
		table.nodes = new decoded_symbol[table.size];
		for(size_t i=0;i<table.size;i++){
			table.nodes[i].cum = stats.cum_freqs[i];
			Rans64DecSymbolInit(&(table.nodes[i].decoded), stats.cum_freqs[i], stats.freqs[i]);
		}
	}
	else if(encodeMode == 5){//exponential
		bool codeCodes = readBits(1,rans);
		bool orderZero = readBits(1,rans);
	}
	else if(encodeMode == 6){//webp exponential
		bool codeCodes = readBits(1,rans);
		bool orderZero = readBits(1,rans);
	}
	else if(encodeMode == 7){//custom
		//bool mirrored  = readBits(1,rans); //maybe
		bool codeCodes = readBits(1,rans);
		bool orderZero = readBits(1,rans);
	}
	return table;
}

//ENCODE

void writeBits(
	uint8_t bits,
	uint16_t value,
	ransInfo& rans
){
	if(bits == 0){
		return;
	}
	rans.buffer = rans.buffer << bits + value;
	rans.buffer_size += bits;
	if(rans.buffer_size > 31){
		rans.buffer_size = rans.buffer_size - 32;
		*(--rans.data) = rans.buffer >> rans.buffer_size;
		rans.buffer = rans.buffer % (1 << rans.buffer_size);
	} 
}


void writeValue(
	uint16_t value,
	symbolTable& table,
	ransInfo& rans
){
	if(table.mode == 0){
		return;
	}
	else if(table.mode == 1){
		writeBits(log2_plus(table.size - 1) + 1,value,rans);//check if this is right
	}
	else{
		if(table.mode == 2){
			Rans64EncPutSymbol(&rans.rans_state, &rans.data, &table.enodes[value].encoded, rans.prob_bits);
		}
		else{
			enode_tree* root = table.etree;
			while(!((*root).leaf)){
				if((*root).thresh > value){
					root = (*root).left;
				}
				else{
					root = (*root).right;
				}
			}
			encoded_symbol enc = *((*root).enode);
			if(table.mode == 3){
				writeBits(enc.extra_bits,value - enc.value,rans);
			}
			else{
				if(enc.extra_bits < table.orderZero_size){
					writeBits(enc.extra_bits,value - enc.value,rans);
				}
				else{
					writeBits(enc.extra_bits - table.orderZero_size,(value - enc.value) >> table.orderZero_size,rans);
					writeValue(
						value % (1 << table.orderZero_size),
						*(table.orderZero),
						rans
					);
				}
			}
			Rans64EncPutSymbol(&rans.rans_state, &rans.data, &enc.encoded, rans.prob_bits);
		}
	}
}

void writeSymbolTable(
	symbolTable& table,
	ransInfo& rans
){
	if(table.mode < 4){
		//nothing to do
	}
	else if(table.mode == 4){
		writeBits(1,table.codeCodes,rans);
		writeBits(1,table.mirrored,rans);
		if(table.mirrored){
		}
		else{
			if(table.codeCodes){
			}
			else{
				for(size_t i=table.size;--i;){
				}
			}
		}
	}
	writeBits(3,table.mode,rans);
}

Rans64EncSymbol* createEncodeTable_strat1(
	SymbolStats2& stats,
	ransInfo& rans
){
	//simple first strategy, encode as fully weighted, simple 4-bit weights
	for(size_t i=0;i<stats.total;i++){
		//idk, see if this can be rounded better than just truncating
		size_t magnitude = log2_plus(stats.freqs[i]);
		size_t shift_dist = (magnitude + 1)/2;
		stats.freqs[i] = (stats.freqs[i] >> shift_dist) << shift_dist;
	}
	stats.normalize_freqs(rans.prob_scale);
	Rans64EncSymbol* esyms = new Rans64EncSymbol[stats.total];
	for(size_t i=0;i<stats.total;i++){
		Rans64EncSymbolInit(&esyms[i], stats.cum_freqs[i], stats.freqs[i], rans.prob_bits);
	}
	return esyms;
}

void writeEncodeTable_strat1(
	SymbolStats2& stats,
	ransInfo& rans
){
}

symbolTable createEncodeTable(
	SymbolStats2& stats,
	ransInfo& rans
){
	//simple first strategy, encode as fully weighted, simple 4-bit weights
	for(size_t i=0;i<stats.total;i++){
		//idk, see if this can be rounded better than just truncating
		size_t magnitude = log2_plus(stats.freqs[i]);
		size_t shift_dist = (magnitude + 1)/2;
		stats.freqs[i] = (stats.freqs[i] >> shift_dist) << shift_dist;
	}
	stats.normalize_freqs(rans.prob_scale);
	symbolTable table;
	table.mode = 2;
	table.size = stats.total;
	table.valSize = table.size;
	table.enodes = new encoded_symbol[table.size];
	for(size_t i=0;i<table.size;i++){
		Rans64EncSymbolInit(&table.enodes[i].encoded, stats.cum_freqs[i], stats.freqs[i], rans.prob_bits);
	}
	return table;
}

typedef struct {
	bool leaf;
	uint16_t value;
	uint8_t extra_bits;
	size_t count;
	size_t smooth_count;
	uint32_t cum_freq;
	double cost;
	Rans64EncSymbol encoded;
	treesymbol* left;
	treesymbol* right;
	~treesymbol();
} treesymbol;

treesymbol::~treesymbol(){
	if(leaf && extra_bits > 0){
		delete left;
		delete right;
	}
}

treesymbol* tree_builder(
	SymbolStats2& stats
){
	size_t row_width = stats.total;
	size_t sum = 0;
	for(size_t i=0;i<row_width;i++){
		sum += stats.freqs[i];
	}
	size_t row_number = 0;
	size_t offset = 0;
	size_t last_offset = 0;
	treesymbol[row_width * 2 - 1] row;
	for(size_t i=0;i<row_width;i++){
		row[i] = new treesymbol;
		row[i].leaf = true;
		row[i].value = i;
		row[i].extra_bits = 0;
		row[i].count = stats.freqs[i];
		row[i].smooth_count = count_round(stats.freqs[i]);
		row[i].cum_freq = 0;//set later after scaling
		row[i].cost = - (sum - row[i].count) * log2((sum - row[i].smooth_count)/sum)
			- row[i].count * log2(row[i].smooth_count/sum);
			+ count_cost(row[i].smooth_count);
	}
	while(row_width > 1){
		last_offset = offset;
		offset += row_width;
		row_width = (row_width + 1)/2;
		for(size_t i=0;i<row_width;i++){
			treesymbol left  = row[last_offset + i*2];
			treesymbol right = row[last_offset + i*2 + 1];
			size_t combi_count = left.count + right.count;
			size_t combi_smooth = count_round(combi_count);

			double iso_cost = left.cost + right.cost + 1;
			double combi_cost = - (sum - combi_count) * log2((sum - combi_smooth)/sum)
				- combi_count * log2(combi_smooth/sum);
				+ count_cost(combi_smooth) + 1;
			row[offset + i] = new treesymbol;
			row[offset + i].value = left.value;
			row[offset + i].extra_bits = left.extra_bits + 1;
			row[offset + i].count = combi_count;
			row[offset + i].cum_freq = 0;//set later after scaling
			if(iso_cost < combi_cost){
				row[offset + i].leaf = false;
				row[offset + i].smooth_count = combi_smooth;//dummy value
				row[offset + i].cost = iso_cost;
			}
			else{
				row[offset + i].leaf = true;
				row[offset + i].smooth_count = combi_smooth;
				row[offset + i].cost = combi_cost;
				delete left;
				delete right;
			}
		}
	}
	return (treesymbol[0])&;
}
	
#endif

