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
#include "mem_map.h"


#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __ARM11__
#define SHA_REGS_BASE  (IO_COMMON_BASE + 0x1000)
#elif __ARM9__
#define SHA_REGS_BASE  (IO_AHB_BASE + 0xA000)
#endif // #ifdef __ARM11__

// Vectorizing the FIFO improves code generation at the cost of being slightly slower for small data.
typedef u32 ShaFifo __attribute__((vector_size(64)));

typedef struct
{
	vu32 cnt;              // 0x00
	vu32 blkcnt;           // 0x04
	u8 _0x8[0x38];
	vu32 hash[8];          // 0x40
	u8 _0x60[0x20];
	volatile ShaFifo fifo; // 0x80 The FIFO is in the DMA region instead on ARM11.
} Sha;
static_assert(offsetof(Sha, fifo) == 0x80, "Error: Member fifo of Sha is not at offset 0x80!");

ALWAYS_INLINE Sha* getShaRegs(void)
{
	return (Sha*)SHA_REGS_BASE;
}

ALWAYS_INLINE volatile ShaFifo* getShaFifo(Sha *const regs)
{
#ifdef __ARM11__
	return (volatile ShaFifo*)((uintptr_t)regs + 0x200000);
#else
	return &regs->fifo;
#endif // #ifdef __ARM11__
}


// REG_SHA_CNT
#define SHA_EN           BIT(0)  // Also used as busy flag.
#define SHA_FINAL_ROUND  BIT(1)  // Final round/add input padding.
#define SHA_I_DMA_EN     BIT(2)  // Input DMA enable.
#define SHA_IN_BIG       BIT(3)
#define SHA_IN_LITTLE    (0u)
#define SHA_OUT_BIG      (SHA_IN_BIG)
#define SHA_OUT_LITTLE   (SHA_IN_LITTLE)
#define SHA_256_MODE     (0u)
#define SHA_224_MODE     (1u<<4)
#define SHA_1_MODE       (2u<<4)
#define SHA_MODE_MASK    (SHA_1_MODE | SHA_224_MODE | SHA_256_MODE)
#define SHA_RB_MODE      BIT(8)  // Readback mode.
#define SHA_RB_FIFO_NE   BIT(9)  // Readback mode FIFO not empty status.
#define SHA_O_DMA_EN     BIT(10) // Output DMA enable (readback mode).



/**
 * @brief      Sets input mode, endianess and starts the hash operation.
 *
 * @param[in]  params  Extra parameters like endianess. See REG_SHA_CNT defines above.
 */
void SHA_start(u16 params);

/**
 * @brief      Hashes the data pointed to.
 *
 * @param[in]  data  Pointer to data to hash.
 * @param[in]  size  Size of the data to hash.
 */
void SHA_update(const u32 *data, u32 size);

/**
 * @brief      Generates the final hash.
 *
 * @param      hash       Pointer to memory to copy the hash to.
 * @param[in]  endianess  Endianess bitmask for the hash.
 */
void SHA_finish(u32 *const hash, u16 endianess);

/**
 * @brief      Returns the current SHA engine state.
 *
 * @param      out   Pointer to memory to copy the state to.
 */
void SHA_getState(u32 *const out);

/**
 * @brief      Hashes a single block of data and outputs the hash.
 *
 * @param[in]  data           Pointer to data to hash.
 * @param[in]  size           Size of the data to hash.
 * @param      hash           Pointer to memory to copy the hash to.
 * @param[in]  params         Extra parameters like endianess. See REG_SHA_CNT defines above.
 * @param[in]  hashEndianess  Endianess bitmask for the hash.
 */
void sha(const u32 *data, u32 size, u32 *const hash, u16 params, u16 hashEndianess);

/**
 * @brief      Hashes a single block of data with DMA and outputs the hash.
 *
 * @param[in]  data           Pointer to data to hash.
 * @param[in]  size           Size of the data to hash.
 * @param      hash           Pointer to memory to copy the hash to.
 * @param[in]  params         Extra parameters like endianess. See REG_SHA_CNT defines above.
 * @param[in]  hashEndianess  Endianess bitmask for the hash.
 */
#ifdef __ARM9__
void sha_dma(const u32 *data, u32 size, u32 *const hash, u16 params, u16 hashEndianess);
#endif // #ifdef __ARM9__

#ifdef __cplusplus
} // extern "C"
#endif