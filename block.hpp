#pragma once

struct pblock{
	uint16_t predictor;//really an index to a context tree predictor selector
	uint16_t context;
	double entropy;
	size_t skipped_count;
	size_t context_unchanged;
	size_t predictor_unchanged;
	bool flat;//blocks having perfect intra prediction is not unusual, for instance in a solid colour background. A speed optimisation is to not try many predictors in those cases
	bool lz_covered;
}


class partition{
	public:
		size_t block_width;//usually 8
		size_t block_height;
		size_t width;
		size_t height;
		size_t img_width;
		size_t img_height;
		size_t size;
		pblock* blocks;
}
