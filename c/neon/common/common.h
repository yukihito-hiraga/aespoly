#pragma once
#include "wrapper.h"
#include "aes.h"
#include "headers.h"
#include "measurement.h"
#include "test.h"

extern __m512i GLOBAL_POLY512;
extern __m512i global_aes_key512[20];

const char *result_dir = "/home/ubuntu/works/result/";

double global_min_cpb, global_avg_cpb;
char fname[200];

const size_t size_set_len = 16;

static inline __m128i ozp128(size_t len, const uint8_t *data)
{
	uint8_t padded[20];
	memset(padded, 0, 20);
	memcpy(padded, data, len);
	padded[len] = 0x80;
	return _mm_loadu_si128((__m128i *)padded);
}

void bench_set(size_t num, bool clflush, void (*measure)(bool, size_t, size_t), const char *name)
{
	FILE *fp_min;
	FILE *fp_avg;
	sprintf(fname, "%s%s_num%ld_clflush%d_min.dat", result_dir, name, num, clflush);
	fp_min = fopen(fname, "w");
	sprintf(fname, "%s%s_num%ld_clflush%d_avg.dat", result_dir, name, num, clflush);
	fp_avg = fopen(fname, "w");

	for (size_t i = 1; i < size_set_len; i++)
	{
		size_t len = 10000 * i;
		measure(clflush, len, num);
		fprintf(fp_min, "%ld, %f\n", len, global_min_cpb);
		fprintf(fp_avg, "%ld, %f\n", len, global_avg_cpb);
	}
	fclose(fp_min);
	fclose(fp_avg);
}

void bench_set3(size_t num, bool clflush, void (*measure)(bool, size_t, size_t, size_t), const char *name, size_t arg1)
{
	FILE *fp_min;
	FILE *fp_avg;
	sprintf(fname, "%s%s_num%ld_clflush%d_min.dat", result_dir, name, num, clflush);
	fp_min = fopen(fname, "w");
	sprintf(fname, "%s%s_num%ld_clflush%d_avg.dat", result_dir, name, num, clflush);
	fp_avg = fopen(fname, "w");

	for (size_t i = 1; i < size_set_len; i++)
	{
		size_t len = 10000 * i;
		measure(clflush, arg1, len, num);
		fprintf(fp_min, "%ld, %f\n", len, global_min_cpb);
		fprintf(fp_avg, "%ld, %f\n", len, global_avg_cpb);
	}
	fclose(fp_min);
	fclose(fp_avg);
}

void bench_set4(size_t num, bool clflush, void (*measure)(bool, size_t, size_t, size_t, size_t), const char *name, size_t arg1, size_t arg2)
{
	FILE *fp_min;
	FILE *fp_avg;
	sprintf(fname, "%s%s_num%ld_clflush%d_min.dat", result_dir, name, num, clflush);
	fp_min = fopen(fname, "w");
	sprintf(fname, "%s%s_num%ld_clflush%d_avg.dat", result_dir, name, num, clflush);
	fp_avg = fopen(fname, "w");

	for (size_t i = 1; i < size_set_len; i++)
	{
		size_t len = 10000 * i;
		measure(clflush, arg1, arg2, len, num);
		fprintf(fp_min, "%ld, %f\n", len, global_min_cpb);
		fprintf(fp_avg, "%ld, %f\n", len, global_avg_cpb);
	}
	fclose(fp_min);
	fclose(fp_avg);
}

#define loadx2(P, tmps)                       \
	{                                         \
		*(tmps + 0) = _mm_loadu_si128(P + 0); \
		*(tmps + 1) = _mm_loadu_si128(P + 1); \
	}

#define storex2(C, tmps)                      \
	{                                         \
		_mm_storeu_si128(C + 0, *(tmps + 0)); \
		_mm_storeu_si128(C + 1, *(tmps + 1)); \
	}

#define loadx4(P, tmps)          \
	{                            \
		loadx2(P, tmps);         \
		loadx2(P + 2, tmps + 2); \
	}

#define storex4(C, tmps)          \
	{                             \
		storex2(C, tmps);         \
		storex2(C + 2, tmps + 2); \
	}

#define loadx8(P, tmps)          \
	{                            \
		loadx4(P, tmps);         \
		loadx4(P + 4, tmps + 4); \
	}

#define storex8(C, tmps)          \
	{                             \
		storex4(C, tmps);         \
		storex4(C + 4, tmps + 4); \
	}

