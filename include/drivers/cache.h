#pragma once

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

#include "types.h"



#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief      Invalidates all instruction cache lines.
 */
void invalidateICache(void);

/**
 * @brief      Invalidates instruction cache lines in an address range.
 *
 * @param[in]  base  The base address.
 * @param[in]  size  The range size in bytes.
 */
void invalidateICacheRange(const void *base, size_t size);

/**
 * @brief      Invalidates all data cache lines.
 */
void invalidateDCache(void);

/**
 * @brief      Invalidates data cache lines in an address range.
 *
 * @param[in]  base  The base address.
 * @param[in]  size  The range size in bytes.
 */
void invalidateDCacheRange(const void *base, size_t size);

/**
 * @brief      Invalidates all instruction and data caches lines.
 */
void invalidateBothCaches(void);

/**
 * @brief      Cleans (writes back to memory) all data cache lines.
 */
void cleanDCache(void);

/**
 * @brief      Cleans (writes back to memory) data cache lines in an address range.
 *
 * @param[in]  base  The base address.
 * @param[in]  size  The range size in bytes.
 */
void cleanDCacheRange(const void *base, size_t size);

/**
 * @brief      Flushes (clean + invalidate) all data cache lines.
 */
void flushDCache(void);

/**
 * @brief      Flushes (clean + invalidate) data cache lines in an address range.
 *
 * @param[in]  base  The base address.
 * @param[in]  size  The range size in bytes.
 */
void flushDCacheRange(const void *base, size_t size);

#ifdef __cplusplus
} // extern "C"
#endif