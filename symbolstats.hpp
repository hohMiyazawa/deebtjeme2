#ifndef SYMBOLSTATS_HEADER
#define SYMBOLSTATS_HEADER

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <cmath>

struct SymbolStats
{
    uint32_t freqs[256];
    uint32_t cum_freqs[257];

    void count_freqs(uint8_t const* in, size_t nbytes);
    void calc_cum_freqs();
    void init();
    void normalize_freqs(uint32_t target_total);
};

void SymbolStats::init()
{
    for (int i=0; i < 256; i++)
        freqs[i] = 0;
}

void SymbolStats::count_freqs(uint8_t const* in, size_t nbytes)
{
    for (int i=0; i < 256; i++)
        freqs[i] = 0;

    for (size_t i=0; i < nbytes; i++)
        freqs[in[i]]++;
}

void SymbolStats::calc_cum_freqs()
{
    cum_freqs[0] = 0;
    for (int i=0; i < 256; i++)
        cum_freqs[i+1] = cum_freqs[i] + freqs[i];
}

void SymbolStats::normalize_freqs(uint32_t target_total)
{
    assert(target_total >= 256);
    
    calc_cum_freqs();
    uint32_t cur_total = cum_freqs[256];
    
    // resample distribution based on cumulative freqs
    for (int i = 1; i <= 256; i++)
        cum_freqs[i] = ((uint64_t)target_total * cum_freqs[i])/cur_total;

    // if we nuked any non-0 frequency symbol to 0, we need to steal
    // the range to make the frequency nonzero from elsewhere.
    //
    // this is not at all optimal, i'm just doing the first thing that comes to mind.
    for (int i=0; i < 256; i++) {
        if (freqs[i] && cum_freqs[i+1] == cum_freqs[i]) {
            // symbol i was set to zero freq

            // find best symbol to steal frequency from (try to steal from low-freq ones)
            uint32_t best_freq = ~0u;
            int best_steal = -1;
            for (int j=0; j < 256; j++) {
                uint32_t freq = cum_freqs[j+1] - cum_freqs[j];
                if (freq > 1 && freq < best_freq) {
                    best_freq = freq;
                    best_steal = j;
                }
            }
            assert(best_steal != -1);

            // and steal from it!
            if (best_steal < i) {
                for (int j = best_steal + 1; j <= i; j++)
                    cum_freqs[j]--;
            } else {
                assert(best_steal > i);
                for (int j = i + 1; j <= best_steal; j++)
                    cum_freqs[j]++;
            }
        }
    }

    // calculate updated freqs and make sure we didn't screw anything up
    assert(cum_freqs[0] == 0 && cum_freqs[256] == target_total);
    for (int i=0; i < 256; i++) {
        if (freqs[i] == 0)
            assert(cum_freqs[i+1] == cum_freqs[i]);
        else
            assert(cum_freqs[i+1] > cum_freqs[i]);

        // calc updated freq
        freqs[i] = cum_freqs[i+1] - cum_freqs[i];
    }
}

///

struct SymbolStats2{
	uint32_t* freqs;
	uint32_t* cum_freqs;

	void calc_cum_freqs();
	void init(size_t size);
	void normalize_freqs(uint32_t target_total);
	size_t total;
	double* cost_table();
	~SymbolStats2();
};

void SymbolStats2::init(size_t size){
	total = size;
	freqs = new uint32_t[size];
	cum_freqs = new uint32_t[size + 1];
	for(size_t i=0;i<size;i++){
		freqs[i] = 0;
	}
}

SymbolStats2::~SymbolStats2(){
	delete freqs;
	delete cum_freqs;
}

void SymbolStats2::calc_cum_freqs(){
	cum_freqs[0] = 0;
	for(size_t i=0;i<total;i++){
		cum_freqs[i+1] = cum_freqs[i] + freqs[i];
	}
}

double* SymbolStats2::cost_table(){
	double* table = new double[total];
	for(size_t i=0;i<total;i++){
		if(freqs[i] == 0){
			table[i] = std::log2((double)total);
		}
		else{
			table[i] = -std::log2((double)freqs[i]/(double)total);
		}
	}
	return table;
}

void SymbolStats2::normalize_freqs(uint32_t target_total){
    calc_cum_freqs();
    uint32_t cur_total = cum_freqs[total];
    
    // resample distribution based on cumulative freqs
    for (int i = 1; i <= total; i++)
        cum_freqs[i] = ((uint64_t)target_total * cum_freqs[i])/cur_total;

    // if we nuked any non-0 frequency symbol to 0, we need to steal
    // the range to make the frequency nonzero from elsewhere.
    //
    // this is not at all optimal, i'm just doing the first thing that comes to mind.
    for (int i=0; i < total; i++) {
        if (freqs[i] && cum_freqs[i+1] == cum_freqs[i]) {
            // symbol i was set to zero freq

            // find best symbol to steal frequency from (try to steal from low-freq ones)
            uint32_t best_freq = ~0u;
            int best_steal = -1;
            for (int j=0; j < total; j++) {
                uint32_t freq = cum_freqs[j+1] - cum_freqs[j];
                if (freq > 1 && freq < best_freq) {
                    best_freq = freq;
                    best_steal = j;
                }
            }
            assert(best_steal != -1);

            // and steal from it!
            if (best_steal < i) {
                for (int j = best_steal + 1; j <= i; j++)
                    cum_freqs[j]--;
            } else {
                assert(best_steal > i);
                for (int j = i + 1; j <= best_steal; j++)
                    cum_freqs[j]++;
            }
        }
    }

    // calculate updated freqs and make sure we didn't screw anything up
    assert(cum_freqs[0] == 0 && cum_freqs[total] == target_total);
    for (int i=0; i < total; i++) {
        if (freqs[i] == 0)
            assert(cum_freqs[i+1] == cum_freqs[i]);
        else
            assert(cum_freqs[i+1] > cum_freqs[i]);

        // calc updated freq
        freqs[i] = cum_freqs[i+1] - cum_freqs[i];
    }
}
#endif //SYMBOLSTATS_HEADER
