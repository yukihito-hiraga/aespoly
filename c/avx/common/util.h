#pragma once

#include "headers.h"

typedef struct _bytearray
{
	size_t size;
	uint8_t *head;
} bytearray;

bytearray alloc_bytearray(size_t align, size_t size)
{
	bytearray res;
	res.size = size;
	res.head = (uint8_t *)aligned_alloc(align, size);
	return res;
}

void memset_bytearray(bytearray b, uint8_t val)
{
	memset(b.head, val, b.size);
}

uint8_t accumulate_bytearray(bytearray b)
{
	uint8_t res = 0;
	for (size_t i = 0; i < b.size; i++)
	{
		res ^= b.head[i];
	}
	return res;
}

void clflush_bytearray(bytearray b)
{
	for (size_t _i = 0; _i < b.size / 64; _i++)
	{
		_mm_clflush(b.head + _i * 64);
	}
}

void free_bytearray(bytearray b)
{
	if (b.head == NULL)
		free(b.head);
	b.head = NULL;
}

static int char2hex(char c)
{
	if (c >= 'a' && c <= 'f')
	{
		return (c - 'a') + 10;
	}
	if (c >= 'A' && c <= 'F')
	{
		return (c - 'A') + 10;
	}
	return c - '0';
}

static int hex2char(int x)
{
	if (x >= 10)
	{
		return 'a' + (x - 10);
	}
	return '0' + x;
}

static void str2hex128(const char *str, uint8_t *dat, size_t len)
{
	for (size_t i = 0; i < len; i++)
	{
		dat[i] = char2hex(str[2 * i]) * 16 + char2hex(str[2 * i + 1]);
	}
}

static void str2hexr128(const char *str, uint8_t *dat, size_t len)
{
	for (size_t i = 0; i < len; i++)
	{
		dat[i] = char2hex(str[2 * (len - 1 - i)]) * 16 + char2hex(str[2 * (len - 1 - i) + 1]);
	}
}

static void printhex(uint8_t *dat, size_t len)
{
	for (size_t i = 0; i < len; i++)
	{
		printf("%c%c", hex2char(dat[i] / 16), hex2char(dat[i] % 16));
	}
}

static void printhexr(uint8_t *dat, size_t len)
{
	for (size_t i = 0; i < len; i++)
	{
		printf("%c%c", hex2char(dat[len - 1 - i] / 16), hex2char(dat[len - 1 - i] % 16));
	}
}

static size_t len_strhex(const char *str)
{
	size_t len = strlen(str);
	assert(len % 2 == 0);
	return len / 2;
}
