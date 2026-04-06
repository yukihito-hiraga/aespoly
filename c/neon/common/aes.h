#pragma once
#include "headers.h"

typedef struct _aes_context
{
	alignas(64) uint8x16x4_t keys[12];
	alignas(32) uint8x16x2_t keys256[12];
	alignas(16) uint8x16_t keys128[12];
} aes_context;

const uint8_t invsr_rtw_raw[16] = {
	0x0, 0xd, 0xa, 0x7,
	0x4, 0x1, 0xe, 0xb,
	0x8, 0x5, 0x2, 0xf,
	0x9, 0x6, 0x3, 0xc};

uint8_t rcon[10][16];

static inline uint8x16_t subrotword(uint8x16_t w)
{
	uint8x16_t tmp = vaeseq_u8(w, vdupq_n_u8(0));
	tmp = vqtbl1q_u8(tmp, vld1q_u8(invsr_rtw_raw));
	return tmp;
}

static inline uint8x16_t shiftadd(uint8x16_t w, uint8x16_t s)
{
	uint8x16_t tmp;
	tmp = veorq_u8(w, vextq_u8(s, vdupq_n_u8(0), 12));
	tmp = veorq_u8(tmp, vextq_u8(vdupq_n_u8(0), tmp, 12));
	tmp = veorq_u8(tmp, vextq_u8(vdupq_n_u8(0), tmp, 12));
	tmp = veorq_u8(tmp, vextq_u8(vdupq_n_u8(0), tmp, 12));
	return tmp;
}

static inline uint8x16_t aesenc128(uint8x16_t pt, uint8x16_t *keys)
{
	alignas(16) uint8x16_t state = pt;

	state = vaeseq_u8(state, keys[0]);
	state = vaesmcq_u8(state);

	state = vaeseq_u8(state, keys[1]);
	state = vaesmcq_u8(state);

	state = vaeseq_u8(state, keys[2]);
	state = vaesmcq_u8(state);

	state = vaeseq_u8(state, keys[3]);
	state = vaesmcq_u8(state);

	state = vaeseq_u8(state, keys[4]);
	state = vaesmcq_u8(state);

	state = vaeseq_u8(state, keys[5]);
	state = vaesmcq_u8(state);

	state = vaeseq_u8(state, keys[6]);
	state = vaesmcq_u8(state);

	state = vaeseq_u8(state, keys[7]);
	state = vaesmcq_u8(state);

	state = vaeseq_u8(state, keys[8]);
	state = vaesmcq_u8(state);

	state = vaeseq_u8(state, keys[9]);

	state = veorq_u8(state, keys[10]);

	return state;
}

static inline uint8x16_t aesdec128(uint8x16_t ct, uint8x16_t *keys)
{
	alignas(16) uint8x16_t state = ct;

	state = vaesdq_u8(state, keys[10]);
	state = vaesimcq_u8(state);

	state = vaesdq_u8(state, keys[9]);
	state = vaesimcq_u8(state);

	state = vaesdq_u8(state, keys[8]);
	state = vaesimcq_u8(state);

	state = vaesdq_u8(state, keys[7]);
	state = vaesimcq_u8(state);

	state = vaesdq_u8(state, keys[6]);
	state = vaesimcq_u8(state);

	state = vaesdq_u8(state, keys[5]);
	state = vaesimcq_u8(state);

	state = vaesdq_u8(state, keys[4]);
	state = vaesimcq_u8(state);

	state = vaesdq_u8(state, keys[3]);
	state = vaesimcq_u8(state);

	state = vaesdq_u8(state, keys[2]);
	state = vaesimcq_u8(state);

	state = vaesdq_u8(state, keys[1]);

	state = veorq_u8(state, keys[0]);

	return state;
}

static inline uint8x16_t aeskeyex128(uint8x16_t key, uint8_t rcon)
{
	alignas(16) uint8x16_t tmp;
	alignas(16) uint8x16_t rcon_v = vdupq_n_u8(0);
	rcon_v = vsetq_lane_u8(rcon, rcon_v, 12);
	tmp = veorq_u8(rcon_v, subrotword(key));
	tmp = shiftadd(key, tmp);
	return tmp;
}

