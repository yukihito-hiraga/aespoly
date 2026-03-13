#pragma once
#include "../common/common.h"

#include "core.h"

static inline void xctrxoradd128(aes_context aesctx, hctr2_context ctx, __m128i S, size_t len, uint8_t *N, uint8_t *C)
{
	alignas(16) __m128i tmp0, tmp1;

	size_t remainder = len % ctx.blocklength;
	size_t ln = len / ctx.blocklength;

	alignas(16) __m128i ctr = _mm_setr_epi32(1, 0, 0, 0);
	alignas(16) __m128i inc = _mm_setr_epi32(1, 0, 0, 0);

	for (size_t i = 0; i < ln; i++)
	{
		tmp0 = _mm_xor_si128(ctr, S);
		tmp0 = aesenc128(tmp0, aesctx.keys128);
		tmp1 = _mm_loadu_si128((__m128i *)N + i);
		tmp1 = _mm_xor_si128(tmp0, tmp1);
		_mm_storeu_si128((__m128i *)C + i, tmp1);
		ctr = _mm_add_epi64(ctr, inc);
	}

	if (remainder > 0)
	{
		uint8_t padded[16];
		memset(padded, 0, ctx.blocklength);
		memcpy(padded, N + len - remainder, remainder);
		alignas(16) __m128i lastblk = _mm_loadu_si128(((__m128i *)padded));
		tmp0 = aesenc128(_mm_xor_si128(S, _mm_setr_epi64(_mm_set_pi64x((ln + 1)), _mm_setzero_si64())), aesctx.keys128);
		tmp0 = _mm_xor_si128(_mm_loadu_si128((__m128i *)&lastblk), tmp0);
		memcpy((__m128i *)C + ln, (uint8_t *)(&tmp0), remainder);
	}
}
