#include <immintrin.h>

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>

static inline __m128i loadu128(const uint8_t *p) {
    return _mm_loadu_si128(reinterpret_cast<const __m128i *>(p));
}

static inline void storeu128(uint8_t *p, __m128i x) {
    _mm_storeu_si128(reinterpret_cast<__m128i *>(p), x);
}

static inline __m128i byterev(__m128i x) {
    const __m128i idx = _mm_setr_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7,  6,  5,  4,  3,  2,  1, 0);
    return _mm_shuffle_epi8(x, idx);
}

static inline __m128i mul2_lane32(__m128i pp, __m128i x) {
    __m128i tmp1 = _mm_slli_epi32(x, 1);
    __m128i tmp2 = _mm_srli_epi32(x, 31);
    __m128i tmp3 = _mm_slli_si128(tmp2, 4);
    x = _mm_xor_si128(tmp1, tmp3);
    __m128i tmp4 = _mm_srli_si128(tmp2, 12);
    __m128i tmp5 = _mm_shuffle_epi8(pp, tmp4);
    return _mm_xor_si128(x, tmp5);
}

static inline __m128i mul2rev_lane32(__m128i pp, __m128i x) {
    return byterev(mul2_lane32(pp, byterev(x)));
}

static std::array<uint8_t, 16> scalar_mul2_byte_left(const std::array<uint8_t, 16> &in) {
    std::array<uint8_t, 16> out{};
    uint8_t carry = 0;
    for (int i = 15; i >= 0; --i) {
        uint8_t next = static_cast<uint8_t>(in[i] >> 7);
        out[i] = static_cast<uint8_t>((in[i] << 1) | carry);
        carry = next;
    }
    if (carry) {
        out[15] ^= 0x87;
    }
    return out;
}

static std::array<uint8_t, 16> scalar_mul2_byte_right(const std::array<uint8_t, 16> &in) {
    std::array<uint8_t, 16> out{};
    uint8_t carry = 0;
    for (int i = 0; i < 16; ++i) {
        uint8_t next = static_cast<uint8_t>((in[i] & 1u) << 7);
        out[i] = static_cast<uint8_t>((in[i] >> 1) | carry);
        carry = next;
    }
    if (carry) {
        out[0] ^= 0x87;
    }
    return out;
}

static void print_hex(const char *label, const uint8_t *buf, size_t len = 16) {
    std::printf("%-18s", label);
    for (size_t i = 0; i < len; ++i) {
        std::printf("%02x", buf[i]);
    }
    std::printf("\n");
}

static void print_hex_arr(const char *label, const std::array<uint8_t, 16> &buf) {
    print_hex(label, buf.data(), buf.size());
}

static void run_case(const char *name, const std::array<uint8_t, 16> &input) {
    alignas(16) uint8_t tmp[16];
    const __m128i x = loadu128(input.data());
    const __m128i pp = _mm_setr_epi32(0x87 << 8, 0, 0, 0);

    std::printf("\n== %s ==\n", name);
    print_hex_arr("input", input);

    storeu128(tmp, byterev(x));
    print_hex("byterev(input)", tmp);

    storeu128(tmp, mul2_lane32(pp, x));
    print_hex("mul2_lane32", tmp);

    storeu128(tmp, mul2rev_lane32(pp, x));
    print_hex("mul2rev_lane32", tmp);

    print_hex_arr("scalar_left", scalar_mul2_byte_left(input));
    print_hex_arr("scalar_right", scalar_mul2_byte_right(input));
}

int main() {
    run_case("single-bit byte0 lsb", {0x01});
    run_case("single-bit byte0 msb", {0x80});
    run_case("single-bit byte1 msb", {0x00, 0x80});
    run_case("ascending", {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                           0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff});
    return 0;
}
