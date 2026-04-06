#include "aespolyW.h"

aespolyW_context ctx;

void wrapper_aespolyWx4(measurement_context measure_ctx)
{
	encx4(ctx, measure_ctx.tweak.head, measure_ctx.tweak.size, measure_ctx.pt.head, measure_ctx.pt.size, measure_ctx.ct.head);
}

void wrapper_aespolyWx8(measurement_context measure_ctx)
{
	encx8(ctx, measure_ctx.tweak.head, measure_ctx.tweak.size, measure_ctx.pt.head, measure_ctx.pt.size, measure_ctx.ct.head);
}

void wrapper_upper(measurement_context measure_ctx)
{
	alignas(16) __m128i tmps[10];
	tmps[4] = _mm_loadu_si128((__m128i *)measure_ctx.ppt.head);
	tmps[5] = _mm_loadu_si128((__m128i *)measure_ctx.ppt.head + 1);
	upper(ctx, tmps + 4, measure_ctx.pt.head, measure_ctx.pt.size, measure_ctx.ct.head, tmps, tmps + 2);
	storex4((__m128i *)measure_ctx.ct.head, tmps);
}

void wrapper_middlelower(measurement_context measure_ctx)
{
	alignas(16) __m128i tmps[10];
	loadx4((__m128i *)measure_ctx.ppt.head, tmps);
	loadx2((__m128i *)measure_ctx.ppt.head + 4, tmps + 4);
	middlelower(ctx, tmps, tmps + 2, tmps + 4, measure_ctx.pt.head, measure_ctx.pt.size, measure_ctx.ct.head, tmps + 6);
	storex2((__m128i *)measure_ctx.ct.head, tmps + 6);
}

int main(int argc, char **argv)
{
	alignas(16) uint8_t key[32];
	str2hex128("9c8dc4bd7136dc827ca1caa3235adba49c8dc4bd7136dc827ca1caa3235adba4", key, 32);
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
	option.size_pt = 32768;
	option.size_tweak = 64;

	measurement_result res;

	set_tscfreq(argc, argv);
	set_cpufreq(argc, argv);

	if (argc < 2)
	{
		res = measure(option, wrapper_aespolyWx4);
		print_cpb("aespolyWx4", option, res);
	}

	size_t num_base, num_itr;
	num_base = get_num_base(argc, argv);
	num_itr = get_num_itr(argc, argv);

	GArray *targets;
	targets = g_array_new(false, true, sizeof(measurement_target));

	measurement_target target_aespolyWx4 = make_target("aespolyWx4", wrapper_aespolyWx4);
	measurement_target target_aespolyWx8 = make_target("aespolyWx8", wrapper_aespolyWx8);
	measurement_target target_upper = make_target("upper", wrapper_upper);
	measurement_target target_middlelower = make_target("middlelower", wrapper_middlelower);

	g_array_append_val(targets, target_aespolyWx4);
	g_array_append_val(targets, target_aespolyWx8);
	// g_array_append_val(targets, target_upper);
	// g_array_append_val(targets, target_middlelower);

	measure_save(targets, option, num_base, num_itr);

	return 0;
}