#pragma once

#include "headers.h"
#include "util.h"

static bool eq_uint8array(uint8_t *a, uint8_t *b, size_t len)
{
	for (size_t i = 0; i < len; i++)
	{
		if (a[i] != b[i])
			return false;
	}
	return true;
}

bool eq_bytearray(bytearray a, bytearray b)
{
	if (a.size != b.size)
	{
		return false;
	}
	return eq_uint8array(a.head, b.head, a.size);
}

#ifdef __AVX512VL__

void myprint_m512(__m512i x, char *tag)
{
	const uint8_t *x8 = (uint8_t *)&x;
	for (int word = 0; word < 4; ++word)
	{
		printf("[%s%d] ", tag, word);
		for (size_t i = 0; i < 16; i++)
		{
			printf("%02x", x8[word * 16 + i]);
			if (i % 4 == 3)
				putchar(' ');
		}
		printf("\n");
	}
}

#endif

void myprint_m128(__m128i x, char *tag)
{
	const uint8_t *x8 = (uint8_t *)&x;
	printf("[%s] ", tag);
	for (int i = 0; i < 16; ++i)
	{
		printf("%02x", x8[15 - i]);
		if (i % 4 == 3)
			putchar(' ');
	}
	putchar('\n');
	//  const uint64_t * x64 = (uint64_t *) &x;
	//  printf("%016lx %016lx\n", x64[0], x64[1]);
}

typedef struct _testvalue
{
	char *name;
	bytearray entity;
	int attribute; // 0: input, 1: output
} testvalue;

typedef struct _testunit_info
{
	testvalue *head;
	size_t size;
	size_t capacity;
} testunit_info;

typedef struct _test_context
{
	testunit_info *units;
	size_t size;
	size_t capacity;
} test_context;

testunit_info init_testunit(size_t capacity)
{
	testunit_info res;
	res.head = (testvalue *)calloc(sizeof(testvalue), capacity);
	res.size = 0;
	res.capacity = capacity;
	return res;
}

test_context init_test(size_t capacity)
{
	test_context res;
	res.units = (testunit_info *)calloc(sizeof(testunit_info), capacity);
	res.size = 0;
	res.capacity = capacity;
	return res;
}

// if not found, return -1
int name2index_unit(testunit_info info, const char *name)
{
	for (size_t i = 0; i < info.size; i++)
	{
		if (strcmp(info.head[i].name, name) == 0)
		{
			return i;
		}
	}
	return -1;
}

// if not found, return namedbytearray whose attribute is -1
testvalue getdata_unit(testunit_info info, const char *name)
{
	int i = name2index_unit(info, name);
	testvalue res;
	if (i == -1)
	{
		res.entity.head = NULL;
		res.entity.size = 0;
		res.name = "";
		res.attribute = -1;
		return res;
	}
	return info.head[i];
}

void add_testinfo(testunit_info *info, const char *name, int attribute, bytearray entity)
{
	int i = name2index_unit(*info, name);
	if (i > -1)
	{
		return;
	}

	if (info->size == info->capacity)
	{
		size_t newcapacity = info->capacity * 2;
		testvalue *newhead = (testvalue *)calloc(sizeof(testvalue), newcapacity);
		memcpy(newhead, info->head, info->capacity);
		free(info->head);
		info->head = newhead;
		info->capacity = newcapacity;
	}
	info->head[info->size].name = (char *)calloc(sizeof(char), strlen(name) + 10);
	strncpy(info->head[info->size].name, name, strlen(name));
	info->head[info->size].entity = entity;
	info->head[info->size].attribute = attribute;
	info->size++;
}

void add_test(test_context *ctx, testunit_info unit)
{
	if (ctx->size == ctx->capacity)
	{
		size_t newcapacity = ctx->capacity * 2;
		testunit_info *newhead = (testunit_info *)calloc(sizeof(testunit_info), newcapacity);
		memcpy(newhead, ctx->units, ctx->capacity);
		free(ctx->units);
		ctx->units = newhead;
		ctx->capacity = newcapacity;
	}

	ctx->units[ctx->size] = unit;
	ctx->size++;
}

