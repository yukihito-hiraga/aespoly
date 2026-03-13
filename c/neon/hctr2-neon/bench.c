#include "hctr2.h"

hctr2_context ctx;
aes_context aesctx;

void wrapper_xctr(measurement_context measure_ctx)
{
    alignas(16) __m128i S;
    xctrxoradd128(aesctx, ctx, S, measure_ctx.pt.size, measure_ctx.pt.head, measure_ctx.ct.head);
}

void wrapper_hash(measurement_context measure_ctx)
{
    alignas(16) __m128i hash;
    hash = hash128(ctx, measure_ctx.pt.head, measure_ctx.pt.size, measure_ctx.tweak.head, measure_ctx.tweak.size);
    _mm_storeu_si128((__m128i *)measure_ctx.ct.head, hash);
}

void wrapper_hashx4(measurement_context measure_ctx)
{
    alignas(16) __m128i hash;
    hash = hash128x4(ctx, measure_ctx.pt.head, measure_ctx.pt.size, measure_ctx.tweak.head, measure_ctx.tweak.size);
    _mm_storeu_si128((__m128i *)measure_ctx.ct.head, hash);
}

void wrapper_hashx8(measurement_context measure_ctx)
{
    alignas(16) __m128i hash;
    hash = hash128x8(ctx, measure_ctx.pt.head, measure_ctx.pt.size, measure_ctx.tweak.head, measure_ctx.tweak.size);
    _mm_storeu_si128((__m128i *)measure_ctx.ct.head, hash);
}

void wrapper_xctrhash(measurement_context measure_ctx)
{
    alignas(16) __m128i S, hash;
    hash = xctrxoradd_hash128(aesctx, ctx, S, measure_ctx.pt.size, measure_ctx.pt.head, measure_ctx.ct.head,
                              measure_ctx.tweak.head, measure_ctx.tweak.size);
    _mm_storeu_si128((__m128i *)measure_ctx.ct.head, hash);
}

void wrapper_xctrhashx4(measurement_context measure_ctx)
{
    alignas(16) __m128i S;
    xctrxoradd_hash128x4(aesctx, ctx, S, measure_ctx.pt.size, measure_ctx.pt.head, measure_ctx.ct.head,
                         measure_ctx.tweak.head, measure_ctx.tweak.size);
}

void wrapper_xctrhashx8(measurement_context measure_ctx)
{
    alignas(16) __m128i S;
    xctrxoradd_hash128x8(aesctx, ctx, S, measure_ctx.pt.size, measure_ctx.pt.head, measure_ctx.ct.head,
                         measure_ctx.tweak.head, measure_ctx.tweak.size);
}

void wrapper_hctr2(measurement_context measure_ctx)
{
    hctr2enc128(aesctx, ctx, measure_ctx.pt.head, measure_ctx.pt.size, measure_ctx.tweak.head, measure_ctx.tweak.size,
                measure_ctx.ct.head);
}

void wrapper_hctr2p_x4(measurement_context measure_ctx)
{
    hctr2enc128p_x4(aesctx, ctx, measure_ctx.pt.head, measure_ctx.pt.size, measure_ctx.tweak.head, measure_ctx.tweak.size,
                 measure_ctx.ct.head);
}

void wrapper_hctr2p_x8(measurement_context measure_ctx)
{
    hctr2enc128p_x8(aesctx, ctx, measure_ctx.pt.head, measure_ctx.pt.size, measure_ctx.tweak.head, measure_ctx.tweak.size,
                 measure_ctx.ct.head);
}

int main(int argc, char **argv)
{
    ctx.poly = _mm_setr_epi32(0x1, 0, 0, 0xc2000000);
    ctx.blocklength = 16;

    alignas(16) uint8_t key[16];
    str2hex128("9c8dc4bd7136dc827ca1caa3235adba4", key, 16);
    hctr2init128(&aesctx, &ctx, key);

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

        res = measure(option, wrapper_hctr2p_x4);
        print_cpb("hctr2px4", option, res);
    }

    size_t num_base, num_itr;
    num_base = get_num_base(argc, argv);
    num_itr = get_num_itr(argc, argv);

    GArray *targets;
    targets = g_array_new(false, true, sizeof(measurement_target));

    measurement_target target_polyvalx4 = make_target("polyvalx4", wrapper_hashx4);
    measurement_target target_polyvalx8 = make_target("polyvalx8", wrapper_hashx8);
    // measurement_target target_xctr = make_target("xctr", wrapper_xctr);
    measurement_target target_xctr_polyvalx4 = make_target("xctr‖polyvalx4", wrapper_xctrhashx4);
    measurement_target target_xctr_polyvalx8 = make_target("xctr‖polyvalx8", wrapper_xctrhashx8);
    measurement_target target_hctr2x4 = make_target("hctr2x4", wrapper_hctr2p_x4);
    measurement_target target_hctr2x8 = make_target("hctr2x8", wrapper_hctr2p_x8);

    g_array_append_val(targets, target_polyvalx4);
    g_array_append_val(targets, target_polyvalx8);
    g_array_append_val(targets, target_xctr_polyvalx4);
    g_array_append_val(targets, target_xctr_polyvalx8);
    g_array_append_val(targets, target_hctr2x4);
    g_array_append_val(targets, target_hctr2x8);

    measure_save(targets, option, num_base, num_itr);

    return 0;
}