#define loadx16(P, tmps)         \
	{                            \
		loadx8(P, tmps);         \
		loadx8(P + 8, tmps + 8); \
	}

#define storex16(C, tmps)         \
	{                             \
		storex8(C, tmps);         \
		storex8(C + 8, tmps + 8); \
	}

#define loadx32(P, tmps)            \
	{                               \
		loadx16(P, tmps);           \
		loadx16(P + 16, tmps + 16); \
	}

#define storex32(C, tmps)            \
	{                                \
		storex16(C, tmps);           \
		storex16(C + 16, tmps + 16); \
	}

#define loadx64(P, tmps)            \
	{                               \
		loadx32(P, tmps);           \
		loadx32(P + 32, tmps + 32); \
	}

#define storex64(C, tmps)            \
	{                                \
		storex32(C, tmps);           \
		storex32(C + 32, tmps + 32); \
	}

#define copyx2(S, D)     \
	{                    \
		(D)[0] = (S)[0]; \
		(D)[1] = (S)[1]; \
	}

#define copyx4(S, D)          \
	{                         \
		copyx2(S, D);         \
		copyx2(S + 2, D + 2); \
	}

#define copyx8(S, D)          \
	{                         \
		copyx4(S, D);         \
		copyx4(S + 4, D + 4); \
	}

#define copyx16(S, D)         \
	{                         \
		copyx8(S, D);         \
		copyx8(S + 8, D + 8); \
	}

#define copyx32(S, D)            \
	{                            \
		copyx16(S, D);           \
		copyx16(S + 16, D + 16); \
	}

#define xorx2_1wise(a, b, c)                    \
	{                                           \
		(c)[0] = _mm_xor_si128((a)[0], (b)[0]); \
		(c)[1] = _mm_xor_si128((a)[1], (b)[1]); \
	}

#define xorx4_1wise(a, b, c)              \
	{                                     \
		xorx2_1wise(a, b, c);             \
		xorx2_1wise(a + 2, b + 2, c + 2); \
	}

#define xorx8_1wise(a, b, c)              \
	{                                     \
		xorx4_1wise(a, b, c);             \
		xorx4_1wise(a + 4, b + 4, c + 4); \
	}

#define xorx16_1wise(a, b, c)             \
	{                                     \
		xorx8_1wise(a, b, c);             \
		xorx8_1wise(a + 8, b + 8, c + 8); \
	}

#define xorx32_1wise(a, b, c)                 \
	{                                         \
		xorx16_1wise(a, b, c);                \
		xorx16_1wise(a + 16, b + 16, c + 16); \
	}

#define addx2_1wise(a, b, c)                    \
	{                                           \
		(c)[0] = _mm_add_epi64((a)[0], (b)[0]); \
		(c)[1] = _mm_add_epi64((a)[1], (b)[1]); \
	}

#define addx4_1wise(a, b, c)              \
	{                                     \
		addx2_1wise(a, b, c);             \
		addx2_1wise(a + 2, b + 2, c + 2); \
	}

#define addx8_1wise(a, b, c)              \
	{                                     \
		addx4_1wise(a, b, c);             \
		addx4_1wise(a + 4, b + 4, c + 4); \
	}

#define addx16_1wise(a, b, c)             \
	{                                     \
		addx8_1wise(a, b, c);             \
		addx8_1wise(a + 8, b + 8, c + 8); \
	}

#define addx32_1wise(a, b, c)                 \
	{                                         \
		addx16_1wise(a, b, c);                \
		addx16_1wise(a + 16, b + 16, c + 16); \
	}

#define xorx2_bfix(a, b, c)                \
	{                                      \
		(c)[0] = _mm_xor_si128((a)[0], b); \
		(c)[1] = _mm_xor_si128((a)[1], b); \
	}

#define xorx4_bfix(a, b, c)          \
	{                                \
		xorx2_bfix(a, b, c);         \
		xorx2_bfix(a + 2, b, c + 2); \
	}

#define xorx8_bfix(a, b, c)          \
	{                                \
		xorx4_bfix(a, b, c);         \
		xorx4_bfix(a + 4, b, c + 4); \
	}

#define xorx16_bfix(a, b, c)         \
	{                                \
		xorx8_bfix(a, b, c);         \
		xorx8_bfix(a + 8, b, c + 8); \
	}

