#pragma once
#include "./common.h"
#include "./headers.h"

#define seq_graycode_x1(tmps, L, i)                                            \
  {                                                                            \
    (tmps)[0] = _mm_xor_si128((L)[_tzcnt_u64(i)], (tmps)[0]);                  \
  }

#define seq_graycode_x1_online(tmps, ctx, i)                                   \
  {                                                                            \
    (tmps)[0] = _mm_xor_si128(Lntz((ctx), (i) + 1), (tmps)[0]);                \
  }

#define seq_graycode_odd_x1(tmps, L, i)                                        \
  {                                                                            \
    (tmps)[0] = _mm_xor_si128((L)[0], (tmps)[0]);                              \
  }

#define seq_graycode_x4(tmps, L, L1, Li, i)                                    \
  {                                                                            \
    alignas(16) __m128i ___tmp3 = _mm_xor_si128(L1, Li);                       \
    Li = (L)[_tzcnt_u64(4 * i + 4)];                                           \
    alignas(16) __m128i ___tmp4 = _mm_xor_si128(L1, Li);                       \
    tmps[0] = _mm_xor_si128(tmps[0], ___tmp3);                                 \
    tmps[1] = _mm_xor_si128(tmps[1], ___tmp3);                                 \
    tmps[2] = _mm_xor_si128(tmps[2], ___tmp3);                                 \
    tmps[3] = _mm_xor_si128(tmps[3], ___tmp4);                                 \
  }

#define seq_graycode_x4_online(tmps, ctx, c, i)                                \
  {                                                                            \
    alignas(16) __m128i ___tmp0 = (ctx).L[0];                                  \
    alignas(16) __m128i ___tmp1 = (ctx).L[1];                                  \
    alignas(16) __m128i ___tmp2 = (ctx).L[_tzcnt_u64(4 * (i) + 4)];            \
    (tmps)[0] = _mm_xor_si128((c), ___tmp0);                                   \
    (tmps)[1] = _mm_xor_si128((tmps)[0], ___tmp1);                             \
    (tmps)[2] = _mm_xor_si128((c), ___tmp1);                                   \
    (tmps)[3] = _mm_xor_si128((tmps)[2], ___tmp2);                             \
    (c) = (tmps)[3];                                                           \
  }


#define seq_graycode_x8(tmps, L, L2, Li, i)                                    \
  {                                                                            \
    alignas(16) __m128i ___tmp7 = _mm_xor_si128((L2), (Li));                   \
    (Li) = (L)[3 + _tzcnt_u64((i) + 2)];                                       \
    alignas(16) __m128i ___tmp8 = _mm_xor_si128((L2), (Li));                   \
    tmps[0] = _mm_xor_si128(tmps[0], ___tmp7);                                 \
    tmps[1] = _mm_xor_si128(tmps[1], ___tmp7);                                 \
    tmps[2] = _mm_xor_si128(tmps[2], ___tmp7);                                 \
    tmps[3] = _mm_xor_si128(tmps[3], ___tmp7);                                 \
    tmps[4] = _mm_xor_si128(tmps[4], ___tmp7);                                 \
    tmps[5] = _mm_xor_si128(tmps[5], ___tmp7);                                 \
    tmps[6] = _mm_xor_si128(tmps[6], ___tmp7);                                 \
    tmps[7] = _mm_xor_si128(tmps[7], ___tmp8);                                 \
  }

#define seq_graycode_n2x1(tmps, L, i)                                          \
  {                                                                            \
    (tmps)[0] = _mm_xor_si128((L)[_tzcnt_u64(i) * 2], (tmps)[0]);              \
    (tmps)[1] = _mm_xor_si128((L)[_tzcnt_u64(i) * 2 + 1], (tmps)[1]);          \
  }

