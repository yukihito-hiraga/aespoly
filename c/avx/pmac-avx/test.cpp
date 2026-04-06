#include "pmac.h"

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
        if (scheme.empty() || scheme != "pmac_aes128") {
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

TEST_P(SchemeTest, PMAC_AES128_x4) {
    auto tv = GetParam();
    auto K = tv.inputs.at("key");
    auto P = tv.inputs.at("pt");
    auto E_T = tv.expected.at("tag");

    aes_context aesctx;
    pmac_context ctx;
    pmacinit128(&aesctx, &ctx, K.data());
    bytes_t T(16);
    auto tag = pmac128x4(&aesctx, &ctx, P.data(), P.size());
    _mm_storeu_si128((__m128i *)T.data(), tag);

    EXPECT_EQ(to_hex(T), to_hex(E_T));
}

TEST_P(SchemeTest, PMAC_AES128_x8) {
    auto tv = GetParam();
    auto K = tv.inputs.at("key");
    auto P = tv.inputs.at("pt");
    auto E_T = tv.expected.at("tag");

    aes_context aesctx;
    pmac_context ctx;
    pmacinit128(&aesctx, &ctx, K.data());
    bytes_t T(16);
    auto tag = pmac128x8(&aesctx, &ctx, P.data(), P.size());
    _mm_storeu_si128((__m128i *)T.data(), tag);

    EXPECT_EQ(to_hex(T), to_hex(E_T));
}

INSTANTIATE_TEST_SUITE_P(
    Test, SchemeTest, ::testing::ValuesIn(load_jsonl("pmac_aes128.jsonl")),
    [](const ::testing::TestParamInfo<testvector_t> &info) {
        return info.param.scheme + info.param.test_name;
    });

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
