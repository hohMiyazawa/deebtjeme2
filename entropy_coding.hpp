#ifndef ENTROPY_CODING_HEADER
#define ENTROPY_CODING_HEADER

#include "rans64.h"

typedef struct {
	Rans64DecSymbol decoded;
	uint16_t value;
	uint8_t extra_bits;
} decoded_symbol;

class symbolTable{
	public:
		uint16_t symbols;
		decoded_symbol decode(uint32_t cum_sym);
	private:
		uint16_t index[256];
		decoded_symbol* full_table;
};

decoded_symbol symbolTable::decode(uint32_t cum_sym){
	uint16_t start_location = index[cum_sym >> 23];
	while(start_location < symbols){
		if((*(full_table + start_location + 1)).decoded.start < cum_sym){
			start_location += 1;
		}
		else{
			break;
		}
	}
	return full_table[start_location];
}

class entropyDecoder{
	public:
		uint32_t* data;
		uint16_t decode(symbolTable* table);
		uint16_t read_range(uint16_t range);
		uint16_t read_extraBits(uint8_t val);
		void init(uint32_t*& fileIndex);
		symbolTable decode_symbolTable(uint8_t length);
	private:
		Rans64State rans_state;
		uint32_t bypass_state;
		uint8_t bypass_length;
		uint32_t prob_bits;
};

uint16_t entropyDecoder::read_extraBits(uint8_t val){
	return read_range(1 << (val - 1));
/*
	if(val <= this.bypass_length){
		uint16_t value = ((this.bypass_state << (32 - this.bypass_length)) >> (32 - val));
		this.bypass_length -= val;
		return value;
	}
	else{
		uint8_t missing = val - this.bypass_length;
		uint16_t upper = ((this.bypass_state << (32 - this.bypass_length)) >> (32 - this.bypass_length)) << missing;
		this.bypass_state = *(this.data++);
		this.bypass_length = 32;
		return upper + this.read_extraBits(missing);
	}
*/
}

uint16_t entropyDecoder::read_range(uint16_t range){
	uint32_t cum = Rans64DecGet(&rans_state, prob_bits);
	uint32_t perRange = (1 << prob_bits)/range;
	uint16_t sym = cum/perRange;	
	Rans64DecSymbol dsym;
	Rans64DecSymbolInit(&dsym, sym*perRange, perRange);
	Rans64DecAdvanceSymbol(&rans_state, &data, &dsym, 31);
	return sym;
}

void entropyDecoder::init(uint32_t*& fileIndex){
	data = fileIndex;
/*
	this.bypass_state = *this.data;
	this.bypass_length = 32;
	for(size_t i=31;i--;){
		if((1 << i) & this.bypass_state){
			this.bypass_state = this.bypass_state ^ (1 << i);
			this.bypass_length--;
		}
		else{
			break;
		}
	}
	this.bypass_length--;
	//TODO
	(*this.data)++;
*/
	//read rans state
	Rans64State rans;
        Rans64DecInit(&rans, &fileIndex);
	prob_bits = 31;
	(*data)++;
}

uint16_t entropyDecoder::decode(symbolTable* table){
	if((*table).symbols == 1){
		return 0;
	}
	uint32_t s = Rans64DecGet(&rans_state, prob_bits);
	decoded_symbol dec = table->decode(s);
	Rans64DecAdvanceSymbol(&rans_state, &data, &dec.decoded, prob_bits);
	if(dec.extra_bits){
		return dec.value + read_extraBits(dec.extra_bits);
	}
	return dec.value;
}

symbolTable entropyDecoder::decode_symbolTable(uint8_t length){
	uint8_t mode = read_extraBits(2);
	symbolTable table;
	if(mode == 0){//all zeroes
	}
	else if(mode == 1){//equal weights
	}
	else if(mode == 2){//laplace?
		uint8_t readymade = read_extraBits(2);
	}
	else{//custom
		uint8_t treemode = read_extraBits(2);
		uint8_t mirroring = read_extraBits(1);
		uint8_t powermode = read_extraBits(2);
	}
	return table;
}
/*
	0: FLAT 1
	2-x: some prefab tables. Laplace(?)
	x: tailored
	
	[0,1,2,3] power range
	[0|1] binary tree
	[0|1] delta modulation
	[0|1] power encoding table

	    0,    1,   2,    4

	    8,   16,  32,   64

	  128,  256, 512, 1024
	 2048, 4096,8192,16384

	1<<16, 1<<17, 1<<18, 1<<19
	1<<20, 1<<21, 1<<22, 1<<23
	1<<24, 1<<25, 1<<26, 1<<27
	1<<28, 1<<29, 1<<30, 1<<30
*/

class entropyEncoder{
	public:
		uint32_t* data;
		void encode(symbolTable* table,uint16_t val);
		void write_range(uint16_t range, uint16_t val);
		void write_extraBits(uint8_t val, uint16_t bits);
		entropyEncoder(uint32_t*& out_end);
		void encode_symbolTable(uint8_t length, symbolTable table);
	private:
		Rans64State rans_state;
		uint32_t bypass_state;
		uint8_t bypass_length;
		uint32_t prob_bits;
};

entropyEncoder::entropyEncoder(uint32_t*& fileIndex){
	data = fileIndex;
	Rans64State rans;
        Rans64DecInit(&rans, &fileIndex);
	prob_bits = 31;
	(*data)--;
}

#endif

