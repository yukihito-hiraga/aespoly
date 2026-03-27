#pragma once
#include "headers.h"

#define __m128i uint8x16_t
#define __m512i uint8x16x4_t

#define _mm_clmulepi64_si128(x, y, imm)                                                           \
	__extension__({                                                                               \
		poly64x2_t p1 = vreinterpretq_p64_u8(x);                                                  \
		poly64x2_t p2 = vreinterpretq_p64_u8(y);                                                  \
		poly128_t p = vmull_p64(vgetq_lane_p64(p1, imm & 1), vgetq_lane_p64(p2, imm >> 4)); \
		vreinterpretq_u8_p128(p);                                                                 \
	})

static inline __m128i _mm_xor_si128(__m128i x, __m128i y)
{
	return veorq_u8(x, y);
}

static inline __m128i _mm_setr_epi8(uint8_t i0, uint8_t i1, uint8_t i2, uint8_t i3, uint8_t i4, uint8_t i5, uint8_t i6, uint8_t i7, uint8_t i8, uint8_t i9, uint8_t i10, uint8_t i11, uint8_t i12, uint8_t i13, uint8_t i14, uint8_t i15)
{
	uint8_t tmp[16] = {i0, i1, i2, i3, i4, i5, i6, i7, i8, i9, i10, i11, i12, i13, i14, i15};
	return vld1q_u8(tmp);
}

static inline __m128i _mm_set_epi8(uint8_t i15, uint8_t i14, uint8_t i13, uint8_t i12, uint8_t i11, uint8_t i10, uint8_t i9, uint8_t i8, uint8_t i7, uint8_t i6, uint8_t i5, uint8_t i4, uint8_t i3, uint8_t i2, uint8_t i1, uint8_t i0)
{
	uint8_t tmp[16] = {i0, i1, i2, i3, i4, i5, i6, i7, i8, i9, i10, i11, i12, i13, i14, i15};
	return vld1q_u8(tmp);
}

static inline __m128i _mm_setr_epi16(uint16_t i0, uint16_t i1, uint16_t i2, uint16_t i3, uint16_t i4, uint16_t i5, uint16_t i6, uint16_t i7)
{
	uint16_t tmp[8] = {i0, i1, i2, i3, i4, i5, i6, i7};
	return vreinterpretq_u8_u16(vld1q_u16(tmp));
}

static inline __m128i _mm_setr_epi32(uint32_t i0, uint32_t i1, uint32_t i2, uint32_t i3)
{
	uint32_t tmp[4] = {i0, i1, i2, i3};
	return vreinterpretq_u8_u32(vld1q_u32(tmp));
}

#define _m_from_int64(x) x

static inline __m128i _mm_setr_epi64(uint64_t i0, uint64_t i1)
{
	uint64_t tmp[2] = {i0, i1};
	return vreinterpretq_u8_u64(vld1q_u64(tmp));
}

static inline __m128i _mm_setzero_si128()
{
	return vdupq_n_u8(0);
}

static inline __m128i _mm_loadu_si128(__m128i *p)
{
	return vld1q_u8((uint8_t *)(p));
}

static inline __m128i _mm_load_si128(__m128i *p)
{
	return vld1q_u8((uint8_t *)(p));
}

static inline void _mm_storeu_si128(__m128i *p, __m128i x)
{
	vst1q_u8((uint8_t *)(p), x);
}

static inline void _mm_store_si128(__m128i *p, __m128i x)
{
	vst1q_u8((uint8_t *)(p), x);
}

#define _mm_bsrli_si128(x, imm)                \
	__extension__({                            \
		vextq_u8(x, _mm_setzero_si128(), imm); \
	})

#define _mm_bslli_si128(x, imm)                     \
	__extension__({                                 \
		vextq_u8(_mm_setzero_si128(), x, 16 - imm); \
	})

#define _mm_srli_si128(x, imm)                 \
	__extension__({                            \
		vextq_u8(x, _mm_setzero_si128(), imm); \
	})

#define _mm_slli_si128(x, imm)                      \
	__extension__({                                 \
		vextq_u8(_mm_setzero_si128(), x, 16 - imm); \
	})

