#include "aespoly.h"

aespoly_context ctx;

void wrapper_aespoly(measurement_context measure_ctx)
{
	enc(&ctx, measure_ctx.tweak.head, measure_ctx.tweak.size, measure_ctx.pt.head, measure_ctx.pt.size, measure_ctx.ct.head);
}

void wrapper_upper(measurement_context measure_ctx)
{
	alignas(16) __m128i tmps[4];
	alignas(16) __m128i X;
	X = _mm_loadu_si128((__m128i*)measure_ctx.ppt.head);
	upper(&ctx, X, measure_ctx.pt.head + 16, measure_ctx.pt.size - 16, measure_ctx.ct.head + 16, tmps, tmps + 1);
	_mm_storeu_si128((__m128i *)measure_ctx.ct.head, tmps[0]);
	_mm_storeu_si128((__m128i *)measure_ctx.ct.head + 1, tmps[1]);
}

void wrapper_middlelower(measurement_context measure_ctx)
{
	alignas(16) __m128i tmps[4];
	tmps[0] = _mm_loadu_si128((__m128i *)measure_ctx.ppt.head);
	tmps[1] = _mm_loadu_si128((__m128i *)measure_ctx.ppt.head + 1);
	tmps[2] = _mm_loadu_si128((__m128i *)measure_ctx.ppt.head + 2);
	middlelower(&ctx, tmps[0], tmps[1], tmps[2], measure_ctx.pt.head, measure_ctx.pt.size, measure_ctx.ct.head);
}

int main(int argc, char **argv)
{
	alignas(16) uint8_t key[16];
	str2hex128("9c8dc4bd7136dc827ca1caa3235adba4", key, 16);
	init(&ctx, key);

	measurement_option option;
	option.clflush = true;
	option.core_used = get_core_fromarg(argc, argv);
	option.num_exp = 200;
	option.num_warmup = 20;

	set_cpu_affinity(option.core_used);
	set_cpufreq_governor("performance");

	option.size_ad = 0;
	option.size_nonce = 0;
	option.size_pt = 131072 * 2;
	option.size_tweak = 64;

	measurement_result res;

	set_tscfreq(argc, argv);
	set_cpufreq(argc, argv);

	if (argc < 2)
	{
		res = measure(option, wrapper_aespoly);
		print_cpb("aespoly", option, res);
		return 0;
	}

	size_t num_base, num_itr;
	num_base = get_num_base(argc, argv);
	num_itr = get_num_itr(argc, argv);

	GArray *targets;
	targets = g_array_new(false, true, sizeof(measurement_target));

	measurement_target target_aespoly = make_target("aespoly", wrapper_aespoly);
	measurement_target target_upper = make_target("upper", wrapper_upper);
	measurement_target target_middlelower = make_target("middlelower", wrapper_middlelower);

	g_array_append_val(targets, target_upper);
	g_array_append_val(targets, target_middlelower);
	g_array_append_val(targets, target_aespoly);

	measure_save(targets, option, num_base, num_itr);

	return 0;
}