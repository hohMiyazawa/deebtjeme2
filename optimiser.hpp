void optimiser(
	image_3ch_8bit* image
	uint8_t*& outPointer,
	size_t speed
){
	if(speed == 0){
		//entropy coding only
	}
	else if(speed == 1){
		//clamped grad, subtract green
	}
}