void add_testinfo_from_str(testunit_info *info, const char *name, int attribute, const char *str)
{
	size_t len = len_strhex(str);
	bytearray b = alloc_bytearray(1, len);
	str2hex128(str, b.head, len);
	add_testinfo(info, name, attribute, b);
}

void add_testinfo_from_src(testunit_info *info, const char *src)
{
	char *name = (char *)calloc(sizeof(char), (strlen(src) + 1));
	char *str = (char *)calloc(sizeof(char), (strlen(src) + 1));
	char attr;
	int name_i = 0, str_i = 0, attr_i = 0;
	int state = 0;
	for (size_t i = 0; i < strlen(src); i++)
	{
		if (state == 0)
		{
			if (src[i] == ':')
			{
				state = 1;
			}
			else if (!isblank(src[i]))
			{
				name[name_i] = src[i];
				name_i++;
			}
		}
		else if (state == 1)
		{
			if (src[i] == ':')
			{
				state = 2;
			}
			else if (!isblank(src[i]) && attr_i == 0)
			{
				attr = src[i] - '0';
			}
		}
		else if (state == 2)
		{
			if (!isblank(src[i]))
			{
				str[str_i] = src[i];
				str_i++;
			}
		}
	}

	add_testinfo_from_str(info, name, attr, str);

	free(name);
	free(str);
}

testunit_info make_testinfo_from_line(const char *src)
{
	testunit_info info = init_testunit(10);
	size_t len = strlen(src) + 1;
	char *unit_src = (char *)calloc(sizeof(char), len);
	int unit_i = 0;
	for (size_t i = 0; i < len; i++)
	{
		if (src[i] == ',')
		{
			add_testinfo_from_src(&info, unit_src);
			unit_i = 0;
			memset(unit_src, 0, len);
		}
		else
		{
			unit_src[unit_i] = src[i];
			unit_i++;
		}
	}
	if (unit_i > 0)
	{
		add_testinfo_from_src(&info, unit_src);
	}

	free(unit_src);
	return info;
}

void add_test_from_src(test_context *ctx, const char *src)
{
	size_t len = strlen(src) + 1;
	char *line_src = (char *)calloc(sizeof(char), len);
	int line_i = 0;
	for (size_t i = 0; i < len; i++)
	{
		if (src[i] == '\n')
		{
			add_test(ctx, make_testinfo_from_line(line_src));
			line_i = 0;
			memset(line_src, 0, len);
		}
		else
		{
			line_src[line_i] = src[i];
			line_i++;
		}
	}
	if (line_i > 0)
	{
		add_test(ctx, make_testinfo_from_line(line_src));
	}
	free(line_src);
}

void add_test_from_file(test_context *ctx, const char *path)
{
	FILE *fp;
	fp = fopen(path, "r");

	fseek(fp, 0, SEEK_END);
	long fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	char *string = calloc(sizeof(char), fsize + 1);
	fread(string, fsize, 1, fp);
	fclose(fp);

	string[fsize] = 0;

	add_test_from_src(ctx, string);

	free(string);
}

void destruct_testunit(testunit_info info)
{
	for (size_t i = 0; i < info.size; i++)
	{
		free_bytearray(info.head[i].entity);
		free(info.head[i].name);
	}
	free(info.head);
	info.head = NULL;
}

void destruct_test(test_context ctx)
{
	for (size_t i = 0; i < ctx.size; i++)
	{
		destruct_testunit(ctx.units[i]);
	}
	free(ctx.units);
	ctx.units = NULL;
}

typedef struct _unitresultvalue
{
	char *name;
	bytearray entity;
} unitresultvalue;

typedef struct _unitresult
{
	unitresultvalue *values;
	size_t size;
} unitresult;

typedef struct _testresult
{
	size_t error_i;
	bool is_passed;
	unitresult result_actual;
} testresult;

