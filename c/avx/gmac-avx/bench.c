#include "gmac.h"

aes_context aesctx;
gmac_context ctx;

static inline void wrapper_gmacx4(measurement_context measure_ctx)
{
	alignas(16) __m128i tag = ghash128x4(ctx, measure_ctx.pt.head, measure_ctx.pt.size, measure_ctx.nonce.head, measure_ctx.nonce.size);
	_mm_storeu_si128((__m128i *)measure_ctx.ct.head, tag);
}

static inline void wrapper_gmacx8(measurement_context measure_ctx)
{
	alignas(16) __m128i tag = ghash128x8(ctx, measure_ctx.pt.head, measure_ctx.pt.size, measure_ctx.nonce.head, measure_ctx.nonce.size);
	_mm_storeu_si128((__m128i *)measure_ctx.ct.head, tag);
}

int main(int argc, char **argv)
{

	alignas(16) uint8_t key[16];
	str2hex128("9c8dc4bd7136dc827ca1caa3235adba4", key, 16);

	gmacinit128(&aesctx, &ctx, key);

	measurement_option option;
	option.clflush = true;
	option.core_used = get_core_fromarg(argc, argv);
	option.num_exp = 200;
	option.num_warmup = 20;

	option.size_ad = 0;
	option.size_nonce = 0;
	option.size_pt = 160000;
	option.size_tweak = 64;

	measurement_result res;

	set_tscfreq(argc, argv);
	set_cpufreq(argc, argv);

	size_t num_base, num_itr;
	num_base = get_num_base(argc, argv);
	num_itr = get_num_itr(argc, argv);

	GArray *targets;
	targets = g_array_new(false, true, sizeof(measurement_target));

	measurement_target target_gmacx4 = make_target("gmacx4", wrapper_gmacx4);
	measurement_target target_gmacx8 = make_target("gmacx8", wrapper_gmacx8);

	g_array_append_val(targets, target_gmacx4);
	g_array_append_val(targets, target_gmacx8);

	measure_save(targets, option, num_base, num_itr);

	return 0;
}