#define addx2_bfix(a, b, c)                \
	{                                      \
		(c)[0] = _mm_add_epi64((a)[0], b); \
		(c)[1] = _mm_add_epi64((a)[1], b); \
	}

#define addx4_bfix(a, b, c)          \
	{                                \
		addx2_bfix(a, b, c);         \
		addx2_bfix(a + 2, b, c + 2); \
	}

#define addx8_bfix(a, b, c)          \
	{                                \
		addx4_bfix(a, b, c);         \
		addx4_bfix(a + 4, b, c + 4); \
	}

#define addx16_bfix(a, b, c)         \
	{                                \
		addx8_bfix(a, b, c);         \
		addx8_bfix(a + 8, b, c + 8); \
	}

#define addx1_bfix_2wise(a, b, c)          \
	{                                      \
		(c)[0] = _mm_add_epi64((a)[0], b); \
	}

#define addx2_bfix_2wise(a, b, c)          \
	{                                      \
		addx1_bfix_2wise(a, b, c);         \
		addx1_bfix_2wise(a + 2, b, c + 2); \
	}

#define addx4_bfix_2wise(a, b, c)          \
	{                                      \
		addx2_bfix_2wise(a, b, c);         \
		addx2_bfix_2wise(a + 4, b, c + 4); \
	}

#define addx8_bfix_2wise(a, b, c)          \
	{                                      \
		addx4_bfix_2wise(a, b, c);         \
		addx4_bfix_2wise(a + 8, b, c + 8); \
	}

#define addx16_bfix_2wise(a, b, c)           \
	{                                        \
		addx8_bfix_2wise(a, b, c);           \
		addx8_bfix_2wise(a + 16, b, c + 16); \
	}

#define addx1_bfixodd_2wise(a, b, c)       \
	{                                      \
		(c)[1] = _mm_add_epi64((a)[1], b); \
	}

#define addx2_bfixodd_2wise(a, b, c)          \
	{                                         \
		addx1_bfixodd_2wise(a, b, c);         \
		addx1_bfixodd_2wise(a + 2, b, c + 2); \
	}

#define addx4_bfixodd_2wise(a, b, c)          \
	{                                         \
		addx2_bfixodd_2wise(a, b, c);         \
		addx2_bfixodd_2wise(a + 4, b, c + 4); \
	}

#define addx8_bfixodd_2wise(a, b, c)          \
	{                                         \
		addx4_bfixodd_2wise(a, b, c);         \
		addx4_bfixodd_2wise(a + 8, b, c + 8); \
	}

#define addx16_bfixodd_2wise(a, b, c)           \
	{                                           \
		addx8_bfixodd_2wise(a, b, c);           \
		addx8_bfixodd_2wise(a + 16, b, c + 16); \
	}

#define addkey256(keys, pt, tmps)                      \
	{                                                  \
		(tmps)[0] = _mm_xor_si128((pt)[0], (keys)[0]); \
		(tmps)[1] = _mm_xor_si128((pt)[1], (keys)[1]); \
	}

#define addkey256x2(keys, pt, tmps)        \
	{                                      \
		addkey256(keys, pt, tmps);         \
		addkey256(keys, pt + 2, tmps + 2); \
	}

#define addkey256x4(keys, pt, tmps)          \
	{                                        \
		addkey256x2(keys, pt, tmps);         \
		addkey256x2(keys, pt + 4, tmps + 4); \
	}

#define addkey256x8(keys, pt, tmps)          \
	{                                        \
		addkey256x4(keys, pt, tmps);         \
		addkey256x4(keys, pt + 8, tmps + 8); \
	}

#define addkey256x16(keys, pt, tmps)           \
	{                                          \
		addkey256x8(keys, pt, tmps);           \
		addkey256x8(keys, pt + 16, tmps + 16); \
	}

#define addkey256x32(keys, pt, tmps)            \
	{                                           \
		addkey256x16(keys, pt, tmps);           \
		addkey256x16(keys, pt + 32, tmps + 32); \
	}

#define addkey256x64(keys, pt, tmps)            \
	{                                           \
		addkey256x32(keys, pt, tmps);           \
		addkey256x32(keys, pt + 64, tmps + 64); \
	}

#define addctr256(keys, pt, tmps)                      \
	{                                                  \
		(tmps)[0] = _mm_add_epi64((pt)[0], (keys)[0]); \
		(tmps)[1] = _mm_add_epi64((pt)[1], (keys)[1]); \
	}

