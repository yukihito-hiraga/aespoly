#pragma once

#include "headers.h"
#include "util.h"

#define __USE_GNU
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <linux/module.h>
#include <sched.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>

#include <stdbool.h>
#include <stdlib.h>

#include <glib.h>

size_t tsc_freq_hz = 0;
size_t cpu_freq_hz = 0;

typedef struct _cpuid_info
{
    uint32_t eax, ebx, ecx, edx;
} cpuid_info;

typedef struct _rdmsr_info
{
    uint32_t eax, edx;
} rdmsr_info;

cpuid_info cpuid(int leaf, int subleaf)
{
    cpuid_info ret;
    __asm__ __volatile__("cpuid"
                         : "=a"(ret.eax), "=b"(ret.ebx), "=c"(ret.ecx), "=d"(ret.edx)
                         : "a"(leaf), "c"(subleaf));
    return ret;
}

size_t get_corecrystalfreq()
{
    cpuid_info info = cpuid(0x15, 0);
    size_t res = info.ecx;
    return res;
}

size_t get_tscfreq()
{
    cpuid_info info = cpuid(0x15, 0);
    if (info.ebx != 0)
    {
        size_t res = get_corecrystalfreq() * info.ebx / info.eax;
        return res;
    }
    return 0;
}

void set_tscfreq(int argc, char **argv)
{
    tsc_freq_hz = get_tscfreq();
    if (tsc_freq_hz < 1000000)
    {
        if (argc > 4)
        {
            char *endptr;
            long int num = strtol(argv[4], &endptr, 10);
            if (*endptr == '\0')
            {
                tsc_freq_hz = num;
            }
        }
    }
    printf("TSC freq %ld\n", tsc_freq_hz);
}

void set_cpufreq(int argc, char **argv)
{
    if (argc > 5)
    {
        char *endptr;
        long int num = strtol(argv[5], &endptr, 10);
        if (*endptr == '\0')
        {
            cpu_freq_hz = num;
        }
    }
    printf("cpu freq %ld (from script)\n", cpu_freq_hz);
}

size_t get_cur_freq(int core)
{
    FILE *fp;
    char file[70];
    sprintf(file, "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_cur_freq", core);
    fp = fopen(file, "r");
    if (fp == 0)
    {
        return 0;
    }
    size_t cur_freq_kHz;
    fscanf(fp, "%ld", &cur_freq_kHz);
    fclose(fp);
    return cur_freq_kHz * 1000;
}

/*
static int read_msr(int cpu, unsigned int idx, unsigned long *val)
{
    int fd;
    char msr_file_name[64];

    __asm__ __volatile__("mfence \n\t");

    sprintf(msr_file_name, "/dev/cpu/%d/msr", cpu);
    fd = open(msr_file_name, O_RDONLY);
    if (fd < 0)
        return -1;
    if (lseek(fd, idx, SEEK_CUR) == -1)
    {
        close(fd);
        return -1;
    }
    if (read(fd, val, sizeof *val) != sizeof *val)
    {
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}
*/

static int read_msr(int cpu, unsigned int idx)
{
    FILE *fp;

    char cmd[50];
    snprintf(cmd, sizeof(cmd), "rdmsr -c -p%d %#x", cpu, idx);

    if (!(fp = popen(cmd, "r")))
    {
        printf("Error reading from \"%s\"", cmd);
        return 0;
    }

    char buf[20];
    fgets(buf, sizeof(buf), fp);
    pclose(fp);

    uint64_t val;
    val = strtoul(buf, NULL, 0);
    return val;
}

uint64_t get_aperf(int cpu)
{
    uint64_t aperf = read_msr(cpu, 0xe8);
    return aperf;
}

double ratio_op_tsc(int core)
{
    return 1.0 * get_cur_freq(core) / get_tscfreq();
}

size_t get_max_freq(int core)
{
    FILE *fp;
    char file[70];
    sprintf(file, "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_max_freq", core);
    fp = fopen(file, "r");
    if (fp == 0)
    {
        return 0;
    }
    size_t cur_freq_kHz;
    fscanf(fp, "%ld", &cur_freq_kHz);
    fclose(fp);
    return cur_freq_kHz * 1000;
}

void set_cpu_affinity(size_t i)
{
    pid_t pid;
    cpu_set_t cpu_set;
    int res;
    pid = getpid();
    CPU_ZERO(&cpu_set);
    CPU_SET(i, &cpu_set);
    res = sched_setaffinity(pid, sizeof(cpu_set_t), &cpu_set);

    if (res != 0)
    {
        printf("To set cpu affinity is failed.\n");
    }
}

static char saved_cpufreq_governor[65];

