#pragma once

/*
 *   This file is part of open_agb_firm
 *   Copyright (C) 2023 derrek, profi200
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

#include "types.h"


#ifdef __cplusplus
extern "C"
{
#endif

#define ARRAY_ENTRIES(a)  (sizeof(a) / sizeof(*a))



/**
 * @brief      Waits at least the specified amount of CPU cycles.
 *
 * @param[in]  cycles  The cycles to wait.
 */
void wait_cycles(u32 cycles);

/**
 * @brief      Safer strcpy with checks.
 *             The dst string always gets terminated except when num is 0.
 *             If the src string is too long nothing is copied and dst will be terminated.
 *             This function is not safe against race conditions!
 *
 * @param      dst   The destination pointer.
 * @param[in]  src   The source pointer.
 * @param[in]  num   Maximum number of chars to copy including null terminator.
 *
 * @return     The length of the copied string in bytes including null terminator.
 */
size_t safeStrcpy(char *const dst, const char *const src, size_t num);

/**
 * @brief      Basic string to float conversion. Limited to 6 decimal places.
 *             Doesn't support exponents.
 *
 * @param[in]  str   The string.
 *
 * @return     The floatingpoint number represented by str.
 */
float str2float(const char *str);

/**
 * @brief      Calculates the integer log2 of value.
 *
 * @param[in]  val   The input value.
 *
 * @return     log2 of val.
 */
static inline u32 intLog2(u32 val)
{
	// The result is undefined if __builtin_clzl() is called with 0.
	return (val ? 31u - __builtin_clzl(val) : val);
}

/**
 * @brief      Round up to the next power of 2.
 *
 * @param[in]  val   The input value. Ranges outside 2 - 2147483648 are undefined behavior!
 *
 * @return     Next power of 2 of val.
 */
static inline u32 nextPow2(u32 val)
{
	// Portable variant:
	// https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
	/*val--;
	val |= val>>1;
	val |= val>>2;
	val |= val>>4;
	val |= val>>8;
	val |= val>>16;
	val++;

	return val;*/

	// Warning: Allowed range is 2 - 2147483648.
	// Everything else is undefined behavior.
	return 1u<<(32u - __builtin_clzl(val - 1));
}

/**
 * @brief      Converts a 8 bit BCD number to decimal.
 *
 * @param[in]  bcd   The BCD number.
 *
 * @return     Decimal representaion of bcd.
 */
static inline u8 bcd2dec(const u8 bcd)
{
	// https://stackoverflow.com/a/42340213
    return bcd - 6u * (bcd>>4);
}

/**
 * @brief      Clamps s32 number x between min and max.
 *
 * @param[in]  x     The value to clamp.
 * @param[in]  min   The minimum.
 * @param[in]  max   The maximum.
 *
 * @return     Clamped value of x.
 */
static inline s32 clamp_s32(const s32 x, const s32 min, const s32 max)
{
	return (x < min ? min : (x > max ? max : x));
}

#ifdef __cplusplus
} // extern "C"
#endif