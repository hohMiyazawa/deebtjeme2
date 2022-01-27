#ifndef HOH_HEADER
#define HOH_HEADER

struct HEADER{
	uint32_t width;
	uint32_t height;
	uint8_t depth;
	bool joined;
	bool hasAlpha;
	enum{
		GREY,
		RGB,
		YUV444,
		YUV420
	} mode;
	bool hasJFIF;
	bool hasFallback;
	bool hasProgressive;
	bool hasLength;
	bool hasChecksum;
	bool hasTileOffsets;
	uint32_t tile_x;
	uint32_t tile_y;
	uint32_t tiles;
	uint32_t* tile_offsets;
	uint32_t length;
	uint32_t checksum;
};

struct tileHeader{
	uint32_t width;
	uint32_t height;
};


HEADER parseHeader(uint32_t*& fileIndex){
	HEADER header;
	return header;
}

void writeHeader(HEADER header, uint32_t*& outPointer){
	if(header.hasTileOffsets && (header.tile_x || header.tile_y)){
		for(size_t i = header.tiles-1;i--;){
			*(--outPointer) = *(header.tile_offsets + i);
		}
	}
	if(header.hasChecksum){
		*(--outPointer) = header.checksum;
	}
	if(header.hasLength){
		*(--outPointer) = header.length;
	}

	uint32_t mode_byte =
		(header.depth << 24)
		+ (((int)header.mode + ((int)header.hasAlpha << 3) + ((int)header.joined << 4)) << 16);
		+ ((
			+ ((int)header.hasTileOffsets << 1)
			+ ((int)header.hasChecksum << 2)
			+ ((int)header.hasLength << 3)
			+ ((int)header.hasProgressive << 4)
			+ ((int)header.hasFallback << 5)
			+ ((int)header.hasJFIF << 6)
		) << 8)
		+ 0;//write tile dimensions TODO
	*(--outPointer) = mode_byte;

	*(--outPointer) = header.height - 1;
	*(--outPointer) = header.width  - 1;
	*(--outPointer) = 0x89727972;
}

int headerSize(HEADER header){
	int sum = 4;
	if(header.hasLength){
		sum++;
	}
	if(header.hasChecksum){
		sum++;
	}
	if(header.hasTileOffsets && header.tiles){
		sum += header.tiles - 1;
	}
	return sum;
}

int channelNumber(HEADER header){
	int channels = 3;
	if(header.mode == header.GREY){
		channels = 1;
	}
	if(header.hasAlpha){
		channels++;
	}
	return channels;
}

#endif //HOH_HEADER