static int get_num_cpus(void)
{
    static int ncpus;

    if (ncpus <= 0)
        ncpus = sysconf(_SC_NPROCESSORS_ONLN);

    if (ncpus <= 0)
    {
        fprintf(stderr, "Unable to determine number of CPUs, assuming 1\n");
        ncpus = 1;
    }
    return ncpus;
}

static inline int get_core_fromarg(int argc, char **argv)
{
    if (argc > 3)
    {
        char *endptr;
        long int num = strtol(argv[3], &endptr, 10);
        if (*endptr == '\0')
        {
            printf("cpu used is set to %ld\n", num);
            return num;
        }
    }
    return 0;
}

static void set_cpufreq_governor(const char *governor)
{
    int cpu, ncpus = get_num_cpus();

    for (cpu = 0; cpu < ncpus; cpu++)
    {
        char path[128];
        char cur_governor[64];
        int fd;
        int res;

        sprintf(path, "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_governor", cpu);
        fd = open(path, O_RDONLY);
        if (fd < 0)
        {
            // fprintf(stderr, "Unable to open '%s' for reading: %s\n", path, strerror(errno));
            continue;
        }
        res = read(fd, cur_governor, sizeof(cur_governor) - 1);
        if (res < 0)
        {
            fprintf(stderr, "Error reading '%s': %s\n", path, strerror(errno));
            close(fd);
            continue;
        }
        close(fd);
        if (res > 0 && cur_governor[res - 1] == '\n')
            res--;
        cur_governor[res] = '\0';
        if (!strcmp(cur_governor, governor))
            continue;
        fd = open(path, O_WRONLY);
        if (fd < 0)
        {
            if (errno == EACCES)
            {
                fprintf(
                    stderr,
                    "This program is not authorized to change the CPU frequency scaling governor (currently '%s').\n"
                    "Recommend re-running with sudo or 'adb root'\n",
                    cur_governor);
                break;
            }
            fprintf(stderr, "Unable to open '%s' for writing: %s\n", path, strerror(errno));
            break;
        }
        if (write(fd, governor, strlen(governor)) != strlen(governor))
        {
            fprintf(stderr, "Error setting '%s' CPU frequency scaling governor: %s\n", governor, strerror(errno));
            close(fd);
            break;
        }
        if (strcmp(governor, saved_cpufreq_governor) != 0)
            strncpy(saved_cpufreq_governor, cur_governor, sizeof(saved_cpufreq_governor));
        close(fd);
    }
}

static inline uint64_t rdtsc_inline(void)
{
    uint32_t cycles_high0, cycles_low0;
    __asm__ __volatile__("mfence \n\t"
                         "RDTSCP \n\t"
                         "mov %%edx, %0\n\t"
                         "mov %%eax, %1\n\t"
                         : "=r"(cycles_high0), "=r"(cycles_low0)::"%rax", "%rbx", "%rcx", "%rdx");
    return (((uint64_t)cycles_high0) << 32) | cycles_low0;
}

typedef struct _measurement_option
{
    bool clflush;
    int core_used;
    size_t num_exp;
    size_t num_warmup;
    size_t size_tweak, size_ad, size_pt, size_nonce;
} measurement_option;

typedef struct _measurement_context
{
    bytearray tweak, ad, pt, nonce, ct, ppt;
} measurement_context;

typedef struct _measurement_result
{
    double min_cpb, avg_cpb, avg_cycles;
    size_t min_cycles;
    uint8_t _dummy;
} measurement_result;

typedef void measure_func(measurement_context);

