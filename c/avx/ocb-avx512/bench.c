#include "ocb.h"

aes_context aesctx;
ocb_context ctx;

void wrapper_ocb(measurement_context measure_ctx)
{
	ocbenc512(aesctx, ctx, 16, measure_ctx.nonce.head, measure_ctx.nonce.size, measure_ctx.ad.head, measure_ctx.ad.size, measure_ctx.pt.head, measure_ctx.pt.size - 16, measure_ctx.ct.head);
}

void wrapper_ocbx4(measurement_context measure_ctx)
{
	ocbenc512x4(aesctx, ctx, 16, measure_ctx.nonce.head, measure_ctx.nonce.size, measure_ctx.ad.head, measure_ctx.ad.size, measure_ctx.pt.head, measure_ctx.pt.size - 16, measure_ctx.ct.head);
}

void wrapper_ocbx4mod(measurement_context measure_ctx)
{
	ocbenc512x4_modified(aesctx, ctx, 16, measure_ctx.nonce.head, measure_ctx.nonce.size, measure_ctx.ad.head, measure_ctx.ad.size, measure_ctx.pt.head, measure_ctx.pt.size - 16, measure_ctx.ct.head);
}

int main()
{

	ctx.poly = _mm512_setr_epi64(0x87, 0, 0x87, 0, 0x87, 0, 0x87, 0);
	ctx.blocklength = 16;

	alignas(16) uint8_t key[16];
	str2hex128("9c8dc4bd7136dc827ca1caa3235adba4", key, 16);
	ocbinit512(&aesctx, &ctx, key);

	measurement_option option;
	option.clflush = false;
	option.core_used = 2;
	option.num_exp = 1000;
	option.num_warmup = 100;

	set_cpu_affinity(option.core_used);
	set_cpufreq_governor("performance");

	option.size_ad = 0;
	option.size_nonce = 15;
	option.size_pt = 160000;
	option.size_tweak = 64;

	measurement_result res;

	res = measure(option, wrapper_ocb);
	print_cpb("ocb", option, res);

	res = measure(option, wrapper_ocbx4);
	print_cpb("ocbx4", option, res);

	res = measure(option, wrapper_ocbx4mod);
	print_cpb("ocbx4mod", option, res);

		return 0;
}