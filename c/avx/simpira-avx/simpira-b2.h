#pragma once

#include "core.h"

#define simpira_C_b2(c) _mm_setr_epi32(0x02 ^ c, 0x12 ^ c, 0x22 ^ c, 0x32 ^ c)

#define simpira_round(c, x0, x1)                            \
    {                                                       \
        x1 = _mm_aesenc_si128(_mm_aesenc_si128(x0, c), x1); \
    }

#define simpira_roundx1(c, tmps)                \
    {                                           \
        simpira_round(c, (tmps)[0], (tmps)[1]); \
    }

#define simpira_roundx1_rev(c, tmps)            \
    {                                           \
        simpira_round(c, (tmps)[1], (tmps)[0]); \
    }

#define simpira_roundx2(c, tmps)                    \
    {                                               \
        simpira_round(c, *(tmps + 0), *(tmps + 1)); \
        simpira_round(c, *(tmps + 2), *(tmps + 3)); \
    }

#define simpira_roundx2_rev(c, tmps)                \
    {                                               \
        simpira_round(c, *(tmps + 1), *(tmps + 0)); \
        simpira_round(c, *(tmps + 3), *(tmps + 2)); \
    }

#define simpira_roundx4(c, tmps)      \
    {                                 \
        simpira_roundx2(c, tmps + 0); \
        simpira_roundx2(c, tmps + 4); \
    }

#define simpira_roundx4_rev(c, tmps)      \
    {                                     \
        simpira_roundx2_rev(c, tmps + 0); \
        simpira_roundx2_rev(c, tmps + 4); \
    }

#define simpira_roundx8(c, tmps)      \
    {                                 \
        simpira_roundx4(c, tmps + 0); \
        simpira_roundx4(c, tmps + 8); \
    }

#define simpira_roundx8_rev(c, tmps)      \
    {                                     \
        simpira_roundx4_rev(c, tmps + 0); \
        simpira_roundx4_rev(c, tmps + 8); \
    }

#define simpira_roundx16(c, tmps)      \
    {                                  \
        simpira_roundx8(c, tmps + 0);  \
        simpira_roundx8(c, tmps + 16); \
    }

#define simpira_roundx16_rev(c, tmps)      \
    {                                      \
        simpira_roundx8_rev(c, tmps + 0);  \
        simpira_roundx8_rev(c, tmps + 16); \
    }

#define simpira_roundx32(c, tmps)       \
    {                                   \
        simpira_roundx16(c, tmps + 0);  \
        simpira_roundx16(c, tmps + 32); \
    }

#define simpira_roundx32_rev(c, tmps)       \
    {                                       \
        simpira_roundx16_rev(c, tmps + 0);  \
        simpira_roundx16_rev(c, tmps + 32); \
    }

static inline void simpira_b2_init(simpira_context *ctx)
{
    for (size_t i = 1; i <= 15; i++)
    {
        ctx->c[i] = simpira_C_b2(i);
    }
}

#define simpira_allrounds(simpira_round_f, simpira_round_rev_f, c, tmps) \
    {                                                                    \
        simpira_round_f(c[1], tmps);                                     \
        simpira_round_rev_f(c[2], tmps);                                 \
        simpira_round_f(c[3], tmps);                                     \
        simpira_round_rev_f(c[4], tmps);                                 \
        simpira_round_f(c[5], tmps);                                     \
        simpira_round_rev_f(c[6], tmps);                                 \
        simpira_round_f(c[7], tmps);                                     \
        simpira_round_rev_f(c[8], tmps);                                 \
        simpira_round_f(c[9], tmps);                                     \
        simpira_round_rev_f(c[10], tmps);                                \
        simpira_round_f(c[11], tmps);                                    \
        simpira_round_rev_f(c[12], tmps);                                \
        simpira_round_f(c[13], tmps);                                    \
        simpira_round_rev_f(c[14], tmps);                                \
        simpira_round_f(c[15], tmps);                                    \
    }