measurement_result measure(measurement_option option, measure_func *code)
{
    measurement_context ctx;
    ctx.ad = alloc_bytearray(64, option.size_ad);
    ctx.tweak = alloc_bytearray(64, option.size_tweak);
    ctx.nonce = alloc_bytearray(64, option.size_nonce);
    ctx.pt = alloc_bytearray(64, option.size_pt);
    ctx.ppt = alloc_bytearray(64, option.size_pt);
    ctx.ct = alloc_bytearray(64, option.size_pt);

    memset_bytearray(ctx.ad, 1);
    memset_bytearray(ctx.tweak, 1);
    memset_bytearray(ctx.nonce, 1);
    memset_bytearray(ctx.pt, 1);
    memset_bytearray(ctx.ppt, 1);
    memset_bytearray(ctx.ct, 1);

    measurement_result res;
    res.avg_cycles = 0;
    res.min_cycles = 1 << 30;
    long long t_start, t_end;

    double clocktscratio = 1.0;
    bool available_tsc = (tsc_freq_hz > 0);

    // assume that initialization has done
    // information is contained in context value
    for (int i = -((int64_t)option.num_warmup); i < ((int64_t)option.num_exp); ++i)
    {
        if (option.clflush)
        {
            clflush_bytearray(ctx.ad);
            clflush_bytearray(ctx.tweak);
            clflush_bytearray(ctx.nonce);
            clflush_bytearray(ctx.pt);
            clflush_bytearray(ctx.ppt);
            clflush_bytearray(ctx.ct);
        }
        {
            memset_bytearray(ctx.ad, (uint8_t)i);
            memset_bytearray(ctx.tweak, (uint8_t)i);
            memset_bytearray(ctx.nonce, (uint8_t)i);
            memset_bytearray(ctx.pt, (uint8_t)i);
            memset_bytearray(ctx.ppt, (uint8_t)i);
            memset_bytearray(ctx.ct, (uint8_t)i);
        }
        t_start = rdtsc_inline();
        // t_start = get_aperf(2);
        __asm__ __volatile__("" ::: "memory");
        code(ctx);
        __asm__ __volatile__("" ::: "memory");
        t_end = rdtsc_inline();
        // t_end = get_aperf(2);
        if (i >= 0)
        {
            // double cycles = (t_end - t_start) * ratio_op_tsc(option.core_used);
            double cycles = (t_end - t_start);
            if (available_tsc)
            {
                clocktscratio = get_cur_freq(option.core_used) * 1.0 / tsc_freq_hz;
                if(clocktscratio < 0.001){
                    clocktscratio = 1.0;
                    if(cpu_freq_hz > 0){
                        clocktscratio = cpu_freq_hz * 1.0 / tsc_freq_hz;
                    }
                }
                cycles *= clocktscratio;
            }
            res.avg_cycles += cycles;
            if (cycles < res.min_cycles)
                res.min_cycles = cycles;
            {
                res._dummy ^= accumulate_bytearray(ctx.ct);
            }
        }
    }
    res.avg_cycles /= option.num_exp;
    size_t all_size = option.size_ad + option.size_tweak + option.size_pt;

    res.min_cpb = ((double)res.min_cycles) / all_size;
    res.avg_cpb = res.avg_cycles / all_size;

    free_bytearray(ctx.tweak);
    free_bytearray(ctx.ad);
    free_bytearray(ctx.nonce);
    free_bytearray(ctx.pt);
    free_bytearray(ctx.ppt);
    free_bytearray(ctx.ct);

    return res;
}

void print_cpb(const char *name, measurement_option option, measurement_result res)
{
    printf("%s\t: num_exp=%ld, num_warmup=%ld, size_ad=%ld, size_tweak=%ld, size_nonce=%ld, size_pt=%ld\t: cpb_min=%f, "
           "cpb_avg=%f\n",
           name, option.num_exp, option.num_warmup, option.size_ad, option.size_tweak, option.size_nonce,
           option.size_pt, res.min_cpb, res.avg_cpb);
}

size_t get_num_itr(int argc, char **argv)
{
    if (argc > 2)
    {
        char *endptr;
        long int num = strtol(argv[2], &endptr, 10);
        if (*endptr == '\0')
        {
            return num;
        }
    }
    return 20;
}

size_t get_num_base(int argc, char **argv)
{
    if (argc > 1)
    {
        char *endptr;
        long int num = strtol(argv[1], &endptr, 10);
        if (*endptr == '\0')
        {
            return num;
        }
    }
    return 16384;
}

typedef struct _measurement_target
{
    char name[100];
    measure_func *code;
} measurement_target;

measurement_target make_target(char *name, measure_func *code)
{
    measurement_target res;
    snprintf(res.name, 70, "%s", name);
    res.code = code;
    return res;
}

void measure_save(GArray *targets, measurement_option option, size_t num_base, size_t num_itr)
{
    set_cpu_affinity(option.core_used);
    set_cpufreq_governor("performance");

    char *name;
    measurement_result res;
    name = (char *)calloc(150, sizeof(char));

    for (size_t k = 0; k < targets->len; k++)
    {
        snprintf(name, 120, "result/%s.csv", g_array_index(targets, measurement_target, k).name);
        FILE *fp;
        fp = fopen(name, "w");
		chmod(name, 0666);
        if (fp != NULL)
        {
            for (size_t i = 1; i <= num_itr; i++)
            {
                option.size_pt = num_base * i;
                res = measure(option, g_array_index(targets, measurement_target, k).code);
                printf("\r %s %ld/%ld", g_array_index(targets, measurement_target, k).name, i, num_itr);
                fflush(stdout);
                fprintf(fp, "%ld,%f,%f\n", option.size_pt, res.min_cpb, res.avg_cpb);
            }
            fclose(fp);
            printf("\n");
        }
    }
    free(name);
}