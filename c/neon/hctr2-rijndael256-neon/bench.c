#include "hctr2.h"

hctr2_context ctx;

void wrapper_xctr(measurement_context measure_ctx)
{
    alignas(16) __m128i S[2];
    S[0] = _mm_loadu_si128((__m128i *)measure_ctx.ppt.head);
    S[1] = _mm_loadu_si128((__m128i *)measure_ctx.ppt.head + 1);
    xctrxoradd256(ctx, S, measure_ctx.pt.head, measure_ctx.pt.size, measure_ctx.ct.head);
}

void wrapper_hashx4(measurement_context measure_ctx)
{
    alignas(16) __m128i hash[2];
    alignas(16) __m128i state[2];
    tweakhash128n2x4(ctx, measure_ctx.pt.size % 16 > 0, measure_ctx.tweak.head, measure_ctx.tweak.size, state);
    hash128x4(ctx, state, measure_ctx.pt.head, measure_ctx.pt.size, hash);
    _mm_storeu_si128((__m128i *)measure_ctx.ct.head, hash[0]);
    _mm_storeu_si128((__m128i *)measure_ctx.ct.head + 1, hash[1]);
}

void wrapper_hashx8(measurement_context measure_ctx)
{
    alignas(16) __m128i hash[2];
    alignas(16) __m128i state[2];
    tweakhash128n2x8(ctx, measure_ctx.pt.size % 16 > 0, measure_ctx.tweak.head, measure_ctx.tweak.size, state);
    hash128x8(ctx, state, measure_ctx.pt.head, measure_ctx.pt.size, hash);
    _mm_storeu_si128((__m128i *)measure_ctx.ct.head, hash[0]);
    _mm_storeu_si128((__m128i *)measure_ctx.ct.head + 1, hash[1]);
}

void wrapper_xctrhashx4(measurement_context measure_ctx)
{
    alignas(16) __m128i S[2];
    S[0] = _mm_loadu_si128((__m128i *)measure_ctx.ppt.head);
    S[1] = _mm_loadu_si128((__m128i *)measure_ctx.ppt.head + 1);
    alignas(16) __m128i hash[2];
    alignas(16) __m128i state[2] = {_mm_setzero_si128(), _mm_setzero_si128()};
    xctrxoradd_hash128x4(ctx, state, S, measure_ctx.pt.head, measure_ctx.pt.size, measure_ctx.ct.head, hash);
    _mm_storeu_si128((__m128i *)measure_ctx.ct.head, hash[0]);
    _mm_storeu_si128((__m128i *)measure_ctx.ct.head + 1, hash[1]);
}

void wrapper_xctrhashx8(measurement_context measure_ctx)
{
    alignas(16) __m128i S[2];
    S[0] = _mm_loadu_si128((__m128i *)measure_ctx.ppt.head);
    S[1] = _mm_loadu_si128((__m128i *)measure_ctx.ppt.head + 1);
    alignas(16) __m128i hash[2];
    alignas(16) __m128i state[2] = {_mm_setzero_si128(), _mm_setzero_si128()};
    xctrxoradd_hash128x8(ctx, state, S, measure_ctx.pt.head, measure_ctx.pt.size, measure_ctx.ct.head, hash);
    _mm_storeu_si128((__m128i *)measure_ctx.ct.head, hash[0]);
    _mm_storeu_si128((__m128i *)measure_ctx.ct.head + 1, hash[1]);
}

void wrapper_hctr2x4(measurement_context measure_ctx)
{
    hctr2enc128x4(ctx, measure_ctx.tweak.head, measure_ctx.tweak.size, measure_ctx.pt.head, measure_ctx.pt.size, measure_ctx.ct.head);
}

void wrapper_hctr2x8(measurement_context measure_ctx)
{
    hctr2enc128x8(ctx, measure_ctx.tweak.head, measure_ctx.tweak.size, measure_ctx.pt.head, measure_ctx.pt.size, measure_ctx.ct.head);
}

int main(int argc, char **argv)
{

    alignas(16) uint8_t key[32];
    str2hex128("9c8dc4bd7136dc827ca1caa3235adba49c8dc4bd7136dc827ca1caa3235adba4", key, 32);
    hctr2init128(&ctx, key);

    measurement_option option;
    option.clflush = true;
    option.core_used = get_core_fromarg(argc, argv);
    option.num_exp = 200;
    option.num_warmup = 20;

    set_cpu_affinity(option.core_used);
    set_cpufreq_governor("performance");

    option.size_ad = 0;
    option.size_nonce = 0;
    option.size_pt = 32768;
    option.size_tweak = 64;

    measurement_result res;

    set_tscfreq(argc, argv);
    set_cpufreq(argc, argv);

    if (argc < 2)
    {
        res = measure(option, wrapper_xctr);
        print_cpb("xctr", option, res);

        res = measure(option, wrapper_hashx4);
        print_cpb("hashx4", option, res);

        res = measure(option, wrapper_xctrhashx4);
        print_cpb("xctrhashx4", option, res);

        res = measure(option, wrapper_hctr2x4);
        print_cpb("hctr2x4", option, res);
    }


    size_t num_base, num_itr;
    num_base = get_num_base(argc, argv);
    num_itr = get_num_itr(argc, argv);

    GArray *targets;
    targets = g_array_new(false, true, sizeof(measurement_target));

    measurement_target target_hashx4 = make_target("hashx4", wrapper_hashx4);
    measurement_target target_hashx8 = make_target("hashx8", wrapper_hashx8);
    measurement_target target_xctr = make_target("xctr", wrapper_xctr);
    measurement_target target_xctrhashx4 = make_target("xctr‖hashx4", wrapper_xctrhashx4);
    measurement_target target_xctrhashx8 = make_target("xctr‖hashx8", wrapper_xctrhashx8);
    measurement_target target_hctr2x4 = make_target("hctr2x4", wrapper_hctr2x4);
    measurement_target target_hctr2x8 = make_target("hctr2x8", wrapper_hctr2x8);

    g_array_append_val(targets, target_hashx4);
    g_array_append_val(targets, target_hashx8);
    g_array_append_val(targets, target_xctr);
    g_array_append_val(targets, target_xctrhashx4);
    g_array_append_val(targets, target_xctrhashx8);
    g_array_append_val(targets, target_hctr2x4);
    g_array_append_val(targets, target_hctr2x8);

    measure_save(targets, option, num_base, num_itr);

    return 0;
}