#define seq_graycode_n2x4(tmps, L, L1, Li, i)                                  \
  {                                                                            \
    alignas(16) __m128i ___tmp30 = _mm_xor_si128((L1)[0], (Li)[0]);            \
    alignas(16) __m128i ___tmp31 = _mm_xor_si128((L1)[1], (Li)[1]);            \
    (Li)[0] = (L)[_tzcnt_u64(4 * i + 4) * 2];                                  \
    (Li)[1] = (L)[_tzcnt_u64(4 * i + 4) * 2 + 1];                              \
    alignas(16) __m128i ___tmp40 = _mm_xor_si128((L1)[0], (Li)[0]);            \
    alignas(16) __m128i ___tmp41 = _mm_xor_si128((L1)[1], (Li)[1]);            \
    tmps[0 * 2 + 0] = _mm_xor_si128(tmps[0 * 2 + 0], ___tmp30);                \
    tmps[1 * 2 + 0] = _mm_xor_si128(tmps[1 * 2 + 0], ___tmp30);                \
    tmps[2 * 2 + 0] = _mm_xor_si128(tmps[2 * 2 + 0], ___tmp30);                \
    tmps[3 * 2 + 0] = _mm_xor_si128(tmps[3 * 2 + 0], ___tmp40);                \
    tmps[0 * 2 + 1] = _mm_xor_si128(tmps[0 * 2 + 1], ___tmp31);                \
    tmps[1 * 2 + 1] = _mm_xor_si128(tmps[1 * 2 + 1], ___tmp31);                \
    tmps[2 * 2 + 1] = _mm_xor_si128(tmps[2 * 2 + 1], ___tmp31);                \
    tmps[3 * 2 + 1] = _mm_xor_si128(tmps[3 * 2 + 1], ___tmp41);                \
  }

#define seq_graycode_n2x8(tmps, L, L2, Li, i)                                  \
  {                                                                            \
    alignas(16) __m128i ___tmp70 = _mm_xor_si128((L2)[0], (Li)[0]);            \
    alignas(16) __m128i ___tmp71 = _mm_xor_si128((L2)[1], (Li)[1]);            \
    (Li)[0] = (L)[(3 + _tzcnt_u64((i) + 2)) * 2];                              \
    (Li)[1] = (L)[(3 + _tzcnt_u64((i) + 2)) * 2 + 1];                          \
    alignas(16) __m128i ___tmp80 = _mm_xor_si128((L2)[0], (Li)[0]);            \
    alignas(16) __m128i ___tmp81 = _mm_xor_si128((L2)[1], (Li)[1]);            \
    tmps[0 * 2 + 0] = _mm_xor_si128(tmps[0 * 2 + 0], ___tmp70);                \
    tmps[1 * 2 + 0] = _mm_xor_si128(tmps[1 * 2 + 0], ___tmp70);                \
    tmps[2 * 2 + 0] = _mm_xor_si128(tmps[2 * 2 + 0], ___tmp70);                \
    tmps[3 * 2 + 0] = _mm_xor_si128(tmps[3 * 2 + 0], ___tmp70);                \
    tmps[4 * 2 + 0] = _mm_xor_si128(tmps[4 * 2 + 0], ___tmp70);                \
    tmps[5 * 2 + 0] = _mm_xor_si128(tmps[5 * 2 + 0], ___tmp70);                \
    tmps[6 * 2 + 0] = _mm_xor_si128(tmps[6 * 2 + 0], ___tmp70);                \
    tmps[7 * 2 + 0] = _mm_xor_si128(tmps[7 * 2 + 0], ___tmp80);                \
    tmps[0 * 2 + 1] = _mm_xor_si128(tmps[0 * 2 + 1], ___tmp71);                \
    tmps[1 * 2 + 1] = _mm_xor_si128(tmps[1 * 2 + 1], ___tmp71);                \
    tmps[2 * 2 + 1] = _mm_xor_si128(tmps[2 * 2 + 1], ___tmp71);                \
    tmps[3 * 2 + 1] = _mm_xor_si128(tmps[3 * 2 + 1], ___tmp71);                \
    tmps[4 * 2 + 1] = _mm_xor_si128(tmps[4 * 2 + 1], ___tmp71);                \
    tmps[5 * 2 + 1] = _mm_xor_si128(tmps[5 * 2 + 1], ___tmp71);                \
    tmps[6 * 2 + 1] = _mm_xor_si128(tmps[6 * 2 + 1], ___tmp71);                \
    tmps[7 * 2 + 1] = _mm_xor_si128(tmps[7 * 2 + 1], ___tmp81);                \
  }

static inline void setgraycode(__m128i *tmps, __m128i *L, size_t n) {
  tmps[0] = L[0];
  for (size_t i = 1; i < n; i++) {
    tmps[i] = _mm_xor_si128(L[_tzcnt_u64(i + 1)], tmps[i - 1]);
  }
}