static inline void aesinit128(aes_context *aesctx, uint8_t *key)
{
	aesctx->keys128[0] = vld1q_u8(key);
	aesctx->keys128[1] = aeskeyex128(aesctx->keys128[0], 0x01);
	aesctx->keys128[2] = aeskeyex128(aesctx->keys128[1], 0x02);
	aesctx->keys128[3] = aeskeyex128(aesctx->keys128[2], 0x04);
	aesctx->keys128[4] = aeskeyex128(aesctx->keys128[3], 0x08);
	aesctx->keys128[5] = aeskeyex128(aesctx->keys128[4], 0x10);
	aesctx->keys128[6] = aeskeyex128(aesctx->keys128[5], 0x20);
	aesctx->keys128[7] = aeskeyex128(aesctx->keys128[6], 0x40);
	aesctx->keys128[8] = aeskeyex128(aesctx->keys128[7], 0x80);
	aesctx->keys128[9] = aeskeyex128(aesctx->keys128[8], 0x1B);
	aesctx->keys128[10] = aeskeyex128(aesctx->keys128[9], 0x36);
}

#define aes_roundx1(key, src, dst)           \
	{                                        \
		(dst)[0] = vaeseq_u8((src)[0], key); \
		(dst)[0] = vaesmcq_u8((dst)[0]);     \
	}

#define aes_lastroundx1(key, src, dst)       \
	{                                        \
		(dst)[0] = vaeseq_u8((src)[0], key); \
	}

#define aes_addkeyx1(key, src, dst)         \
	{                                       \
		(dst)[0] = veorq_u8((src)[0], key); \
	}

#define aes_fx2(f, key, src, dst) \
	{                             \
		f(key, src, dst);         \
		f(key, src + 1, dst + 1); \
	}

#define aes_fx4(f, key, src, dst)          \
	{                                      \
		aes_fx2(f, key, src, dst);         \
		aes_fx2(f, key, src + 2, dst + 2); \
	}

#define aes_fx8(f, key, src, dst)          \
	{                                      \
		aes_fx4(f, key, src, dst);         \
		aes_fx4(f, key, src + 4, dst + 4); \
	}

#define aes_fx16(f, key, dst, src)         \
	{                                      \
		aes_fx8(f, key, src, dst);         \
		aes_fx8(f, key, src + 8, dst + 8); \
	}

#define aes_roundx2(key, src, dst) aes_fx2(aes_roundx1, key, src, dst)

#define aes_lastroundx2(key, src, dst) aes_fx2(aes_lastroundx1, key, src, dst)

#define aes_addkeyx2(key, src, dst) aes_fx2(aes_addkeyx1, key, src, dst)

#define aes_roundx4(key, src, dst) aes_fx4(aes_roundx1, key, src, dst)

#define aes_lastroundx4(key, src, dst) aes_fx4(aes_lastroundx1, key, src, dst)

#define aes_addkeyx4(key, src, dst) aes_fx4(aes_addkeyx1, key, src, dst)

#define aes_roundx8(key, src, dst) aes_fx8(aes_roundx1, key, src, dst)

#define aes_lastroundx8(key, src, dst) aes_fx8(aes_lastroundx1, key, src, dst)

#define aes_addkeyx8(key, src, dst) aes_fx8(aes_addkeyx1, key, src, dst)

#define aes_roundx16(key, src, dst) aes_fx16(aes_roundx1, key, src, dst)

#define aes_lastroundx16(key, src, dst) aes_fx16(aes_lastroundx1, key, src, dst)

#define aes_addkeyx16(key, src, dst) aes_fx16(aes_addkeyx1, key, src, dst)

#define aes_allrounds(aes_addkey_f, aes_round_f, aes_lastround_f, keys, src, dst) \
	{                                                                             \
		aes_round_f(keys[0], src, dst);                                           \
		aes_round_f(keys[1], dst, dst);                                           \
		aes_round_f(keys[2], dst, dst);                                           \
		aes_round_f(keys[3], dst, dst);                                           \
		aes_round_f(keys[4], dst, dst);                                           \
		aes_round_f(keys[5], dst, dst);                                           \
		aes_round_f(keys[6], dst, dst);                                           \
		aes_round_f(keys[7], dst, dst);                                           \
		aes_round_f(keys[8], dst, dst);                                           \
		aes_lastround_f(keys[9], dst, dst);                                       \
		aes_addkey_f(keys[10], dst, dst);                                         \
	}

#define aesx1(keys, src, dst) aes_allrounds(aes_addkeyx1, aes_roundx1, aes_lastroundx1, keys, src, dst)

#define aesx2(keys, src, dst) aes_allrounds(aes_addkeyx2, aes_roundx2, aes_lastroundx2, keys, src, dst)

#define aesx4(keys, src, dst) aes_allrounds(aes_addkeyx4, aes_roundx4, aes_lastroundx4, keys, src, dst)

#define aesx8(keys, src, dst) aes_allrounds(aes_addkeyx8, aes_roundx8, aes_lastroundx8, keys, src, dst)

#define aesx16(keys, src, dst) aes_allrounds(aes_addkeyx16, aes_roundx16, aes_lastroundx16, keys, src, dst)
