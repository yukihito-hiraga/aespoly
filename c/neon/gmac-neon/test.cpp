#include "gmac.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <gtest/gtest.h>
#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using json = nlohmann::json;

using bytes_t = std::vector<uint8_t>;
using params_t = std::map<std::string, bytes_t>;

struct testvector_t {
    std::string test_name;
    std::string scheme;
    params_t inputs;
    params_t expected;
};

class SchemeTest : public ::testing::TestWithParam<testvector_t> {};

inline bytes_t from_hex(const std::string &hex) {
    bytes_t out;
    out.reserve(hex.size() / 2);
    for (size_t i = 0; i + 1 < hex.size(); i += 2)
        out.push_back(
            static_cast<uint8_t>(std::stoul(hex.substr(i, 2), nullptr, 16)));
    return out;
}

inline std::string to_hex(const bytes_t &b) {
    std::string out;
    out.reserve(b.size() * 2);
    for (uint8_t byte : b)
        out += std::format("{:02X}", byte);
    return out;
}

inline std::vector<testvector_t> load_jsonl(const std::string &filename) {
    const char *env = std::getenv("TV_DIR");
    std::string tv_dir = env ? env : "testvectors";

    namespace fs = std::filesystem;
    fs::path p = fs::path(tv_dir) / filename;

    std::vector<testvector_t> result;
    if (!fs::exists(p))
        return result;

    std::ifstream file(p);
    std::string line;
    int line_num = 0;

    while (std::getline(file, line)) {
        ++line_num;
        if (line.empty())
            continue;

        auto j = json::parse(line, nullptr, /*exceptions=*/false);
        if (j.is_discarded())
            continue;

        std::string scheme = j.value("scheme", "");
        if (scheme.empty())
            continue;

        params_t inputs, expected;
        for (auto &[k, v] : j["inputs"].items())
            inputs[k] = from_hex(v.get<std::string>());
        for (auto &[k, v] : j["expected"].items())
            expected[k] = from_hex(v.get<std::string>());

        result.push_back(
            {std::format("L{:04d}", line_num), scheme, inputs, expected});
    }
    return result;
}

inline __m128i gmac128x4(aes_context aesctx, gmac_context ctx, uint8_t *IV,
                         size_t IVlen, uint8_t *A, size_t Alen) {
    __m128i S, T, tmp;

    if (IVlen == 12) {
        uint8_t s[16];
        memset(s, 0, 16);
        memcpy(s, IV, 12);
        s[15] = 1;
        S = _mm_loadu_si128((__m128i *)s);
    } else {
        uint8_t chunk[16];
        memset(chunk, 0, 16);
        ((uint64_t *)chunk)[0] = IVlen * 8;
        __m128i X = polyval128x4(ctx, _mm_setzero_si128(), (__m128i *)IV,
                                 IVlen - (IVlen % 16));
        if (IVlen % 16) {
            uint8_t padded[16];
            memset(padded, 0, 16);
            memcpy(padded, IV + IVlen - (IVlen % 16), IVlen % 16);
            X = _mm_xor_si128(X, byterev(_mm_loadu_si128((__m128i *)padded)));
            X = polydot128(ctx.poly, X, ctx.htbl[0]);
        }
        X = _mm_xor_si128(X, _mm_loadu_si128((__m128i *)chunk));
        X = polydot128(ctx.poly, X, ctx.htbl[0]);
        S = byterev(X);
    }

    T = ghash128x4(ctx, A, Alen, nullptr, 0);
    tmp = aesenc128(S, aesctx.keys128);
    return _mm_xor_si128(tmp, T);
}

inline __m128i gmac128x8(aes_context aesctx, gmac_context ctx, uint8_t *IV,
                         size_t IVlen, uint8_t *A, size_t Alen) {
    __m128i S, T, tmp;

    if (IVlen == 12) {
        uint8_t s[16];
        memset(s, 0, 16);
        memcpy(s, IV, 12);
        s[15] = 1;
        S = _mm_loadu_si128((__m128i *)s);
    } else {
        uint8_t chunk[16];
        memset(chunk, 0, 16);
        ((uint64_t *)chunk)[0] = IVlen * 8;
        __m128i X = polyval128x8(ctx, _mm_setzero_si128(), (__m128i *)IV,
                                 IVlen - (IVlen % 16));
        if (IVlen % 16) {
            uint8_t padded[16];
            memset(padded, 0, 16);
            memcpy(padded, IV + IVlen - (IVlen % 16), IVlen % 16);
            X = _mm_xor_si128(X, byterev(_mm_loadu_si128((__m128i *)padded)));
            X = polydot128(ctx.poly, X, ctx.htbl[0]);
        }
        X = _mm_xor_si128(X, _mm_loadu_si128((__m128i *)chunk));
        X = polydot128(ctx.poly, X, ctx.htbl[0]);
        S = byterev(X);
    }

    T = ghash128x8(ctx, A, Alen, nullptr, 0);
    tmp = aesenc128(S, aesctx.keys128);
    return _mm_xor_si128(tmp, T);
}

TEST_P(SchemeTest, GMAC_AES128_x4) {
    auto tv = GetParam();
    auto K = tv.inputs.at("key");
    auto IV = tv.inputs.at("iv");
    auto A = tv.inputs.at("ad");
    auto E_T = tv.expected.at("tag");

    aes_context aesctx;
    gmac_context ctx;
    gmacinit128(&aesctx, &ctx, K.data());

    bytes_t T(16);
    auto tag = gmac128x4(aesctx, ctx, IV.data(), IV.size(), A.data(), A.size());
    _mm_storeu_si128((__m128i *)T.data(), tag);

    EXPECT_EQ(to_hex(T), to_hex(E_T));
}

TEST_P(SchemeTest, GMAC_AES128_x8) {
    auto tv = GetParam();
    auto K = tv.inputs.at("key");
    auto IV = tv.inputs.at("iv");
    auto A = tv.inputs.at("ad");
    auto E_T = tv.expected.at("tag");

    aes_context aesctx;
    gmac_context ctx;
    gmacinit128(&aesctx, &ctx, K.data());

    bytes_t T(16);
    auto tag = gmac128x8(aesctx, ctx, IV.data(), IV.size(), A.data(), A.size());
    _mm_storeu_si128((__m128i *)T.data(), tag);

    EXPECT_EQ(to_hex(T), to_hex(E_T));
}

INSTANTIATE_TEST_SUITE_P(
    Test, SchemeTest, ::testing::ValuesIn(load_jsonl("gmac_aes128.jsonl")),
    [](const ::testing::TestParamInfo<testvector_t> &info) {
        return info.param.scheme + info.param.test_name;
    });

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
