#pragma once

static const uint32_t kHashMul = 0x1e35a7bdu;

static int mulHash(uint32_t argb, int shift) {
	return (int)((argb * kHashMul) >> shift);
}
