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
	symbolTable* orderZero;
	size_t orderZero_size;
	size_t size;
};

typedef struct{
	uint32_t* data;
	Rans64State rans_state;
	uint32_t prob_bits;
	uint64_t buffer;
	uint8_t buffer_size;
}ransInfo;

uint16_t readBits(
	uint8_t bits,
	ransInfo rans
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
	ransInfo rans
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
		while(pivot1 != pivot2){
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
	ransInfo rans
){
	uint8_t encodeMode = readBits(3,rans);
	symbolTable table;
	table.size = table_size;
	if(encodeMode == 0){
		table.mode = 0;
	}
	else if(encodeMode == 1){
		table.mode = 1;
	}
	else if(encodeMode == 2){
		SymbolStats2 stats;
	}
	return table;
}

#endif

