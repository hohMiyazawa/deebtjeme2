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
	uint16_t start_location = this.index[cum_sym >> 23];
	while(start_location < this.symbols){
		if(*(this.full_table + start_location + 1).decoded.start < cum_sym){
			start_location += 1;
		}
		else{
			break;
		}
	}
	return start_location;
}

class entropyDecoder{
	public:
		uint32_t* data;
		uint16_t decode(symbolTable* table);
		uint16_t read_extraBits(uint8_t val);
		void init(uint32_t*& fileIndex);
	private:
		Rans64State rans_state;
		uint32_t bypass_state;
		uint8_t bypass_length;
		uint32_t prob_bits;
}

uint16_t entropyDecoder::read_extraBits(uint8_t val){
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
}

void entropyDecoder::init(uint32_t*& fileIndex){
	this.data = fileIndex;
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
	//read rans state
	Rans64State rans;
        Rans64DecInit(&rans, &fileIndex);
	(*this.data)++;
}

uint16_t entropyDecoder::decode(symbolTable* table){
	if(*table.symbols == 1){
		return 0;
	}
	uint32_t s = Rans64DecGet(&this.rans_state, this.prob_bits);
	decoded_symbol = table.decode(s);
	Rans64DecAdvanceSymbol(&this.rans_state, &this.data, &decoded_symbol.decoded, this.prob_bits);
	if(decoded_symbol.extra_bits){
		return decoded_symbol.value + this.read_extraBits(decoded_symbol.extra_bits);
	}
	return decoded_symbol.value;
}

symbolTable decode_symbolTable(){
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

