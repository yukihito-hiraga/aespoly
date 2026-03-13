#include "simpira-b2.h"

aes_context aesctx;
simpira_context ctx;

void wrapper_simpiraecb(measurement_context measure_ctx)
{
	simpira_ecb(ctx, measure_ctx.pt.head, measure_ctx.pt.size, measure_ctx.ct.head);
}

int main(int argc, char **argv)
{
	simpira_b2_init(&ctx);

	alignas(16) uint8_t key[16];
	str2hex128("9c8dc4bd7136dc827ca1caa3235adba4", key, 16);
	aesinit128(&aesctx, key);

	measurement_option option;
	option.clflush = true;
	option.core_used = 2;
	option.num_exp = 200;
	option.num_warmup = 20;

	option.size_ad = 0;
	option.size_nonce = 0;
	option.size_pt = 160000;
	option.size_tweak = 64;

	measurement_result res;

	size_t num_base, num_itr;
	num_base = get_num_base(argc, argv);
	num_itr = get_num_itr(argc, argv);

	GArray *targets;
	targets = g_array_new(false, true, sizeof(measurement_target));

	measurement_target target_simpira = make_target("simpira", wrapper_simpiraecb);

	g_array_append_val(targets, target_simpira);

	measure_save(targets, option, num_base, num_itr);

	return 0;
}