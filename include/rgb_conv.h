#pragma once

/*
 *   This file is part of libn3ds
 *   Copyright (C) 2024 profi200
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

/**
 * @brief      Converts 8-bit RGBA components to 5-bit with nearest rounding.
 *
 * @param[in]  c     The 8-bit color component.
 *
 * @return     The color component converted to 5-bit format.
 */
static inline u32 rgbEight2Five(const u32 c)
{
	// No rounding errors for 0-255.
	return (249 * c + 1024)>>11;
}

/**
 * @brief      Converts 8-bit RGBA components to 6-bit with nearest rounding.
 *
 * @param[in]  c     The 8-bit color component.
 *
 * @return     The color component converted to 6-bit format.
 */
static inline u32 rgbEight2Six(const u32 c)
{
	// No rounding errors for 0-255.
	return (253 * c + 512)>>10;
}

/**
 * @brief      Converts 5-bit RGBA components to 8-bit with nearest rounding.
 *
 * @param[in]  c     The 5-bit color component.
 *
 * @return     The color component converted to 8-bit format.
 */
static inline u32 rgbFive2Eight(const u32 c)
{
	// No rounding errors for 0-31.
	return (527 * c + 23)>>6;
}

/**
 * @brief      Converts 6-bit RGBA components to 8-bit with nearest rounding.
 *
 * @param[in]  c     The 6-bit color component.
 *
 * @return     The color component converted to 8-bit format.
 */
static inline u32 rgbSix2Eight(const u32 c)
{
	// No rounding errors for 0-63.
	return (259 * c + 33)>>6;
}

#ifdef __cplusplus
} // extern "C"
#endif