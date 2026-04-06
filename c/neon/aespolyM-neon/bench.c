#include "aespolyM.h"

aes_context aesctx;
aespolyM_context ctx;

static inline void wrapper_aespolyM(measurement_context measure_ctx)
{
	alignas(16) __m128i tag = aespolyM128x4(aesctx, ctx, measure_ctx.ad.head, measure_ctx.ad.size, measure_ctx.pt.head, measure_ctx.pt.size);
	_mm_storeu_si128((__m128i *)measure_ctx.ct.head, tag);
}

static inline void wrapper_aespolyMx(measurement_context measure_ctx)
{
	alignas(16) __m128i tag = aespolyM128x8(aesctx, ctx, measure_ctx.ad.head, measure_ctx.ad.size, measure_ctx.pt.head, measure_ctx.pt.size);
	_mm_storeu_si128((__m128i *)measure_ctx.ct.head, tag);
}

int main(int argc, char **argv)
{

	alignas(16) uint8_t key[16];
	str2hex128("9c8dc4bd7136dc827ca1caa3235adba4", key, 16);

	aespolyMinit128(&aesctx, &ctx, key);

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

	measurement_target target_aespolyMx4 = make_target("aespolyMx4", wrapper_aespolyM);
	measurement_target target_aespolyMx8 = make_target("aespolyMx8", wrapper_aespolyMx);

	g_array_append_val(targets, target_aespolyMx4);
	g_array_append_val(targets, target_aespolyMx8);

	measure_save(targets, option, num_base, num_itr);

	return 0;
}