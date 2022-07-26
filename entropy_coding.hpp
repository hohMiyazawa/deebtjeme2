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

size_t count_round(size_t count){
	//return count;//implement later
	if(count){
		size_t base = count;
		size_t zeroes = log2_plus(count) - 1;
		for(size_t i=0;i<zeroes/2;i++){
			base = base ^ (1 << i);
		}
		return base;
	}
	return 0;
}

size_t count_mag(size_t count){
	return log2_plus(count);
}

double count_cost(size_t count){
/*
0 1 2 4
8 16 32 64
128 256 512 1024
2048 4096 8192 16384
*/
	if(count){
		return 5 + (log2_plus(count) - 1)/2;
	}
	else{
		return 5;
	}
}

double pair_entropy_saving(size_t a, size_t b){
	if(a == 0){
		return (double)(a+b);
	}
	else if(b == 0){
		return (double)(a+b);
	}
	return (double)a * log2((double)a/(double)(a+b))
		+ (double)b * log2((double)b/(double)(a+b))
		+ (double)(a+b);
}

typedef struct treesymbol treesymbol;

struct treesymbol{
	bool leaf;
	uint16_t value;
	uint8_t extra_bits;
	size_t count;
	size_t smooth_count;
	uint32_t cum_freq;
	double cost;
	double overhead_cost;
	Rans64EncSymbol encoded;
	treesymbol* left;
	treesymbol* right;
	~treesymbol();
};

treesymbol::~treesymbol(){
	if(!leaf){
		delete left;
		delete right;
	}
}

size_t enumerate_nodes(treesymbol* root){
	if(root->leaf){
		return 1;
	}
	else{
		return enumerate_nodes(root->left) + enumerate_nodes(root->right);
	}
}

double tree_cost(treesymbol* root,size_t quant_count){
	if(root->leaf){
		if(root->count){
			return count_cost(root->count)
				- (double)root->count * log2((double)root->smooth_count/(double)quant_count)
				+ (double)root->count * (double)root->extra_bits
				+ (root->extra_bits ? 1 : 0);
		}
		else{
			return count_cost(0)
				+ (root->extra_bits ? 1 : 0);
		}
	}
	else{
		return tree_cost(root->left,quant_count)
			+ tree_cost(root->right,quant_count)
			+ (root->extra_bits ? 1 : 0);//left + right + decision bit
	}
}

void tree_listing(treesymbol* root){
	if(root->leaf){
		printf("ll %d,%d %d\n",(int)root->extra_bits,(int)count_mag(root->smooth_count),(int)root->smooth_count);
	}
	else{
		tree_listing(root->left);
		tree_listing(root->right);
	}
}

size_t tree_quant_summer(treesymbol* root){
	if(root->leaf){
		return root->smooth_count;
	}
	else{
		return tree_quant_summer(root->left) + tree_quant_summer(root->right);
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
	printf("sum %d\n",(int)sum);
	double log_sum = log2(sum);
	size_t row_number = 0;
	size_t offset = 0;
	size_t last_offset = 0;
	treesymbol* row[row_width * 2 - 1];
	for(size_t i=0;i<row_width;i++){
		row[i] = new treesymbol;
		row[i]->leaf = true;
		row[i]->value = i;
		row[i]->extra_bits = 0;
		row[i]->count = stats.freqs[i];
		row[i]->smooth_count = count_round(stats.freqs[i]);
		row[i]->cum_freq = 0;//set later after scaling
		row[i]->cost = 0;
		row[i]->overhead_cost = count_cost(row[i]->count);
	}
	while(row_width > 1){
		last_offset = offset;
		offset += row_width;
		row_width = (row_width + 1)/2;
		for(size_t i=0;i<row_width;i++){
			//printf("off %d\n",(int)(offset + i));
			treesymbol* left  = row[last_offset + i*2];
			treesymbol* right = row[last_offset + i*2 + 1];
			size_t combi_count = left->count + right->count;
			size_t combi_smooth = count_round(combi_count);

			double sep_cost = (double)left->count * log2((double)left->smooth_count/(double)(left->smooth_count + right->smooth_count))
				+ (double)right->count * log2((double)right->smooth_count/(double)(left->smooth_count + right->smooth_count)) + (double)combi_count;
			if(!combi_count){
				sep_cost = 0;
			}
			else if(!left->count){
				sep_cost = (double)combi_count;
			}
			else if(!right->count){
				sep_cost = (double)combi_count;
			}
			row[offset + i] = new treesymbol;
			row[offset + i]->value = left->value;
			row[offset + i]->extra_bits = left->extra_bits + 1;
			row[offset + i]->count = combi_count;
			row[offset + i]->cum_freq = 0;//set later after scaling
			if(count_cost(combi_count) > -sep_cost + left->overhead_cost + right->overhead_cost + left->cost + right->cost){
				row[offset + i]->leaf = false;
				row[offset + i]->left = left;
				row[offset + i]->right = right;
				row[offset + i]->smooth_count = combi_smooth;//dummy value
				row[offset + i]->cost = -sep_cost + left->cost + right->cost;
				row[offset + i]->overhead_cost = left->overhead_cost + right->overhead_cost + 1;
			}
			else{
				row[offset + i]->leaf = true;
				row[offset + i]->smooth_count = combi_smooth;
				row[offset + i]->cost = 0;
				row[offset + i]->overhead_cost = count_cost(combi_count) + 1;
				//printf("pair (%d,%d = %f)\n",(int)left->count,(int)right->count,pair_entropy_saving(left->count,right->count));
				//printf("merge (%d,%d) %f %f\n",(int)left->count,(int)right->count,count_cost(combi_count),-sep_cost + left->overhead_cost + right->overhead_cost + left->cost + right->cost);
				delete left;
				delete right;
			}
		}
	}
	printf("n_count %d\n",(int)enumerate_nodes(row[stats.total * 2 - 2]));
	size_t quant_sum = tree_quant_summer(row[stats.total * 2 - 2]);
	
	printf("cost %f\n",tree_cost(row[stats.total * 2 - 2],quant_sum));
	//tree_listing(row[stats.total * 2 - 2]);
	return row[stats.total * 2 - 2];
}

void writeTree_symbol(
	size_t val,
	treesymbol* root,
	ransInfo& rans
){
	while(!root->leaf){
		if(root->right->value > val){
			root = root->left;
		}
		else{
			root = root->right;
		}
	}
	writeBits(root->extra_bits,val - root->value,rans);
	Rans64EncPutSymbol(&rans.rans_state, &rans.data, &root->encoded, rans.prob_bits);
}
	
#endif

