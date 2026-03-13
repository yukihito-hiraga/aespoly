project('test_project', 'c', default_options : ['warning_level=0', 'werror=true', 'cpp_std=c11', 'optimization=3'], meson_version : '>=1.3.0')

glib = dependency('glib-2.0', version: '>=2.56') 

args = [
	'-Wno-unused-function',
	'-Wno-unused-result',
	'-funroll-all-loops',
	'-Wno-stringop-overflow',
	'-Wno-stringop-truncation',
	'-Wno-maybe-uninitialized',
	'-Wno-unused-variable',
	'-Wno-sign-compare',
	'-Wno-uninitialized',
	'-Wno-format',
	'-Wno-unused-parameter'
]

arch = host_machine.cpu_family()
is_apple_silicon = false

c = meson.get_compiler('c')
asm_flags = c.get_supported_arguments(['-save-temps=obj', '-fverbose-asm'])
add_project_arguments(asm_flags, language: 'c')

cd = configuration_data()

avx512_supported = false

if arch == 'aarch64'
	args += '-march=native'
	args += '-mcpu=native'
	args += '-fstrict-aliasing'
	args += '-funroll-all-loops'

	rc = run_command('sh', './is_applesilicon.sh', check: false).returncode()
	is_apple_silicon = (rc == 0)

	
	message('Apple Silicon: ' + (is_apple_silicon ? 'yes' : 'no'))

	bench_hctr2_aes128 = executable('hctr2-aes128', ['c/neon/hctr2-neon/bench.c'], c_args:args, dependencies: glib)
	bench_hctr2_rijndael256 = executable('hctr2-rijndael256', ['c/neon/hctr2-rijndael256-neon/bench.c'], c_args:args, dependencies: glib)
	bench_hctr2_simpira = executable('hctr2-simpira', ['c/neon/hctr2-simpira-neon/bench.c'], c_args:args, dependencies: glib)
	
	bench_simpira = executable('simpira', ['c/neon/simpira-neon/bench.c'], c_args:args, dependencies: glib)

	bench_aesgcm = executable('aesgcm', ['c/neon/aesgcm-neon/bench.c'], c_args:args, dependencies: glib)
	bench_ocb = executable('ocb', ['c/neon/ocb-neon/bench.c'], c_args:args, dependencies: glib)

	bench_aespolyW_aes128 = executable('aespoly', ['c/neon/aespoly-neon/bench.c'], c_args:args, dependencies: glib)
	bench_aespolyW_rijndael256 = executable('aespoly-rijndael256', ['c/neon/aespoly-rijndael256-neon/bench.c'], c_args:args, dependencies: glib)
	bench_aespolyW_simpira = executable('aespoly-simpira', ['c/neon/aespoly-simpira-neon/bench.c'], c_args:args, dependencies: glib)
	bench_aespolyM = executable('ecbpoly-neon', ['c/neon/ecbpoly-neon/bench.c'], c_args:args, dependencies: glib)
	
	bench_pmac = executable('pmac', ['c/neon/pmac-neon/bench.c'], c_args:args, dependencies: glib)
	bench_gmac = executable('gmac', ['c/neon/gmac-neon/bench.c'], c_args:args, dependencies: glib)
	
	bench_eme_aes128 = executable('eme', ['c/neon/eme-neon/bench.c'], c_args:args, dependencies: glib)
	bench_eme_rijndael256 = executable('eme-rijndael256', ['c/neon/eme-rijndael256-neon/bench.c'], c_args:args, dependencies: glib)
	bench_eme_simpira = executable('eme-simpira', ['c/neon/eme-simpira-neon/bench.c'], c_args:args, dependencies: glib)
	
elif arch == 'x86_64'
	args += '-march=native'

	if c.has_argument('-mavx512f')
		testcode_avx512 = '''
		#include <immintrin.h>
		int main() {
		    __m512i a = _mm512_set1_epi32(1);
		    __m512i b = _mm512_add_epi32(a, a);
		    volatile int sink = _mm_cvtsi128_si32(_mm512_castsi512_si128(b));
		    return sink == 2 ? 0 : 1;
		}
		'''
		if c.links(testcode_avx512, args: ['-O3', '-march=native'], name: 'AVX-512 compile+link')
			avx512_supported = true
			bench_hctr2_aes128_avx512 = executable('hctr2-avx512', ['c/avx/hctr2-avx512/bench.c'], c_args:args, dependencies: glib)
			bench_aespoly_aes128_avx512 = executable('aespoly-avx512', ['c/avx/aespoly-avx512/bench.c'], c_args:args, dependencies: glib)
			bench_eme_aes128_avx512 = executable('eme-avx512', ['c/avx/eme-avx512/bench.c'], c_args:args, dependencies: glib)
		endif
	endif

	bench_hctr2_aes128 = executable('hctr2-aes128', ['c/avx/hctr2-avx/bench.c'], c_args:args, dependencies: glib)
	bench_hctr2_rijndael256 = executable('hctr2-rijndael256', ['c/avx/hctr2-rijndael256-avx/bench.c'], c_args:args, dependencies: glib)
	bench_hctr2_simpira = executable('hctr2-simpira', ['c/avx/hctr2-simpira-avx/bench.c'], c_args:args, dependencies: glib)
	
	bench_simpira = executable('simpira', ['c/avx/simpira-avx/bench.c'], c_args:args, dependencies: glib)

	bench_aesgcm = executable('aesgcm', ['c/avx/aesgcm-avx/bench.c'], c_args:args, dependencies: glib)
	bench_ocb = executable('ocb', ['c/avx/ocb-avx/bench.c'], c_args:args, dependencies: glib)

	bench_aespolyW_aes128 = executable('aespoly-aes128', ['c/avx/aespoly-avx/bench.c'], c_args:args, dependencies: glib)
	bench_aespolyW_rijndael256 = executable('aespoly-rijndael256', ['c/avx/aespoly-rijndael256-avx/bench.c'], c_args:args, dependencies: glib)
	bench_aespolyW_simpira = executable('aespoly-simpira', ['c/avx/aespoly-simpira-avx/bench.c'], c_args:args, dependencies: glib)
	bench_aespolyM = executable('ecbpoly', ['c/avx/ecbpoly-avx/bench.c'], c_args:args, dependencies: glib)
	
	bench_pmac = executable('pmac-avx', ['c/avx/pmac-avx/bench.c'], c_args:args, dependencies: glib)
	bench_gmac = executable('gmac-avx', ['c/avx/gmac-avx/bench.c'], c_args:args, dependencies: glib)
	
	bench_eme_aes128 = executable('eme-aes128', ['c/avx/eme-avx/bench.c'], c_args:args, dependencies: glib)
	bench_eme_rijndael256 = executable('eme-rijndael256', ['c/avx/eme-rijndael256-avx/bench.c'], c_args:args, dependencies: glib)
	bench_eme_simpira = executable('eme-simpira', ['c/avx/eme-simpira-avx/bench.c'], c_args:args, dependencies: glib)
endif

cd.set('avx512', avx512_supported)
cd.set_quoted('arch', arch)
cd.set('is_apple_silicon', is_apple_silicon)
configure_file(
	output: 'meta.json',
	configuration: cd,
	output_format: 'json'
)