#define addctr256x2(keys, pt, tmps)        \
	{                                      \
		addctr256(keys, pt, tmps);         \
		addctr256(keys, pt + 2, tmps + 2); \
	}

#define addctr256x4(keys, pt, tmps)          \
	{                                        \
		addctr256x2(keys, pt, tmps);         \
		addctr256x2(keys, pt + 4, tmps + 4); \
	}

#define addctr256x8(keys, pt, tmps)          \
	{                                        \
		addctr256x4(keys, pt, tmps);         \
		addctr256x4(keys, pt + 8, tmps + 8); \
	}

#define addctr256x16(keys, pt, tmps)           \
	{                                          \
		addctr256x8(keys, pt, tmps);           \
		addctr256x8(keys, pt + 16, tmps + 16); \
	}

#define addctr256x32(keys, pt, tmps)            \
	{                                           \
		addctr256x16(keys, pt, tmps);           \
		addctr256x16(keys, pt + 32, tmps + 32); \
	}

#define addctr256x64(keys, pt, tmps)            \
	{                                           \
		addctr256x32(keys, pt, tmps);           \
		addctr256x32(keys, pt + 64, tmps + 64); \
	}

#define scatter_store_n2x1(C, tmps)         \
	{                                       \
		_mm_storeu_si128(C, (tmps)[0]);     \
		_mm_storeu_si128(C + 1, (tmps)[1]); \
	}

#define scatter_store_n2x2(C, tmps)          \
	{                                        \
		scatter_store_n2x1(C, tmps);         \
		scatter_store_n2x1(C + 4, tmps + 2); \
	}

#define scatter_store_n2x4(C, tmps)          \
	{                                        \
		scatter_store_n2x2(C, tmps);         \
		scatter_store_n2x2(C + 8, tmps + 4); \
	}

#define scatter_store_n2x8(C, tmps)           \
	{                                         \
		scatter_store_n2x4(C, tmps);          \
		scatter_store_n2x4(C + 16, tmps + 8); \
	}

#define scatter_store_n2x16(C, tmps)           \
	{                                          \
		scatter_store_n2x4(C, tmps);           \
		scatter_store_n2x4(C + 32, tmps + 16); \
	}

#define scatter_move_n2x1(src, dest) \
	{                                \
		(dest)[0] = (src)[0];        \
		(dest)[1] = (src)[1];        \
	}

#define scatter_move_n2x2(src, dest)          \
	{                                         \
		scatter_move_n2x1(src, dest);         \
		scatter_move_n2x1(src + 2, dest + 4); \
	}

#define scatter_move_n2x4(src, dest)          \
	{                                         \
		scatter_move_n2x2(src, dest);         \
		scatter_move_n2x2(src + 4, dest + 8); \
	}

#define scatter_move_n2x8(src, dest)           \
	{                                          \
		scatter_move_n2x4(src, dest);          \
		scatter_move_n2x4(src + 8, dest + 16); \
	}

#define scatter_move_n2x16(src, dest)           \
	{                                           \
		scatter_move_n2x8(src, dest);           \
		scatter_move_n2x8(src + 16, dest + 32); \
	}

#define gather_n2x1(src, dest) \
	{                          \
		(dest)[0] = (src)[0];  \
		(dest)[1] = (src)[1];  \
	}

#define gather_n2x2(src, dest)          \
	{                                   \
		gather_n2x1(src, dest);         \
		gather_n2x1(src + 4, dest + 2); \
	}

#define gather_n2x4(src, dest)          \
	{                                   \
		gather_n2x2(src, dest);         \
		gather_n2x2(src + 8, dest + 4); \
	}

#define gather_n2x8(src, dest)           \
	{                                    \
		gather_n2x4(src, dest);          \
		gather_n2x4(src + 16, dest + 8); \
	}

#define gather_n2x16(src, dest)           \
	{                                     \
		gather_n2x8(src, dest);           \
		gather_n2x8(src + 32, dest + 16); \
	}

#define gather_load_n2x1(P, tmps)           \
	{                                       \
		(tmps)[0] = _mm_loadu_si128(P);     \
		(tmps)[1] = _mm_loadu_si128(P + 1); \
	}

#define gather_load_n2x2(P, tmps)          \
	{                                      \
		gather_load_n2x1(P, tmps);         \
		gather_load_n2x1(P + 4, tmps + 2); \
	}

