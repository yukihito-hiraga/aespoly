#include "aespolyM.h"

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

struct case_data_t {
    bytes_t K1;
    bytes_t K2;
    bytes_t H;
    bytes_t P;
    bytes_t E_T;
};

class SchemeTest : public ::testing::TestWithParam<testvector_t> {};

inline bytes_t from_hex(const std::string &hex) {
    bytes_t out;
    out.reserve(hex.size() / 2);
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        out.push_back(
            static_cast<uint8_t>(std::stoul(hex.substr(i, 2), nullptr, 16)));
    }
    return out;
}

inline std::string to_hex(const bytes_t &b) {
    std::string out;
    out.reserve(b.size() * 2);
    for (uint8_t byte : b) {
        out += std::format("{:02X}", byte);
    }
    return out;
}

inline std::vector<testvector_t> load_jsonl(const std::string &filename) {
    const char *env = std::getenv("TV_DIR");
    std::string tv_dir = env ? env : "testvectors";

    namespace fs = std::filesystem;
    fs::path p = fs::path(tv_dir) / filename;

    std::vector<testvector_t> result;
    if (!fs::exists(p)) {
        return result;
    }

    std::ifstream file(p);
    std::string line;
    int line_num = 0;

    while (std::getline(file, line)) {
        ++line_num;
        if (line.empty()) {
            continue;
        }

        auto j = json::parse(line, nullptr, false);
        if (j.is_discarded()) {
            continue;
        }

        std::string scheme = j.value("scheme", "");
        if (scheme.empty()) {
            continue;
        }

        params_t inputs, expected;
        for (auto &[k, v] : j["inputs"].items()) {
            inputs[k] = from_hex(v.get<std::string>());
        }
        for (auto &[k, v] : j["expected"].items()) {
            expected[k] = from_hex(v.get<std::string>());
        }

        result.push_back(
            {std::format("L{:04d}", line_num), scheme, inputs, expected});
    }

    return result;
}

inline case_data_t load_case(const testvector_t &tv) {
    return {
        tv.inputs.at("key1"),
        tv.inputs.at("key2"),
        tv.inputs.at("h"),
        tv.inputs.at("pt"),
        tv.expected.at("tag"),
    };
}

TEST_P(SchemeTest, AESPOLYM_AES128_X4) {
    auto tc = load_case(GetParam());

    aes_context aesctx1;
    aespolyM_context ctx;
    aesinit128(&aesctx1, tc.K1.data());
    aespolyMinit128_with_h(&ctx, tc.K2.data(), tc.H.data());

    __m128i tag =
        aespolyM128x4(aesctx1, ctx, nullptr, 0, tc.P.data(), tc.P.size());

    bytes_t out(16);
    _mm_storeu_si128(reinterpret_cast<__m128i *>(out.data()), tag);
    EXPECT_EQ(to_hex(out), to_hex(tc.E_T));
}

TEST_P(SchemeTest, AESPOLYM_AES128_X8) {
    auto tc = load_case(GetParam());

    aes_context aesctx1;
    aespolyM_context ctx;
    aesinit128(&aesctx1, tc.K1.data());
    aespolyMinit128_with_h(&ctx, tc.K2.data(), tc.H.data());

    __m128i tag =
        aespolyM128x8(aesctx1, ctx, nullptr, 0, tc.P.data(), tc.P.size());

    bytes_t out(16);
    _mm_storeu_si128(reinterpret_cast<__m128i *>(out.data()), tag);
    EXPECT_EQ(to_hex(out), to_hex(tc.E_T));
}

INSTANTIATE_TEST_SUITE_P(
    Test, SchemeTest, ::testing::ValuesIn(load_jsonl("aespolyM_aes128.jsonl")),
    [](const ::testing::TestParamInfo<testvector_t> &info) {
        return info.param.scheme + info.param.test_name;
    });

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
