#include "aesgcm.h"

aesgcm_context ctx;

void wrapper_gctrx4(measurement_context measure_ctx)
{
    alignas(16) __m128i S = _mm_loadu_si128((__m128i *)measure_ctx.ppt.head);
    gctr128x4(ctx, S, measure_ctx.pt.head, measure_ctx.pt.size, measure_ctx.ct.head);
}

void wrapper_gctrx8(measurement_context measure_ctx)
{
    alignas(16) __m128i S = _mm_loadu_si128((__m128i *)measure_ctx.ppt.head);
    gctr128x8(ctx, S, measure_ctx.pt.head, measure_ctx.pt.size, measure_ctx.ct.head);
}

void wrapper_ghashx4(measurement_context measure_ctx)
{
    alignas(16) __m128i T = ghash128x4(ctx, measure_ctx.ad.head, measure_ctx.ad.size, measure_ctx.pt.head, measure_ctx.pt.size);
    _mm_storeu_si128((__m128i *)measure_ctx.ct.head, T);
}

void wrapper_ghashx8(measurement_context measure_ctx)
{
    alignas(16) __m128i T = ghash128x8(ctx, measure_ctx.ad.head, measure_ctx.ad.size, measure_ctx.pt.head, measure_ctx.pt.size);
    _mm_storeu_si128((__m128i *)measure_ctx.ct.head, T);
}

void wrapper_ctrhashx4(measurement_context measure_ctx)
{
    alignas(16) __m128i S = _mm_loadu_si128((__m128i *)measure_ctx.ad.head);
    alignas(16) __m128i T = ctrhash128x4(ctx, measure_ctx.ad.head, measure_ctx.ad.size, S, measure_ctx.pt.head, measure_ctx.pt.size, measure_ctx.ct.head);
    _mm_storeu_si128((__m128i *)measure_ctx.ct.head, T);
}

void wrapper_ctrhashx8(measurement_context measure_ctx)
{
    alignas(16) __m128i S = _mm_loadu_si128((__m128i *)measure_ctx.ad.head);
    alignas(16) __m128i T = ctrhash128x8(ctx, measure_ctx.ad.head, measure_ctx.ad.size, S, measure_ctx.pt.head, measure_ctx.pt.size, measure_ctx.ct.head);
    _mm_storeu_si128((__m128i *)measure_ctx.ct.head, T);
}

void wrapper_aesgcmx4(measurement_context measure_ctx)
{
    uint8_t tag[16];
    aesgcm128x4(ctx, measure_ctx.nonce.head, measure_ctx.nonce.size, measure_ctx.ad.head, measure_ctx.ad.size, measure_ctx.pt.head, measure_ctx.pt.size, measure_ctx.ct.head, tag, 16);
    memcpy(measure_ctx.ct.head, tag, 16);
}

void wrapper_aesgcmx8(measurement_context measure_ctx)
{
    uint8_t tag[16];
    aesgcm128x8(ctx, measure_ctx.nonce.head, measure_ctx.nonce.size, measure_ctx.ad.head, measure_ctx.ad.size, measure_ctx.pt.head, measure_ctx.pt.size, measure_ctx.ct.head, tag, 16);
    memcpy(measure_ctx.ct.head, tag, 16);
}

void wrapper_aesgcm_serialx4(measurement_context measure_ctx)
{
    uint8_t tag[16];
    aesgcm128_serialx4(ctx, measure_ctx.nonce.head, measure_ctx.nonce.size, measure_ctx.ad.head, measure_ctx.ad.size, measure_ctx.pt.head, measure_ctx.pt.size, measure_ctx.ct.head, tag, 16);
    memcpy(measure_ctx.ct.head, tag, 16);
}

void wrapper_aesgcm_serialx8(measurement_context measure_ctx)
{
    uint8_t tag[16];
    aesgcm128_serialx8(ctx, measure_ctx.nonce.head, measure_ctx.nonce.size, measure_ctx.ad.head, measure_ctx.ad.size, measure_ctx.pt.head, measure_ctx.pt.size, measure_ctx.ct.head, tag, 16);
    memcpy(measure_ctx.ct.head, tag, 16);
}

int main(int argc, char **argv)
{
    alignas(16) uint8_t key[16];
    str2hex128("9c8dc4bd7136dc827ca1caa3235adba4", key, 16);
    aesgcminit128(&ctx, key);

    measurement_option option;
    option.clflush = true;
    option.core_used = get_core_fromarg(argc, argv);
    option.num_exp = 200;
    option.num_warmup = 20;

    set_cpu_affinity(option.core_used);
    set_cpufreq_governor("performance");

    option.size_ad = 0;
    option.size_nonce = 0;
    option.size_pt = 65536 * 1024;
    option.size_tweak = 64;

    set_tscfreq(argc, argv);
    set_cpufreq(argc, argv);

    measurement_result res;

    if (argc < 2)
    {
        res = measure(option, wrapper_ctrhashx4);
        print_cpb("aesctr ‖ hash", option, res);
    }

    size_t num_base, num_itr;
    num_base = get_num_base(argc, argv);
    num_itr = get_num_itr(argc, argv);

    GArray *targets;
    targets = g_array_new(false, true, sizeof(measurement_target));

    measurement_target target_ghashx4 = make_target("ghashx4", wrapper_ghashx4);
    measurement_target target_ghashx8 = make_target("ghashx8", wrapper_ghashx8);
    measurement_target target_aesctrx4 = make_target("aesctrx4", wrapper_gctrx4);
    measurement_target target_aesctrx8 = make_target("aesctrx8", wrapper_gctrx8);
    measurement_target target_ctrhashx4 = make_target("aesctrx4‖ghashx4", wrapper_ctrhashx4);
    measurement_target target_ctrhashx8 = make_target("aesctrx8‖ghashx8", wrapper_ctrhashx8);
    measurement_target target_aesgcmx4 = make_target("aesgcmx4", wrapper_aesgcmx4);
    measurement_target target_aesgcmx8 = make_target("aesgcmx8", wrapper_aesgcmx8);
    measurement_target target_aesgcm_serialx4 = make_target("aesgcm_serialx4", wrapper_aesgcm_serialx4);
    measurement_target target_aesgcm_serialx8 = make_target("aesgcm_serialx8", wrapper_aesgcm_serialx8);

    g_array_append_val(targets, target_ghashx4);
    g_array_append_val(targets, target_ghashx8);
    g_array_append_val(targets, target_aesctrx4);
    g_array_append_val(targets, target_aesctrx8);
    g_array_append_val(targets, target_ctrhashx4);
    g_array_append_val(targets, target_ctrhashx8);
    g_array_append_val(targets, target_aesgcmx4);
    g_array_append_val(targets, target_aesgcmx8);
    g_array_append_val(targets, target_aesgcm_serialx4);
    g_array_append_val(targets, target_aesgcm_serialx8);

    measure_save(targets, option, num_base, num_itr);

    return 0;
}