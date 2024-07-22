/*
 *   This file is part of libn3ds
 *   Copyright (C) 2024 derrek, profi200
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <ctype.h>
#include <string.h>
#include "types.h"
#include "util.h"



void wait_cycles(u32 cycles)
{
	__asm__ volatile
	(
#ifdef __ARM9__
		"1: subs %0, %0, #4\n\t"
#elif __ARM11__
		"1: subs %0, %0, #2\n\t"
		"yield\n\t"
#endif // #ifdef __ARM9__
		"bhi 1b"
		: "+r" (cycles)
		:
		: "cc"
	);
}

size_t safeStrcpy(char *const dst, const char *const src, size_t num)
{
	if(num == 0) return 0;

	const size_t len = strlen(src) + 1;
	if(len > num)
	{
		*dst = '\0';
		return 1;
	}

	memcpy(dst, src, len);

	return len;
}

// Limited to 6 decimal places. Doesn't support exponents.
// Based on: https://codereview.stackexchange.com/a/158724
double str2double(const char *str)
{
	while(isspace((unsigned char)*str) != 0) str++; // Skip whitespaces.

	const double sign = (*str == '-' ? -1.0 : 1.0);
	if(*str == '-' || *str == '+') str++;

	double val = 0.0;
	while(isdigit((unsigned char)*str) != 0)
	{
		val = val * 10.0 + (*str - '0');
		str++;
	}

	if(*str == '.') str++;

	u32 place = 1;
	while(isdigit((unsigned char)*str) != 0)
	{
		val = val * 10.0 + (*str - '0');
		place *= 10;
		str++;
	}

	return val * sign / place;
}

float str2float(const char *str)
{
	return str2double(str);
}