unitresult
init_testresult(size_t size)
{
	unitresult res;
	res.size = size;
	res.values = (unitresultvalue *)calloc(sizeof(unitresultvalue), size);
	for (size_t i = 0; i < size; i++)
	{
		res.values[i].name = NULL;
	}

	return res;
}

void destruct_testresult(unitresult res)
{
	for (size_t i = 0; i < res.size; i++)
	{
		if (res.values[i].name != NULL)
		{
			free_bytearray(res.values[i].entity);
			free(res.values[i].name);
			res.values[i].name = NULL;
		}
	}
	free(res.values);
}

void print_testvalue(testvalue v)
{
	printf("%s:", v.name);
	printhex(v.entity.head, v.entity.size);
}

void print_testunit(testunit_info info, int attribute)
{
	bool first = true;
	for (size_t i = 0; i < info.size; i++)
	{
		if (info.head[i].attribute == attribute)
		{
			if (!first)
			{
				printf(", ");
			}
			first = false;
			print_testvalue(info.head[i]);
		}
	}
	printf("\n");
}

bytearray getdata_testresult(unitresult result, const char *name)
{
	for (size_t i = 0; i < result.size; i++)
	{
		if (strcmp(result.values[i].name, name) == 0)
		{
			return result.values[i].entity;
		}
	}
	bytearray nullbytes;
	nullbytes.head = NULL;
	nullbytes.size = 0;
	return nullbytes;
}

bool match_test(testunit_info info, unitresult result)
{
	for (size_t i = 0; i < info.size; i++)
	{
		if (info.head[i].attribute == 1)
		{
			if (!eq_bytearray(info.head[i].entity, getdata_testresult(result, info.head[i].name)))
			{
				return false;
			}
		}
	}
	return true;
}

unitresult make_testresult(const testunit_info info)
{
	unitresult res = init_testresult(info.size);
	int res_i = 0;
	for (size_t i = 0; i < info.size; i++)
	{
		if (info.head[i].attribute == 1)
		{

			res.values[res_i].name = (char *)calloc(sizeof(char), (strlen(info.head[i].name) + 10));
			strncpy(res.values[res_i].name, info.head[i].name, strlen(info.head[i].name) + 1);
			res.values[res_i]
				.entity = alloc_bytearray(1, info.head[i].entity.size);
			res_i++;
		}
	}
	return res;
}

// if test succeeds, return testresult whose size is zero
testresult test(test_context ctx, void (*testunit)(testunit_info, unitresult))
{
	int i;
	testresult res;
	for (i = 0; i < ctx.size; i++)
	{
		testunit_info info = ctx.units[i];
		unitresult result = make_testresult(info);
		testunit(info, result);
		if (!match_test(info, result))
		{
			res.error_i = i;
			res.is_passed = false;
			res.result_actual = result;
			return res;
		}
		destruct_testresult(result);
	}
	res.is_passed = true;
	return res;
}

void print_unitresultvalue(unitresultvalue v)
{
	printf("%s:", v.name);
	printhex(v.entity.head, v.entity.size);
}

void print_unitresult(unitresult res)
{
	bool first = true;
	for (size_t i = 0; i < res.size; i++)
	{
		if (res.values[i].name != NULL)
		{
			if (!first)
			{
				printf(", ");
			}
			first = false;
			print_unitresultvalue(res.values[i]);
		}
	}
	printf("\n");
}

void print_testresult(const char *testname, test_context ctx, testresult res)
{
	printf("test %s\n", testname);
	if (!res.is_passed)
	{
		printf(" error at %ld\n", res.error_i);
		printf(" case :: ");
		print_testunit(ctx.units[res.error_i], 0);
		printf(" correct result :: ");
		print_testunit(ctx.units[res.error_i], 1);
		printf(" actual  result :: ");
		print_unitresult(res.result_actual);
	}
	else
	{
		printf(" succeeds\n");
	}
}

void free_result(testresult *res)
{
	destruct_testresult(res->result_actual);
}

void clear_test(test_context *ctx)
{
	for (size_t i = 0; i < ctx->size; i++)
	{
		destruct_testunit(ctx->units[i]);
	}

	ctx->size = 0;
}
