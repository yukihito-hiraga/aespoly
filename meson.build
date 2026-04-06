project('test_project', 'c', 'cpp', default_options : ['warning_level=0', 'werror=true', 'c_std=c11', 'cpp_std=c++20', 'optimization=3'], meson_version : '>=1.3.0')

glib = dependency('glib-2.0', version: '>=2.56') 
json = dependency('nlohmann_json')
gtest = dependency('gtest')

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

	bench_aespolyW_aes128 = executable('aespolyW', ['c/neon/aespolyW-neon/bench.c'], c_args:args, dependencies: glib)
	bench_aespolyW_rijndael256 = executable('aespolyW-rijndael256', ['c/neon/aespolyW-rijndael256-neon/bench.c'], c_args:args, dependencies: glib)
	bench_aespolyW_simpira = executable('aespolyW-simpira', ['c/neon/aespolyW-simpira-neon/bench.c'], c_args:args, dependencies: glib)
	bench_aespolyM = executable('aespolyM-neon', ['c/neon/aespolyM-neon/bench.c'], c_args:args, dependencies: glib)
	
		bench_pmac = executable('pmac', ['c/neon/pmac-neon/bench.c'], c_args:args, dependencies: glib)
		bench_gmac = executable('gmac', ['c/neon/gmac-neon/bench.c'], c_args:args, dependencies: glib)
		
		bench_eme_aes128 = executable('eme', ['c/neon/eme-neon/bench.c'], c_args:args, dependencies: glib)
		bench_eme_rijndael256 = executable('eme-rijndael256', ['c/neon/eme-rijndael256-neon/bench.c'], c_args:args, dependencies: glib)
		bench_eme_simpira = executable('eme-simpira', ['c/neon/eme-simpira-neon/bench.c'], c_args:args, dependencies: glib)
		test_gmac = executable('test_gmac', ['c/neon/gmac-neon/test.cpp'], cpp_args: args, dependencies: [json, gtest, glib])
		test_pmac = executable('test_pmac', ['c/neon/pmac-neon/test.cpp'], cpp_args: args, dependencies: [json, gtest, glib])
		test_aesgcm = executable('test_aesgcm', ['c/neon/aesgcm-neon/test.cpp'], cpp_args: args, dependencies: [json, gtest, glib])
		test_ocb = executable('test_ocb', ['c/neon/ocb-neon/test.cpp'], cpp_args: args, dependencies: [json, gtest, glib])
		test_aespolyW_aes128 = executable('test_aespolyW_aes128', ['c/neon/aespolyW-neon/test.cpp'], cpp_args: args, dependencies: [json, gtest, glib])
		test_aespolyW_rijndael256 = executable('test_aespolyW_rijndael256', ['c/neon/aespolyW-rijndael256-neon/test.cpp'], cpp_args: args, dependencies: [json, gtest, glib])
		test_aespolyW_simpira = executable('test_aespolyW_simpira', ['c/neon/aespolyW-simpira-neon/test.cpp'], cpp_args: args, dependencies: [json, gtest, glib])
		test_aespolyM = executable('test_aespolyM', ['c/neon/aespolyM-neon/test.cpp'], cpp_args: args, dependencies: [json, gtest, glib])
		test_hctr2_aes128 = executable('test_hctr2_aes128', ['c/neon/hctr2-neon/test.cpp'], cpp_args: args, dependencies: [json, gtest, glib])
		test_hctr2_rijndael256 = executable('test_hctr2_rijndael256', ['c/neon/hctr2-rijndael256-neon/test.cpp'], cpp_args: args, dependencies: [json, gtest, glib])
		test_hctr2_simpira = executable('test_hctr2_simpira', ['c/neon/hctr2-simpira-neon/test.cpp'], cpp_args: args, dependencies: [json, gtest, glib])
		test_eme_aes128 = executable('test_eme_aes128', ['c/neon/eme-neon/test.cpp'], cpp_args: args, dependencies: [json, gtest, glib])
		test_eme_rijndael256 = executable('test_eme_rijndael256', ['c/neon/eme-rijndael256-neon/test.cpp'], cpp_args: args, dependencies: [json, gtest, glib])
		test_eme_simpira = executable('test_eme_simpira', ['c/neon/eme-simpira-neon/test.cpp'], cpp_args: args, dependencies: [json, gtest, glib])
		
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
			bench_aespolyW_aes128_avx512 = executable('aespolyW-avx512', ['c/avx/aespolyW-avx512/bench.c'], c_args:args, dependencies: glib)
			bench_eme_aes128_avx512 = executable('eme-avx512', ['c/avx/eme-avx512/bench.c'], c_args:args, dependencies: glib)
			test_hctr2_aes128_avx512 = executable('test_hctr2_aes128_avx512', ['c/avx/hctr2-avx512/test.cpp'], cpp_args: args, dependencies: [json, gtest, glib])
			test_aespolyW_aes128_avx512 = executable('test_aespolyW_aes128_avx512', ['c/avx/aespolyW-avx512/test.cpp'], cpp_args: args, dependencies: [json, gtest, glib])
			test_eme_aes128_avx512 = executable('test_eme_aes128_avx512', ['c/avx/eme-avx512/test.cpp'], cpp_args: args, dependencies: [json, gtest, glib])
		endif
	endif

	bench_hctr2_aes128 = executable('hctr2-aes128', ['c/avx/hctr2-avx/bench.c'], c_args:args, dependencies: glib)
	bench_hctr2_rijndael256 = executable('hctr2-rijndael256', ['c/avx/hctr2-rijndael256-avx/bench.c'], c_args:args, dependencies: glib)
	bench_hctr2_simpira = executable('hctr2-simpira', ['c/avx/hctr2-simpira-avx/bench.c'], c_args:args, dependencies: glib)
	
	bench_simpira = executable('simpira', ['c/avx/simpira-avx/bench.c'], c_args:args, dependencies: glib)

	bench_aesgcm = executable('aesgcm', ['c/avx/aesgcm-avx/bench.c'], c_args:args, dependencies: glib)
	bench_ocb = executable('ocb', ['c/avx/ocb-avx/bench.c'], c_args:args, dependencies: glib)

	bench_aespolyW_aes128 = executable('aespolyW-aes128', ['c/avx/aespolyW-avx/bench.c'], c_args:args, dependencies: glib)
	bench_aespolyW_rijndael256 = executable('aespolyW-rijndael256', ['c/avx/aespolyW-rijndael256-avx/bench.c'], c_args:args, dependencies: glib)
	bench_aespolyW_simpira = executable('aespolyW-simpira', ['c/avx/aespolyW-simpira-avx/bench.c'], c_args:args, dependencies: glib)
	bench_aespolyM = executable('aespolyM', ['c/avx/aespolyM-avx/bench.c'], c_args:args, dependencies: glib)
	
	bench_pmac = executable('pmac-avx', ['c/avx/pmac-avx/bench.c'], c_args:args, dependencies: glib)
	bench_gmac = executable('gmac-avx', ['c/avx/gmac-avx/bench.c'], c_args:args, dependencies: glib)
	
	bench_eme_aes128 = executable('eme-aes128', ['c/avx/eme-avx/bench.c'], c_args:args, dependencies: glib)
	bench_eme_rijndael256 = executable('eme-rijndael256', ['c/avx/eme-rijndael256-avx/bench.c'], c_args:args, dependencies: glib)
	bench_eme_simpira = executable('eme-simpira', ['c/avx/eme-simpira-avx/bench.c'], c_args:args, dependencies: glib)

	test_gmac = executable('test_gmac', ['c/avx/gmac-avx/test.cpp'], cpp_args: args, dependencies: [json, gtest, glib])
	test_pmac = executable('test_pmac', ['c/avx/pmac-avx/test.cpp'], cpp_args: args, dependencies: [json, gtest, glib])
	test_aesgcm = executable('test_aesgcm', ['c/avx/aesgcm-avx/test.cpp'], cpp_args: args, dependencies: [json, gtest, glib])
	test_ocb = executable('test_ocb', ['c/avx/ocb-avx/test.cpp'], cpp_args: args, dependencies: [json, gtest, glib])
	test_aespolyW_aes128 = executable('test_aespolyW_aes128', ['c/avx/aespolyW-avx/test.cpp'], cpp_args: args, dependencies: [json, gtest, glib])
	test_aespolyW_rijndael256 = executable('test_aespolyW_rijndael256', ['c/avx/aespolyW-rijndael256-avx/test.cpp'], cpp_args: args, dependencies: [json, gtest, glib])
	test_aespolyW_simpira = executable('test_aespolyW_simpira', ['c/avx/aespolyW-simpira-avx/test.cpp'], cpp_args: args, dependencies: [json, gtest, glib])
	test_aespolyM = executable('test_aespolyM', ['c/avx/aespolyM-avx/test.cpp'], cpp_args: args, dependencies: [json, gtest, glib])
	test_hctr2_aes128 = executable('test_hctr2_aes128', ['c/avx/hctr2-avx/test.cpp'], cpp_args: args, dependencies: [json, gtest, glib])
	test_hctr2_rijndael256 = executable('test_hctr2_rijndael256', ['c/avx/hctr2-rijndael256-avx/test.cpp'], cpp_args: args, dependencies: [json, gtest, glib])
	test_hctr2_simpira = executable('test_hctr2_simpira', ['c/avx/hctr2-simpira-avx/test.cpp'], cpp_args: args, dependencies: [json, gtest, glib])
	test_eme_aes128 = executable('test_eme_aes128', ['c/avx/eme-avx/test.cpp'], cpp_args: args, dependencies: [json, gtest, glib])
	test_eme_rijndael256 = executable('test_eme_rijndael256', ['c/avx/eme-rijndael256-avx/test.cpp'], cpp_args: args, dependencies: [json, gtest, glib])
	test_eme_simpira = executable('test_eme_simpira', ['c/avx/eme-simpira-avx/test.cpp'], cpp_args: args, dependencies: [json, gtest, glib])
endif

cd.set('avx512', avx512_supported)
cd.set_quoted('arch', arch)
cd.set('is_apple_silicon', is_apple_silicon)
configure_file(
	output: 'meta.json',
	configuration: cd,
	output_format: 'json'
)
