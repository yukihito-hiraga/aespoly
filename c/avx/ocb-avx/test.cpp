#include "ocb.h"

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

struct case_data_t {
    bytes_t P;
    bytes_t K;
    bytes_t A;
    bytes_t N;
    bytes_t E_CT;
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
    const auto nonce_it = tv.inputs.find("nonce");
    const auto iv_it = tv.inputs.find("iv");
    return {
        tv.inputs.at("pt"),
        tv.inputs.at("key"),
        tv.inputs.at("ad"),
        nonce_it != tv.inputs.end() ? nonce_it->second : iv_it->second,
        tv.expected.at("ct"),
    };
}

inline size_t tag_len(const case_data_t &tc) {
    return tc.E_CT.size() - tc.P.size();
}

inline void expect_ct_with_tag(const bytes_t &out, const case_data_t &tc) {
    EXPECT_EQ(to_hex(out), to_hex(tc.E_CT));
}

inline void fill_pattern(bytes_t &out, uint8_t seed) {
    for (size_t i = 0; i < out.size(); i++)
        out[i] = static_cast<uint8_t>(seed + 17 * i);
}

TEST_P(SchemeTest, OCB_AES128) {
    auto tc = load_case(GetParam());
    bytes_t C(tc.E_CT.size());
    ocb_context ctx;
    ocbinit128(&ctx, tc.K.data(), false);
    ocbenc128(ctx, tag_len(tc), tc.N.data(), tc.N.size(), tc.A.data(),
              tc.A.size(), tc.P.data(), tc.P.size(), C.data());
    expect_ct_with_tag(C, tc);
}

TEST_P(SchemeTest, OCB_AES128_OFFLINE) {
    auto tc = load_case(GetParam());
    bytes_t C(tc.E_CT.size());
    ocb_context ctx;
    ocbinit128(&ctx, tc.K.data(), false);
    ocbenc128_offline(ctx, tag_len(tc), tc.N.data(), tc.N.size(),
                      tc.A.data(), tc.A.size(), tc.P.data(), tc.P.size(),
                      C.data());
    expect_ct_with_tag(C, tc);
}

TEST_P(SchemeTest, OCB_AES128_x4) {
    auto tc = load_case(GetParam());
    bytes_t C(tc.E_CT.size());
    ocb_context ctx;
    ocbinit128(&ctx, tc.K.data(), false);
    ocbenc128x4(ctx, tag_len(tc), tc.N.data(), tc.N.size(), tc.A.data(),
                tc.A.size(), tc.P.data(), tc.P.size(), C.data());
    expect_ct_with_tag(C, tc);
}

TEST_P(SchemeTest, OCB_AES128_x4_OFFLINE) {
    auto tc = load_case(GetParam());
    bytes_t C(tc.E_CT.size());
    ocb_context ctx;
    ocbinit128(&ctx, tc.K.data(), false);
    ocbenc128x4_offline(ctx, tag_len(tc), tc.N.data(), tc.N.size(),
                        tc.A.data(), tc.A.size(), tc.P.data(), tc.P.size(),
                        C.data());
    expect_ct_with_tag(C, tc);
}

TEST_P(SchemeTest, OCB_AES128_x8_OFFLINE) {
    auto tc = load_case(GetParam());
    bytes_t C(tc.E_CT.size());
    ocb_context ctx;
    ocbinit128(&ctx, tc.K.data(), false);
    ocbenc128x8_offline(ctx, tag_len(tc), tc.N.data(), tc.N.size(),
                        tc.A.data(), tc.A.size(), tc.P.data(), tc.P.size(),
                        C.data());
    expect_ct_with_tag(C, tc);
}

TEST(OCBSelfCheck, X4MatchesScalar) {
    const std::vector<size_t> ad_lens = {0, 1, 7, 8, 15, 16, 17, 31, 32};
    const std::vector<size_t> pt_lens = {0, 1, 7, 8, 15, 16, 17, 31, 32, 47,
                                         48, 63, 64, 65, 79, 80, 95, 96, 127};

    bytes_t key(16), nonce(12);
    fill_pattern(key, 0x10);
    fill_pattern(nonce, 0x20);

    for (size_t alen : ad_lens) {
        for (size_t plen : pt_lens) {
            bytes_t ad(alen), pt(plen);
            fill_pattern(ad, 0x30);
            fill_pattern(pt, 0x40);

            bytes_t scalar_out(plen + 16), x4_out(plen + 16);
            ocb_context ctx;
            ocbinit128(&ctx, key.data(), false);
            ocbenc128(ctx, 16, nonce.data(), nonce.size(), ad.data(), ad.size(),
                      pt.data(), pt.size(), scalar_out.data());
            ocbenc128x4(ctx, 16, nonce.data(), nonce.size(), ad.data(),
                        ad.size(), pt.data(), pt.size(), x4_out.data());

            EXPECT_EQ(to_hex(x4_out), to_hex(scalar_out))
                << "alen=" << alen << " plen=" << plen;
        }
    }
}



INSTANTIATE_TEST_SUITE_P(
    Test, SchemeTest, ::testing::ValuesIn(load_jsonl("ocb_aes128.jsonl")),
    [](const ::testing::TestParamInfo<testvector_t> &info) {
        return info.param.scheme + info.param.test_name;
    });

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
