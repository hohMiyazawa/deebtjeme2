#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "../panic.hpp"
#include "../file_io.hpp"
#include "../hash.hpp"
#include "../lode_io.hpp"

/*
simulates a very simple codec, stores matches in 4-byte tokens
16bit backref
8bit matchlen (5 - 260)
8bit offset
*/

int main(int argc, char *argv[]){
	if(argc < 4){
		printf("lz file hash_bits hash_buckets\n");
		return 1;
	}
	size_t hash_bits = atoi(argv[2]);
	size_t hash_buckets = atoi(argv[3]);

	uint32_t hashSize = (1 << hash_bits) * hash_buckets;
	uint32_t* hashMap = new uint32_t[hashSize];
	for(size_t i=0;i<hashSize;i++){
		hashMap[i] = 0xFFFFFFFF;
	}

	size_t in_size;
	uint8_t* in_bytes = read_file(argv[1], &in_size);

	size_t size = 0;
	size_t since_last = 256;
	for(size_t i=0;i<in_size - 3;i++){
		if(since_last == 256){
			since_last = 0;
			size++;
		}
		int hash = mulHash(
			(in_bytes[i] << 24) + (in_bytes[i+1] << 16) + (in_bytes[i+2] << 8) + (in_bytes[i+3]),
			32 - hash_bits
		);
		size_t best_matchlen = 0;
		size_t best_location = 0;
		for(size_t bucket = 0;bucket<hash_buckets;bucket++){
			size_t location = hashMap[hash*hash_buckets + bucket];
			size_t matchlen = 0;
			if(location != 0xFFFFFFFF){
				if(location >= i){
					printf("oh no %d\n",(int)location);
				}
				else{
					for(size_t len=0;i + len < in_size;len++){
						if(in_bytes[location + len] != in_bytes[i + len]){
							break;
						}
						//printf("%c %c\n",(char)in_bytes[i + len],(char)in_bytes[location + len]);
						matchlen++;
					}
					//printf("loc %d cur %d len %d\n",(int)location,(int)i,(int)matchlen);
					if(best_matchlen < matchlen){
						best_matchlen = matchlen;
						best_location = location;
					}
				}
			}
		}
		for(size_t bucket = 1;bucket<hash_buckets;bucket++){
			hashMap[hash*hash_buckets + bucket - 1] = hashMap[hash*hash_buckets + bucket];
		}
		hashMap[hash*hash_buckets + hash_buckets - 1] = i;
		if(best_matchlen > 4){
			//printf("mat %d\n",(int)best_matchlen);
			size+=4;
			while(best_matchlen--){
				i++;
				int hash = mulHash(
					(in_bytes[i] << 24) + (in_bytes[i+1] << 16) + (in_bytes[i+2] << 8) + (in_bytes[i+3]),
					32 - hash_bits
				);
				for(size_t bucket = 1;bucket<hash_buckets;bucket++){
					hashMap[hash*hash_buckets + bucket - 1] = hashMap[hash*hash_buckets + bucket];
				}
				hashMap[hash*hash_buckets + hash_buckets - 1] = i;
			}
		}
		else{
			size++;
		}
	}


	printf("size: %d bytes\n",(int)size);
	delete[] hashMap;
	delete[] in_bytes;
	return 0;
}
