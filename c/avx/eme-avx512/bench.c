#include "eme.h"

eme_context ctx;

static inline void wrapper_eme(measurement_context measure_ctx)
{
	eme(&ctx, measure_ctx.ppt.head, measure_ctx.pt.head, measure_ctx.pt.size, measure_ctx.ct.head);
}

static inline void wrapper_xe(measurement_context measure_ctx)
{
	xe(&ctx, measure_ctx.pt.head, measure_ctx.pt.size, measure_ctx.ct.head);
}

static inline void wrapper_middle(measurement_context measure_ctx)
{
	alignas(16) __m128i M = _mm_loadu_si128((__m128i*)measure_ctx.ppt.head);
	middle(&ctx, M, measure_ctx.pt.head, measure_ctx.pt.size, measure_ctx.ct.head);
}

static inline void wrapper_ex(measurement_context measure_ctx)
{
	ex(&ctx, measure_ctx.pt.head, measure_ctx.pt.size, measure_ctx.ct.head);
}


int main(int argc, char **argv)
{

	alignas(16) uint8_t key[16];
	str2hex128("9c8dc4bd7136dc827ca1caa3235adba4", key, 16);

	emeinit(&ctx, key);

	measurement_option option;
	option.clflush = true;
	option.core_used = get_core_fromarg(argc, argv);
	option.num_exp = 200;
	option.num_warmup = 20;

	option.size_ad = 0;
	option.size_nonce = 0;
	option.size_pt = 160000;
	option.size_tweak = 64;

	set_tscfreq(argc, argv);
	set_cpufreq(argc, argv);

	measurement_result res;

	size_t num_base, num_itr;
	num_base = get_num_base(argc, argv);
	num_itr = get_num_itr(argc, argv);

	GArray *targets;
	targets = g_array_new(false, true, sizeof(measurement_target));

	measurement_target target_eme = make_target("eme", wrapper_eme);
	measurement_target target_xe = make_target("xe", wrapper_xe);
	measurement_target target_middle = make_target("middle", wrapper_middle);
	measurement_target target_ex = make_target("ex", wrapper_ex);

	g_array_append_val(targets, target_eme);
	g_array_append_val(targets, target_xe);
	g_array_append_val(targets, target_middle);
	g_array_append_val(targets, target_ex);

	measure_save(targets, option, num_base, num_itr);

	return 0;
}