#define simpira_b2_128(ctx, tmp0, tmp1)       \
    {                                         \
                                              \
        simpira_round(ctx.c[1], tmp0, tmp1);  \
        simpira_round(ctx.c[2], tmp1, tmp0);  \
                                              \
        simpira_round(ctx.c[3], tmp0, tmp1);  \
        simpira_round(ctx.c[4], tmp1, tmp0);  \
                                              \
        simpira_round(ctx.c[5], tmp0, tmp1);  \
        simpira_round(ctx.c[6], tmp1, tmp0);  \
                                              \
        simpira_round(ctx.c[7], tmp0, tmp1);  \
        simpira_round(ctx.c[8], tmp1, tmp0);  \
                                              \
        simpira_round(ctx.c[9], tmp0, tmp1);  \
        simpira_round(ctx.c[10], tmp1, tmp0); \
                                              \
        simpira_round(ctx.c[11], tmp0, tmp1); \
        simpira_round(ctx.c[12], tmp1, tmp0); \
                                              \
        simpira_round(ctx.c[13], tmp0, tmp1); \
        simpira_round(ctx.c[14], tmp1, tmp0); \
                                              \
        simpira_round(ctx.c[15], tmp0, tmp1); \
    }

#define simpira_allroundx2(ctx, tmps)                                             \
    {                                                                             \
        simpira_allrounds(simpira_roundx1, simpira_roundx1_rev, ctx.c, tmps);     \
        simpira_allrounds(simpira_roundx1, simpira_roundx1_rev, ctx.c, tmps + 2); \
    }

#define simpira_allroundx4(ctx, tmps)      \
    {                                      \
        simpira_allroundx2(ctx, tmps);     \
        simpira_allroundx2(ctx, tmps + 4); \
    }

#define simpira_allroundx8(ctx, tmps)      \
    {                                      \
        simpira_allroundx4(ctx, tmps);     \
        simpira_allroundx4(ctx, tmps + 8); \
    }

#define simpira_allroundx16(ctx, tmps)      \
    {                                       \
        simpira_allroundx8(ctx, tmps);      \
        simpira_allroundx8(ctx, tmps + 16); \
    }

#define simpira_allroundx32(ctx, tmps)       \
    {                                        \
        simpira_allroundx16(ctx, tmps);      \
        simpira_allroundx16(ctx, tmps + 32); \
    }

#define simpira_b2x1_128(ctx, tmps)                                           \
    {                                                                         \
        simpira_allrounds(simpira_roundx1, simpira_roundx1_rev, ctx.c, tmps); \
    }

#define simpira_b2x2_128(ctx, tmps)                                           \
    {                                                                         \
        simpira_allrounds(simpira_roundx2, simpira_roundx2_rev, ctx.c, tmps); \
    }

#define simpira_b2x4_128(ctx, tmps)                                           \
    {                                                                         \
        simpira_allrounds(simpira_roundx4, simpira_roundx4_rev, ctx.c, tmps); \
    }

#define simpira_b2x8_128(ctx, tmps)                                           \
    {                                                                         \
        simpira_allrounds(simpira_roundx8, simpira_roundx8_rev, ctx.c, tmps); \
    }

#define simpira_b2x16_128(ctx, tmps)                                            \
    {                                                                           \
        simpira_allrounds(simpira_roundx16, simpira_roundx16_rev, ctx.c, tmps); \
    }

#define simpira_b2x32_128(ctx, tmps)                                            \
    {                                                                           \
        simpira_allrounds(simpira_roundx32, simpira_roundx32_rev, ctx.c, tmps); \
    }

static inline void simpira_ecb(simpira_context ctx, uint8_t *P, size_t Plen, uint8_t *C)
{
    alignas(16) __m128i tmps[32];

    size_t remainder = Plen % (32 * 16);
    size_t ln = Plen / (32 * 16);

    for (size_t i = 0; i < ln; i++)
    {
        loadx32((__m128i *)P + 32 * i, tmps);

        simpira_b2_128(ctx, tmps[0], tmps[1]);
        simpira_b2_128(ctx, tmps[2], tmps[3]);
        simpira_b2_128(ctx, tmps[4], tmps[5]);
        simpira_b2_128(ctx, tmps[6], tmps[7]);
        simpira_b2_128(ctx, tmps[8], tmps[9]);
        simpira_b2_128(ctx, tmps[10], tmps[11]);
        simpira_b2_128(ctx, tmps[12], tmps[13]);
        simpira_b2_128(ctx, tmps[14], tmps[15]);
        simpira_b2_128(ctx, tmps[16], tmps[17]);
        simpira_b2_128(ctx, tmps[18], tmps[19]);
        simpira_b2_128(ctx, tmps[20], tmps[21]);
        simpira_b2_128(ctx, tmps[22], tmps[23]);
        simpira_b2_128(ctx, tmps[24], tmps[25]);
        simpira_b2_128(ctx, tmps[26], tmps[27]);
        simpira_b2_128(ctx, tmps[28], tmps[29]);
        simpira_b2_128(ctx, tmps[30], tmps[31]);

        storex32((__m128i *)C + 32 * i, tmps);
    }
}

