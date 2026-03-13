#include "ocb.h"

ocb_context ctx;

void wrapper_ocb(measurement_context measure_ctx)
{
	ocbenc128(ctx, 16, measure_ctx.nonce.head, measure_ctx.nonce.size, measure_ctx.ad.head, measure_ctx.ad.size, measure_ctx.pt.head, measure_ctx.pt.size - 16, measure_ctx.ct.head);
}

void wrapper_ocb_offline(measurement_context measure_ctx)
{
	ocbenc128_offline(ctx, 16, measure_ctx.nonce.head, measure_ctx.nonce.size, measure_ctx.ad.head, measure_ctx.ad.size, measure_ctx.pt.head, measure_ctx.pt.size - 16, measure_ctx.ct.head);
}

void wrapper_ocbx4(measurement_context measure_ctx)
{
	ocbenc128x4(ctx, 16, measure_ctx.nonce.head, measure_ctx.nonce.size, measure_ctx.ad.head, measure_ctx.ad.size, measure_ctx.pt.head, measure_ctx.pt.size - 16, measure_ctx.ct.head);
}

void wrapper_ocbx4_offline(measurement_context measure_ctx)
{
	ocbenc128x4_offline(ctx, 16, measure_ctx.nonce.head, measure_ctx.nonce.size, measure_ctx.ad.head, measure_ctx.ad.size, measure_ctx.pt.head, measure_ctx.pt.size - 16, measure_ctx.ct.head);
}

void wrapper_ocbx8_offline(measurement_context measure_ctx)
{
	ocbenc128x8_offline(ctx, 16, measure_ctx.nonce.head, measure_ctx.nonce.size, measure_ctx.ad.head, measure_ctx.ad.size, measure_ctx.pt.head, measure_ctx.pt.size - 16, measure_ctx.ct.head);
}


int main(int argc, char **argv)
{
	alignas(16) uint8_t key[16];
	str2hex128("9c8dc4bd7136dc827ca1caa3235adba4", key, 16);
	ocbinit128(&ctx, key, false);

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
		res = measure(option, wrapper_ocb);
		print_cpb("ocb", option, res);

		res = measure(option, wrapper_ocbx4);
		print_cpb("ocbx4", option, res);
	}

	size_t num_base, num_itr;
	num_base = get_num_base(argc, argv);
	num_itr = get_num_itr(argc, argv);

	GArray *targets;
	targets = g_array_new(false, true, sizeof(measurement_target));

	measurement_target target_ocb = make_target("ocb", wrapper_ocb);
	measurement_target target_ocb_offline = make_target("ocb_offline", wrapper_ocb_offline);
	measurement_target target_ocbx4 = make_target("ocbx4", wrapper_ocbx4);
	measurement_target target_ocbx4_offline = make_target("ocbx4_offline", wrapper_ocbx4_offline);
	measurement_target target_ocbx8_offline = make_target("ocbx8_offline", wrapper_ocbx8_offline);

	g_array_append_val(targets, target_ocb);
	g_array_append_val(targets, target_ocb_offline);
	g_array_append_val(targets, target_ocbx4);
	g_array_append_val(targets, target_ocbx4_offline);
	g_array_append_val(targets, target_ocbx8_offline);

	measure_save(targets, option, num_base, num_itr);

	return 0;
}