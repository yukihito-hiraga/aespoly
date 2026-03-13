#pragma once
#include "../common/common.h"

#include "core.h"

static inline void xctrxoradd512(aes_context* aesctx, hctr2_context* ctx, __m512i S, size_t len, uint8_t *N, uint8_t *C)
{
    alignas(512) __m512i tmp0, tmp1;

    size_t remainder = len % 16;
    size_t ln = len / 16;

    size_t remainder_roll = ln % 4;

    alignas(512) __m512i ctr = _mm512_setr_epi64(1, 0, 2, 0, 3, 0, 4, 0);
    alignas(512) __m512i one1 = _mm512_setr4_epi32(1, 0, 0, 0);
    alignas(512) __m512i inc = _mm512_setr4_epi32(4, 0, 0, 0);
    for (size_t i = 0; i < (ln / 4); i++)
    {
        tmp0 = _mm512_xor_si512(ctr, S);
        tmp0 = aesenc512(tmp0, aesctx->keys);
        tmp1 = _mm512_loadu_si512((__m512i*)N + i);
        tmp1 = _mm512_xor_si512(tmp0, tmp1);
        _mm512_storeu_si512((__m512i*)C + i, tmp1);
        ctr = _mm512_add_epi64(ctr, inc);
    }
    for (size_t i = 0; i < remainder_roll; i++)
    {
        tmp0 = _mm512_xor_si512(ctr, S);
        tmp0 = aesenc512(tmp0, aesctx->keys);
        ((__m128i *)&tmp1)[0] = ((__m128i *)N)[ln - remainder_roll + i];
        tmp1 = _mm512_xor_si512(tmp0, tmp1);
        ((__m128i *)C)[ln - remainder_roll + i] = ((__m128i *)&tmp1)[0];
        ctr = _mm512_add_epi64(ctr, one1);
    }

    if (remainder > 0)
    {
        uint8_t padded[16];
        memset(padded, 0, ctx->blocklength);
        memcpy(padded, N + len - remainder, remainder);
        tmp0 = aesenc512(_mm512_xor_si512(S, CAST2M512i(ln + 1)), aesctx->keys);
        ((__m128i *)(&tmp1))[0] = _mm_loadu_si128(((__m128i *)padded));
        tmp0 = _mm512_xor_si512(tmp1, tmp0);
        memcpy(C + len - remainder, (uint8_t *)&tmp0, remainder);
    }
}