static inline void simpira_ecbx16(simpira_context ctx, uint8_t *P, size_t Plen, uint8_t *C)
{
    alignas(16) __m128i tmps[32];

    size_t remainder = Plen % (16 * 2 * 16);
    size_t ln = Plen / (16 * 2 * 16);

    for (size_t i = 0; i < ln; i++)
    {
        loadx32((__m128i *)P + 16 * 2 * i, tmps);
        simpira_b2x16_128(ctx, tmps);
        storex32((__m128i *)C + 16 * 2 * i, tmps);
    }
}

static inline void simpira_ecbx32(simpira_context ctx, uint8_t *P, size_t Plen, uint8_t *C)
{
    alignas(16) __m128i tmps[64];

    size_t remainder = Plen % (32 * 2 * 16);
    size_t ln = Plen / (32 * 2 * 16);

    for (size_t i = 0; i < ln; i++)
    {
        loadx64((__m128i *)P + 32 * 2 * i, tmps);
        simpira_b2x32_128(ctx, tmps);
        storex64((__m128i *)C + 32 * 2 * i, tmps);
    }
}

static inline void simpiraxctrx16(simpira_context simpiractx, __m128i *S, uint8_t *P, size_t Plen, uint8_t *C)
{
    size_t blen = Plen / 16;
    size_t dlen = blen / 2;
    size_t dx16len = dlen / 16;
    size_t dx16rem = dlen % 16;

    alignas(16) __m128i tmps[32];
    alignas(16) __m128i data[32];

    alignas(16) __m128i inc = _mm_setr_epi32(1, 0, 0, 0);
    alignas(16) __m128i inc16 = _mm_setr_epi32(16, 0, 0, 0);

    alignas(16) __m128i ctr[32] = {
        _mm_xor_si128(_mm_setr_epi32(1, 0, 0, 0), S[0]),
        _mm_xor_si128(_mm_setr_epi32(0, 0, 0, 0), S[1]),
        _mm_xor_si128(_mm_setr_epi32(2, 0, 0, 0), S[0]),
        _mm_xor_si128(_mm_setr_epi32(0, 0, 0, 0), S[1]),
        _mm_xor_si128(_mm_setr_epi32(3, 0, 0, 0), S[0]),
        _mm_xor_si128(_mm_setr_epi32(0, 0, 0, 0), S[1]),
        _mm_xor_si128(_mm_setr_epi32(4, 0, 0, 0), S[0]),
        _mm_xor_si128(_mm_setr_epi32(0, 0, 0, 0), S[1]),
        _mm_xor_si128(_mm_setr_epi32(5, 0, 0, 0), S[0]),
        _mm_xor_si128(_mm_setr_epi32(0, 0, 0, 0), S[1]),
        _mm_xor_si128(_mm_setr_epi32(6, 0, 0, 0), S[0]),
        _mm_xor_si128(_mm_setr_epi32(0, 0, 0, 0), S[1]),
        _mm_xor_si128(_mm_setr_epi32(7, 0, 0, 0), S[0]),
        _mm_xor_si128(_mm_setr_epi32(0, 0, 0, 0), S[1]),
        _mm_xor_si128(_mm_setr_epi32(8, 0, 0, 0), S[0]),
        _mm_xor_si128(_mm_setr_epi32(0, 0, 0, 0), S[1]),

        _mm_xor_si128(_mm_setr_epi32(9, 0, 0, 0), S[0]),
        _mm_xor_si128(_mm_setr_epi32(0, 0, 0, 0), S[1]),
        _mm_xor_si128(_mm_setr_epi32(10, 0, 0, 0), S[0]),
        _mm_xor_si128(_mm_setr_epi32(0, 0, 0, 0), S[1]),
        _mm_xor_si128(_mm_setr_epi32(11, 0, 0, 0), S[0]),
        _mm_xor_si128(_mm_setr_epi32(0, 0, 0, 0), S[1]),
        _mm_xor_si128(_mm_setr_epi32(12, 0, 0, 0), S[0]),
        _mm_xor_si128(_mm_setr_epi32(0, 0, 0, 0), S[1]),
        _mm_xor_si128(_mm_setr_epi32(13, 0, 0, 0), S[0]),
        _mm_xor_si128(_mm_setr_epi32(0, 0, 0, 0), S[1]),
        _mm_xor_si128(_mm_setr_epi32(14, 0, 0, 0), S[0]),
        _mm_xor_si128(_mm_setr_epi32(0, 0, 0, 0), S[1]),
        _mm_xor_si128(_mm_setr_epi32(15, 0, 0, 0), S[0]),
        _mm_xor_si128(_mm_setr_epi32(0, 0, 0, 0), S[1]),
        _mm_xor_si128(_mm_setr_epi32(16, 0, 0, 0), S[0]),
        _mm_xor_si128(_mm_setr_epi32(0, 0, 0, 0), S[1]),
    };

    for (size_t i = 0; i < dx16len; i++)
    {
        loadx32((__m128i *)P + 32 * i, data);

        addkey256x16(simpiractx.keys, ctr, tmps);
        simpira_b2x16_128(simpiractx, tmps);
        addkey256x16(simpiractx.keys, tmps, tmps);
        xorx32_1wise(tmps, data, tmps);

        storex32((__m128i *)C + 32 * i, tmps);
        addx16_bfix_2wise(ctr, inc16, ctr);
    }
    for (size_t i = 0; i < dx16rem; i++)
    {
        loadx2((__m128i *)P + 32 * dx16len + 2 * i, data);
        addkey256(simpiractx.keys, ctr, tmps);
        simpira_b2x1_128(simpiractx, tmps);
        addkey256(simpiractx.keys, tmps, tmps);
        xorx2_1wise(tmps, data, tmps);
        storex2((__m128i *)C + 32 * dx16len + 2 * i, tmps);
        addx1_bfix_2wise(ctr, inc, ctr);
    }
}