#define _mm_slli_epi32(x, imm)                                            \
	__extension__({                                                       \
		vreinterpretq_u8_u32(vqshlq_n_u32(vreinterpretq_u32_u8(x), imm)); \
	})

#define _mm_srli_epi32(x, imm)                                            \
	__extension__({                                                       \
		vreinterpretq_u8_u32(vshrq_n_u32(vreinterpretq_u32_u8(x), imm)); \
	})

#define _mm_srli_epi64(x, imm)                                            \
	__extension__({                                                       \
		vreinterpretq_u8_u64(vshrq_n_u64(vreinterpretq_u64_u8(x), imm)); \
	})

#define _mm_slli_epi64(x, imm)                                            \
	__extension__({                                                       \
		vreinterpretq_u8_u64(vqshlq_n_u64(vreinterpretq_u64_u8(x), imm)); \
	})

#define _mm_add_epi16(x, y) vreinterpretq_u8_u16(vaddq_u16(vreinterpretq_u16_u8(x), vreinterpretq_u16_u8(y)))


#define _mm_clflush(p)                                 \
	__extension__({                                    \
		uint64_t addr = (uintptr_t)(b.head + _i * 64); \
		__asm__ __volatile__(   \
		"dc civac, %0\n\t" :                           \
								: "r"(addr)            \
								:);                    \
	})

static inline __m128i _mm_shuffle_epi32(__m128i x, const int imm)
{
	const int imm0 = imm & 0b11;
	const int imm1 = (imm & 0b1100) >> 2;
	const int imm2 = (imm & 0b110000) >> 4;
	const int imm3 = (imm & 0b11000000) >> 6;
	__m128i y = _mm_setr_epi8(
		imm0 * 4, imm0 * 4 + 1, imm0 * 4 + 2, imm0 * 4 + 3,
		imm1 * 4, imm1 * 4 + 1, imm1 * 4 + 2, imm1 * 4 + 3,
		imm2 * 4, imm2 * 4 + 1, imm2 * 4 + 2, imm2 * 4 + 3,
		imm3 * 4, imm3 * 4 + 1, imm3 * 4 + 2, imm3 * 4 + 3);

	return vqtbl1q_u8(x, y);
}

static inline __m128i _mm_shuffle_epi8(__m128i x, __m128i y)
{
	return vqtbl1q_u8(x, y);
}

static inline __m128i _mm_add_epi64(__m128i x, __m128i y)
{
	return vreinterpretq_u8_u64(vaddq_u64(vreinterpretq_u64_u8(x), vreinterpretq_u64_u8(y)));
}

static inline __m128i _mm_aesenc_si128(__m128i pt, __m128i k)
{
	__m128i res = vaeseq_u8(pt, k);
	return vaesmcq_u8(res);
}

static inline __m128i _mm_aesenclast_si128(__m128i pt, __m128i k)
{
	__m128i res = vaeseq_u8(pt, k);
	return res;
}

static inline __m128i _mm_and_si128(__m128i x, __m128i y)
{
	return vandq_u8(x, y);
}

#define _mm_set_pi64x(x) x

#define _mm_setzero_si64() 0

#define reverse128(x)                                                                                               \
	__extension__({                                                                                                 \
		x = vrev64q_u8(x);                                                                                          \
		x = _mm_setr_epi64(vgetq_lane_u64(vreinterpretq_u64_u8(x), 1), vgetq_lane_u64(vreinterpretq_u64_u8(x), 0)); \
		x = vrbitq_u8(x);                                                                                           \
		x;                                                                                                          \
	})

#define __tzcnt_u64(x)      \
	__extension__({         \
		__clzl(__rbitl(x)); \
	})

#define _tzcnt_u64(x) __tzcnt_u64(x)

#define __dsb(x)              \
	__extension__({           \
		__asm__ __volatile__( \
			"dsb 0\n\t" ::);  \
	})

#define __dmb(x)              \
	__extension__({           \
		__asm__ __volatile__( \
			"dmb 0\n\t" ::);  \
	})

static inline __m128i _mm_blendv_epi8(__m128i x, __m128i y, __m128i mask)
{
	return vbslq_u8(x, y, mask);
}