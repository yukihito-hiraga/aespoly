#include "hctr2.h"

hctr2_context ctx;
aes_context aesctx;

void wrapper_xctr(measurement_context measure_ctx)
{
	alignas(64) __m512i S;
	S = _mm512_loadu_si512((__m512i*)measure_ctx.ppt.head);
	xctrxoradd512(&aesctx, &ctx, S, measure_ctx.pt.size, measure_ctx.pt.head, measure_ctx.ct.head);
}

void wrapper_hash(measurement_context measure_ctx)
{
	alignas(64) __m512i hash;
	hash = hash512(&ctx, measure_ctx.pt.head, measure_ctx.pt.size, measure_ctx.tweak.head, measure_ctx.tweak.size);
	_mm512_storeu_si512(measure_ctx.ct.head, hash);
}

void wrapper_hashx4(measurement_context measure_ctx)
{
	alignas(64) __m512i hash;
	hash = hash512x4(&ctx, measure_ctx.pt.head, measure_ctx.pt.size, measure_ctx.tweak.head, measure_ctx.tweak.size);
	_mm512_storeu_si512(measure_ctx.ct.head, hash);
}

void wrapper_xctrhash(measurement_context measure_ctx)
{
	alignas(64) __m512i S, hash;
	hash = xctrxoradd_hash512(&aesctx, &ctx, S, measure_ctx.pt.size, measure_ctx.pt.head, measure_ctx.ct.head, measure_ctx.tweak.head, measure_ctx.tweak.size);
	_mm512_storeu_si512(measure_ctx.ct.head, hash);
}

void wrapper_xctrhashx4(measurement_context measure_ctx)
{
	alignas(64) __m512i S, hash;
	xctrxoradd_hash512x4(&aesctx, &ctx, S, measure_ctx.pt.size, measure_ctx.pt.head, measure_ctx.ct.head, measure_ctx.tweak.head, measure_ctx.tweak.size);
	_mm512_storeu_si512(measure_ctx.ct.head, hash);
}

void wrapper_hctr2(measurement_context measure_ctx)
{
	hctr2enc512(&aesctx, &ctx, measure_ctx.pt.head, measure_ctx.pt.size, measure_ctx.tweak.head, measure_ctx.tweak.size, measure_ctx.ct.head);
}

void wrapper_hctr2p(measurement_context measure_ctx)
{
	hctr2enc512p(&aesctx, &ctx, measure_ctx.pt.head, measure_ctx.pt.size, measure_ctx.tweak.head, measure_ctx.tweak.size, measure_ctx.ct.head);
}

int main(int argc, char **argv)
{
	ctx.poly = _mm512_broadcast_i64x2(_mm_setr_epi32(0x1, 0, 0, 0xc2000000));
	ctx.blocklength = 16;

	alignas(16) uint8_t key[16];
	str2hex128("9c8dc4bd7136dc827ca1caa3235adba4", key, 16);
	hctr2init512(&aesctx, &ctx, key);

	measurement_option option;
	option.clflush = false;
	option.core_used = 2;
	option.num_exp = 1000;
	option.num_warmup = 100;

	set_cpu_affinity(option.core_used);
	set_cpufreq_governor("performance");

	option.size_ad = 0;
	option.size_nonce = 0;
	option.size_pt = 160000;
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

		res = measure(option, wrapper_hctr2p);
		print_cpb("hctr2p", option, res);

		return 0;
	}

	size_t num_base, num_itr;
	num_base = get_num_base(argc, argv);
	num_itr = get_num_itr(argc, argv);

	GArray *targets;
	targets = g_array_new(false, true, sizeof(measurement_target));

	measurement_target target_polyvalx4 = make_target("polyvalx4", wrapper_hashx4);
	measurement_target target_xctr = make_target("xctr", wrapper_xctr);
	measurement_target target_xctr_polyvalx4 = make_target("xctr‖polyvalx4", wrapper_xctrhashx4);
	measurement_target target_hctr2 = make_target("hctr2", wrapper_hctr2p);

	g_array_append_val(targets, target_polyvalx4);
	g_array_append_val(targets, target_xctr);
	g_array_append_val(targets, target_xctr_polyvalx4);
	g_array_append_val(targets, target_hctr2);

	measure_save(targets, option, num_base, num_itr);

	return 0;
}