static inline void simpiraxctrx4(simpira_context simpiractx, __m128i *S, uint8_t *P, size_t Plen, uint8_t *C)
{
    size_t blen = Plen / 16;
    size_t dlen = blen / 2;
    size_t dx4len = dlen / 4;
    size_t dx4rem = dlen % 4;

    alignas(16) __m128i tmps[32];
    alignas(16) __m128i data[32];

    alignas(16) __m128i inc = _mm_setr_epi32(1, 0, 0, 0);
    alignas(16) __m128i inc4 = _mm_setr_epi32(4, 0, 0, 0);

    alignas(16) __m128i ctr[8] = {
        _mm_xor_si128(_mm_setr_epi32(1, 0, 0, 0), S[0]),
        _mm_xor_si128(_mm_setr_epi32(0, 0, 0, 0), S[1]),
        _mm_xor_si128(_mm_setr_epi32(2, 0, 0, 0), S[0]),
        _mm_xor_si128(_mm_setr_epi32(0, 0, 0, 0), S[1]),
        _mm_xor_si128(_mm_setr_epi32(3, 0, 0, 0), S[0]),
        _mm_xor_si128(_mm_setr_epi32(0, 0, 0, 0), S[1]),
        _mm_xor_si128(_mm_setr_epi32(4, 0, 0, 0), S[0]),
        _mm_xor_si128(_mm_setr_epi32(0, 0, 0, 0), S[1]),
    };

    for (size_t i = 0; i < dx4len; i++)
    {
        loadx8((__m128i *)P + 8 * i, data);

        addkey256x4(simpiractx.keys, ctr, tmps);
        simpira_b2x4_128(simpiractx, tmps)
            addkey256x4(simpiractx.keys, tmps, tmps);
        xorx8_1wise(tmps, data, tmps);

        storex8((__m128i *)C + 8 * i, tmps);
        addx4_bfix_2wise(ctr, inc4, ctr);
    }
    for (size_t i = 0; i < dx4rem; i++)
    {
        loadx2((__m128i *)P + 8 * dx4len + 2 * i, data);
        addkey256(simpiractx.keys, ctr, tmps);
        simpira_b2x1_128(simpiractx, tmps);
        addkey256(simpiractx.keys, tmps, tmps);
        xorx2_1wise(tmps, data, tmps);
        storex2((__m128i *)C + 8 * dx4len + 2 * i, tmps);
        addx1_bfix_2wise(ctr, inc, ctr);
    }
}