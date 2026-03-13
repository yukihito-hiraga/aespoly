#include "pmac.h"

aes_context aesctx;
pmac_context ctx;

static inline void wrapper_pmacx4(measurement_context measure_ctx)
{
	alignas(16) __m128i tag = pmac128x4(aesctx, ctx, measure_ctx.pt.head, measure_ctx.pt.size);
	_mm_storeu_si128((__m128i *)measure_ctx.ct.head, tag);
}

static inline void wrapper_pmacx8(measurement_context measure_ctx)
{
	alignas(16) __m128i tag = pmac128x8(aesctx, ctx, measure_ctx.pt.head, measure_ctx.pt.size);
	_mm_storeu_si128((__m128i *)measure_ctx.ct.head, tag);
}

int main(int argc, char **argv)
{

	alignas(16) uint8_t key[16];
	str2hex128("9c8dc4bd7136dc827ca1caa3235adba4", key, 16);

	pmacinit128(&aesctx, &ctx, key);

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

	measurement_target target_pmacx4 = make_target("pmacx4", wrapper_pmacx4);
	measurement_target target_pmacx8 = make_target("pmacx8", wrapper_pmacx8);

	g_array_append_val(targets, target_pmacx4);
	g_array_append_val(targets, target_pmacx8);

	measure_save(targets, option, num_base, num_itr);

	return 0;
}