#define gather_load_n2x4(P, tmps)          \
	{                                      \
		gather_load_n2x2(P, tmps);         \
		gather_load_n2x2(P + 8, tmps + 4); \
	}

#define gather_load_n2x8(P, tmps)           \
	{                                       \
		gather_load_n2x4(P, tmps);          \
		gather_load_n2x4(P + 16, tmps + 8); \
	}

#define gather_load_n2x16(P, tmps)           \
	{                                        \
		gather_load_n2x8(P, tmps);           \
		gather_load_n2x8(P + 32, tmps + 16); \
	}

#define scatter_store_x1(C, tmps)       \
	{                                   \
		_mm_storeu_si128(C, (tmps)[0]); \
	}

#define scatter_store_x2(C, tmps)          \
	{                                      \
		scatter_store_x1(C, tmps);         \
		scatter_store_x1(C + 2, tmps + 1); \
	}

#define scatter_store_x4(C, tmps)          \
	{                                      \
		scatter_store_x2(C, tmps);         \
		scatter_store_x2(C + 4, tmps + 2); \
	}

#define scatter_store_x8(C, tmps)          \
	{                                      \
		scatter_store_x4(C, tmps);         \
		scatter_store_x4(C + 8, tmps + 4); \
	}

#define scatter_store_x16(C, tmps)          \
	{                                       \
		scatter_store_x4(C, tmps);          \
		scatter_store_x4(C + 16, tmps + 8); \
	}

#define scatter_move_x1(src, dest) \
	{                              \
		(dest)[0] = (src)[0];      \
	}

#define scatter_move_x2(src, dest)          \
	{                                       \
		scatter_move_x1(src, dest);         \
		scatter_move_x1(src + 1, dest + 2); \
	}

#define scatter_move_x4(src, dest)          \
	{                                       \
		scatter_move_x2(src, dest);         \
		scatter_move_x2(src + 2, dest + 4); \
	}

#define scatter_move_x8(src, dest)          \
	{                                       \
		scatter_move_x4(src, dest);         \
		scatter_move_x4(src + 4, dest + 8); \
	}

#define scatter_move_x16(src, dest)          \
	{                                        \
		scatter_move_x8(src, dest);          \
		scatter_move_x8(src + 8, dest + 16); \
	}

#define gather_x1(src, dest)  \
	{                         \
		(dest)[0] = (src)[0]; \
	}

#define gather_x2(src, dest)          \
	{                                 \
		gather_x1(src, dest);         \
		gather_x1(src + 2, dest + 1); \
	}

#define gather_x4(src, dest)          \
	{                                 \
		gather_x2(src, dest);         \
		gather_x2(src + 4, dest + 2); \
	}

#define gather_x8(src, dest)          \
	{                                 \
		gather_x4(src, dest);         \
		gather_x4(src + 8, dest + 4); \
	}

#define gather_x16(src, dest)          \
	{                                  \
		gather_x8(src, dest);          \
		gather_x8(src + 16, dest + 8); \
	}

#define gather_load_x1(P, tmps)         \
	{                                   \
		(tmps)[0] = _mm_loadu_si128(P); \
	}

#define gather_load_x2(P, tmps)          \
	{                                    \
		gather_load_x1(P, tmps);         \
		gather_load_x1(P + 2, tmps + 1); \
	}

#define gather_load_x4(P, tmps)          \
	{                                    \
		gather_load_x2(P, tmps);         \
		gather_load_x2(P + 4, tmps + 2); \
	}

#define gather_load_x8(P, tmps)          \
	{                                    \
		gather_load_x4(P, tmps);         \
		gather_load_x4(P + 8, tmps + 4); \
	}

#define gather_load_x16(P, tmps)          \
	{                                     \
		gather_load_x8(P, tmps);          \
		gather_load_x8(P + 16, tmps + 8); \
	}

#define setzero_x1(X)                 \
	{                                 \
		(X)[0] = _mm_setzero_si128(); \
	}

#define setzero_x2(X)      \
	{                      \
		setzero_x1(X);     \
		setzero_x1(X + 1); \
	}

#define setzero_x4(X)      \
	{                      \
		setzero_x2(X);     \
		setzero_x2(X + 2); \
	}

#define setzero_x8(X)      \
	{                      \
		setzero_x4(X);     \
		setzero_x4(X + 4); \
	}

#define setzero_x16(X)     \
	{                      \
		setzero_x8(X);     \
		setzero_x8(X + 8); \
	}

