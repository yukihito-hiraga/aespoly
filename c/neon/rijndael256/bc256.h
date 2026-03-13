#include "../common/common.h"

static inline __m256i rijndael256_fix_256(__m256i pt)
{
    alignas(32) __m256i tmp1, tmp2, tmp;
    alignas(32) __m256i shuffle, mask;
    tmp = _mm256_shuffle_epi8(pt, shuffle);
    tmp1 = _mm256_and_si256(tmp, mask);
    tmp1 = _mm256_broadcast_i64x2(_mm_xor_si128(((__m128i *)&tmp1)[0], ((__m128i *)&tmp1)[1]));
    tmp = _mm256_xor_si256(tmp1, tmp);
}

static inline __m256i rijndael256_256(__m256i pt, __m256i *keys)
{
    alignas(32) __m256i tmp;
    tmp = _mm256_xor_si256(pt, keys[0]);
    tmp = rijndael256_fix_256(tmp);
    tmp = _mm256_aesenc_epi128(tmp, keys[1]); //->serial
    tmp = rijndael256_fix_256(tmp);
    tmp = _mm256_aesenc_epi128(tmp, keys[2]);
    tmp = rijndael256_fix_256(tmp);
    tmp = _mm256_aesenc_epi128(tmp, keys[3]);
    tmp = rijndael256_fix_256(tmp);
    tmp = _mm256_aesenc_epi128(tmp, keys[4]);
    tmp = rijndael256_fix_256(tmp);
    tmp = _mm256_aesenc_epi128(tmp, keys[5]);
    tmp = rijndael256_fix_256(tmp);
    tmp = _mm256_aesenc_epi128(tmp, keys[6]);
    tmp = rijndael256_fix_256(tmp);
    tmp = _mm256_aesenc_epi128(tmp, keys[7]);
    tmp = rijndael256_fix_256(tmp);
    tmp = _mm256_aesenc_epi128(tmp, keys[8]);
    tmp = rijndael256_fix_256(tmp);
    tmp = _mm256_aesenc_epi128(tmp, keys[9]);
    tmp = rijndael256_fix_256(tmp);
    tmp = _mm256_aesenclast_epi128(tmp, keys[10]);
    return tmp;
}
