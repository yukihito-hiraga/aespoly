#include "aesgcm.h"

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

struct testvector_t {
    std::string test_name;
    std::string scheme;
    params_t inputs;
    params_t expected;
};

class SchemeTest : public ::testing::TestWithParam<testvector_t> {};

struct case_data_t {
    bytes_t P;
    bytes_t K;
    bytes_t A;
    bytes_t IV;
    bytes_t E_C;
    bytes_t E_T;
};

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

inline case_data_t load_case(const testvector_t &tv) {
    return {
        tv.inputs.at("pt"),
        tv.inputs.at("key"),
        tv.inputs.at("ad"),
        tv.inputs.at("iv"),
        tv.expected.at("ct"),
        tv.expected.at("tag"),
    };
}

inline void expect_cipher_and_tag(const bytes_t &C, const bytes_t &T,
                                  const case_data_t &tc) {
    EXPECT_EQ(to_hex(C), to_hex(tc.E_C));
    EXPECT_EQ(to_hex(T), to_hex(tc.E_T));
}

TEST_P(SchemeTest, GCM_AES128_x4_PARALLEL) {
    auto tc = load_case(GetParam());
    bytes_t C(tc.P.size()), T(tc.K.size());
    aesgcm_context ctx;
    aesgcminit128(&ctx, tc.K.data());
    aesgcm128x4(ctx, tc.IV.data(), tc.IV.size(), tc.A.data(), tc.A.size(),
                tc.P.data(), tc.P.size(), C.data(), T.data(), T.size());
    expect_cipher_and_tag(C, T, tc);
}

TEST_P(SchemeTest, GCM_AES128_x8_PARALLEL) {
    auto tc = load_case(GetParam());
    bytes_t C(tc.P.size()), T(tc.K.size());
    aesgcm_context ctx;
    aesgcminit128(&ctx, tc.K.data());
    aesgcm128x8(ctx, tc.IV.data(), tc.IV.size(), tc.A.data(), tc.A.size(),
                tc.P.data(), tc.P.size(), C.data(), T.data(), T.size());
    expect_cipher_and_tag(C, T, tc);
}

TEST_P(SchemeTest, GCM_AES128_x4_SERIAL) {
    auto tc = load_case(GetParam());
    bytes_t C(tc.P.size()), T(tc.K.size());
    aesgcm_context ctx;
    aesgcminit128(&ctx, tc.K.data());
    aesgcm128_serialx4(ctx, tc.IV.data(), tc.IV.size(), tc.A.data(),
                       tc.A.size(), tc.P.data(), tc.P.size(), C.data(),
                       T.data(), T.size());
    expect_cipher_and_tag(C, T, tc);
}

TEST_P(SchemeTest, GCM_AES128_x8_SERIAL) {
    auto tc = load_case(GetParam());
    bytes_t C(tc.P.size()), T(tc.K.size());
    aesgcm_context ctx;
    aesgcminit128(&ctx, tc.K.data());
    aesgcm128_serialx8(ctx, tc.IV.data(), tc.IV.size(), tc.A.data(),
                       tc.A.size(), tc.P.data(), tc.P.size(), C.data(),
                       T.data(), T.size());
    expect_cipher_and_tag(C, T, tc);
}

INSTANTIATE_TEST_SUITE_P(
    Test, SchemeTest, ::testing::ValuesIn(load_jsonl("gcm_aes128.jsonl")),
    [](const ::testing::TestParamInfo<testvector_t> &info) {
        return info.param.scheme + info.param.test_name;
    });

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