#define setzero_x32(X)       \
	{                        \
		setzero_x16(X);      \
		setzero_x16(X + 16); \
	}

#define setzero_x64(X)       \
	{                        \
		setzero_x32(X);      \
		setzero_x32(X + 32); \
	}

#define sum_x2(tmps, S)                  \
	{                                    \
		S = _mm_xor_si128((tmps)[0], S); \
		S = _mm_xor_si128((tmps)[1], S); \
	}

#define sum_x4(tmps, S)      \
	{                        \
		sum_x2(tmps, S);     \
		sum_x2(tmps + 2, S); \
	}

#define sum_x8(tmps, S)      \
	{                        \
		sum_x4(tmps, S);     \
		sum_x4(tmps + 4, S); \
	}

#define sum_x16(tmps, S)     \
	{                        \
		sum_x8(tmps, S);     \
		sum_x8(tmps + 8, S); \
	}

#define sum_n2x1(tmps, S)                        \
	{                                            \
		(S)[0] = _mm_xor_si128((tmps)[0], S[0]); \
		(S)[1] = _mm_xor_si128((tmps)[1], S[1]); \
	}

#define sum_n2x2(tmps, S)      \
	{                          \
		sum_n2x1(tmps, S);     \
		sum_n2x1(tmps + 2, S); \
	}

#define sum_n2x4(tmps, S)      \
	{                          \
		sum_n2x2(tmps, S);     \
		sum_n2x2(tmps + 4, S); \
	}

#define sum_n2x8(tmps, S)      \
	{                          \
		sum_n2x4(tmps, S);     \
		sum_n2x4(tmps + 8, S); \
	}

#define sum_n2x16(tmps, S)      \
	{                           \
		sum_n2x8(tmps, S);      \
		sum_n2x8(tmps + 16, S); \
	}

#define schoolbook_add128(reg, htbl_reg, tmps)                 \
	{                                                          \
		(tmps)[3] = _mm_clmulepi64_si128(reg, htbl_reg, 0x01); \
		(tmps)[2] = _mm_xor_si128((tmps)[2], (tmps)[3]);       \
		(tmps)[3] = _mm_clmulepi64_si128(reg, htbl_reg, 0x00); \
		(tmps)[0] = _mm_xor_si128((tmps)[0], (tmps)[3]);       \
		(tmps)[3] = _mm_clmulepi64_si128(reg, htbl_reg, 0x11); \
		(tmps)[1] = _mm_xor_si128((tmps)[1], (tmps)[3]);       \
		(tmps)[3] = _mm_clmulepi64_si128(reg, htbl_reg, 0x10); \
		(tmps)[2] = _mm_xor_si128((tmps)[2], (tmps)[3]);       \
	}

#define schoolbook_initialadd128(reg, htbl_reg, tmps)          \
	{                                                          \
		(tmps)[2] = _mm_clmulepi64_si128(reg, htbl_reg, 0x01); \
		(tmps)[0] = _mm_clmulepi64_si128(reg, htbl_reg, 0x00); \
		(tmps)[3] = _mm_clmulepi64_si128(reg, htbl_reg, 0x10); \
		(tmps)[1] = _mm_clmulepi64_si128(reg, htbl_reg, 0x11); \
		(tmps)[2] = _mm_xor_si128((tmps)[2], (tmps)[3]);       \
	}

#define mulinit_n2(data, htbl, tmps, n)                             \
	{                                                               \
		schoolbook_initialadd128((data)[n - 1], htbl[0], tmps);     \
		schoolbook_initialadd128((data)[n - 1], htbl[n], tmps + 4); \
	}

#define muladd_n2(data, htbl, tmps, n, i)                              \
	{                                                                  \
		schoolbook_add128((data)[n - 1 - (i)], htbl[i], tmps);         \
		schoolbook_add128((data)[n - 1 - (i)], htbl[i + n], tmps + 4); \
	}

#define muladdlast_n2(Z, htbl, tmps, n)                       \
	{                                                         \
		schoolbook_add128((Z)[0], htbl[n - 1], tmps);         \
		schoolbook_add128((Z)[1], htbl[2 * n - 1], tmps + 4); \
	}

#define byterev(X) _mm_shuffle_epi8(X, _mm_setr_epi32(0x0c0d0e0f, 0x08090a0b, 0x04050607, 0x00010203))