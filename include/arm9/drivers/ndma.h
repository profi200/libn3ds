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

/*
 * H̲a̲r̲d̲w̲a̲r̲e̲ ̲b̲u̲g̲ ̲w̲a̲r̲n̲i̲n̲g̲.̲
 * If the source address of the NDMA transfer is within the range of VRAM start to FCRAM end (AXI bus)
 * the ARM11 cores completely hang. This hang can only be fixed by doing a dummy read from the
 * source address of the NDMA transfer using the ARM9 CPU.
 * In practice this bug will cause transfer size dependent stalls of the ARM11 cores degrading
 * performance significantly.
 */

#include "types.h"
#include "mem_map.h"
#include "util.h"


#ifdef __cplusplus
extern "C"
{
#endif

#define NDMA_REGS_BASE  (IO_AHB_BASE + 0x2000)
#define REG_NDMA_GCNT   *((vu32*)NDMA_REGS_BASE) // Global control.

// Note: The channel regs are offset by 4 (REG_NDMA_GCNT).
typedef struct
{
	vu32 sad;   // 0x00 Source address.
	vu32 dad;   // 0x04 Destination address.
	vu32 tcnt;  // 0x08 Total repeat length in words.
	vu32 wcnt;  // 0x0C Logical block size in words.
	vu32 bcnt;  // 0x10 Block transfer timing/interval.
	vu32 fdata; // 0x14 Fill data.
	vu32 cnt;   // 0x18 Control.
} NdmaCh;
static_assert(offsetof(NdmaCh, cnt) == 0x18, "Error: Member cnt of NdmaCh is not at offset 0x18!");

ALWAYS_INLINE NdmaCh* getNdmaChRegs(u8 c)
{
	return &((NdmaCh*)(NDMA_REGS_BASE + 4))[c];
}


// REG_NDMA_GCNT
#define NDMA_REG_READBACK    BIT(0)                           // Show internal state on REG_NDMAx_SAD/DAD/TCNT/WCNT. 3DS mode only.
#define NDMA_ROUND_ROBIN(n)  (BIT(31) | (intLog2(n) + 1)<<16) // DSP DMA/CPU cycles (power of 2). Maximum 16384.
#define NDMA_HIGHEST_PRIO    (0u)

// REG_NDMA_BCNT
// Inserts a delay with the specified amount of cycles after each physical block transfer.
// TODO: Does NDMA run at 67 MHz in 3DS mode? We will assume so for now.
#define NDMA_CYCLES(n)       (n)      // Maximum 65535. 0 means no delay/interval.
#define NDMA_PRESCALER_1     (0u)     // 67027964 Hz.
#define NDMA_PRESCALER_4     (1u<<16) // 16756991 Hz.
#define NDMA_PRESCALER_16    (2u<<16) // 4189247.75 Hz.
#define NDMA_PRESCALER_64    (3u<<16) // 1047311.9375 Hz.
#define NDMA_FASTEST         (NDMA_PRESCALER_1 | NDMA_CYCLES(0)) // Convenience macro.

// REG_NDMA_CNT
#define NDMA_DAD_INC         (0u)     // Destination address increment.
#define NDMA_DAD_DEC         (1u<<10) // Destination address decrement.
#define NDMA_DAD_FIX         (2u<<10) // Destination address fixed.
#define NDMA_DAD_RELOAD      BIT(12)  // Reload destination address on logical block end (REG_NDMAx_WCNT).
#define NDMA_SAD_INC         (0u)     // Source address increment.
#define NDMA_SAD_DEC         (1u<<13) // Source address decrement.
#define NDMA_SAD_FIX         (2u<<13) // Source address fixed.
#define NDMA_SAD_FILL        (3u<<13) // Source is REG_NDMAx_FDATA.
#define NDMA_SAD_RELOAD      BIT(15)  // Reload source address on logical block end (REG_NDMAx_WCNT).
#define NDMA_BURST_SHIFT     (16u)
#define NDMA_BURST(n)        (intLog2(n)<<NDMA_BURST_SHIFT) // Burst length is 2ⁿ words. Maximum 32768. Must be power of 2.
#define NDMA_TCNT_MODE       (0u)     // REG_NDMAx_TCNT mode.
#define NDMA_REPEAT_MODE     BIT(29)  // Repeat transfer infinitely.
#define NDMA_IRQ_EN          BIT(30)  // IRQ enable.
#define NDMA_EN              BIT(31)  // Channel enable/active.


enum
{
	NDMA_START_TIMER0       =  0u<<24,
	NDMA_START_TIMER1       =  1u<<24,
	NDMA_START_TIMER2       =  2u<<24,
	NDMA_START_TIMER3       =  3u<<24,
	NDMA_START_CTRCARD1     =  4u<<24, // And for SPICARD.
	NDMA_START_CTRCARD2     =  5u<<24, // And for SPICARD?
	NDMA_START_TMIO1        =  6u<<24,
	NDMA_START_TMIO3        =  7u<<24,
	NDMA_START_AES_IN       =  8u<<24, // AES write fifo.
	NDMA_START_AES_OUT      =  9u<<24, // AES read fifo.
	NDMA_START_SHA_IN       = 10u<<24,
	NDMA_START_SHA_OUT      = 11u<<24, // For chaining.
	NDMA_START_NTRCARD      = 12u<<24,
	// 13 and 14 unused?
	NDMA_START_DEV2DEV      = 15u<<24, // Needed for below startup modes.

	// Direct device to device modes.
	NDMA_START_CTRCARD1_AES = NDMA_START_DEV2DEV |  0u, // CTRCARD1 to AES.
	NDMA_START_CTRCARD2_AES = NDMA_START_DEV2DEV |  1u, // CTRCARD2 to AES.
	NDMA_START_AES_CTRCARD1 = NDMA_START_DEV2DEV |  2u, // AES to CTRCARD1.
	NDMA_START_AES_CTRCARD2 = NDMA_START_DEV2DEV |  3u, // AES to CTRCARD2.
	NDMA_START_CTRCARD1_SHA = NDMA_START_DEV2DEV |  4u, // CTRCARD1 to SHA.
	NDMA_START_CTRCARD2_SHA = NDMA_START_DEV2DEV |  5u, // CTRCARD2 to SHA.
	NDMA_START_SHA_CTRCARD1 = NDMA_START_DEV2DEV |  6u, // SHA to CTRCARD1.
	NDMA_START_SHA_CTRCARD2 = NDMA_START_DEV2DEV |  7u, // SHA to CTRCARD2.
	NDMA_START_TMIO1_AES    = NDMA_START_DEV2DEV |  8u, // TMIO1 to AES.
	NDMA_START_TMIO3_AES    = NDMA_START_DEV2DEV |  9u, // TMIO3 to AES.
	NDMA_START_AES_TMIO1    = NDMA_START_DEV2DEV | 10u, // AES to TMIO1.
	NDMA_START_AES_TMIO3    = NDMA_START_DEV2DEV | 11u, // AES to TMIO3.
	NDMA_START_TMIO1_SHA    = NDMA_START_DEV2DEV | 12u, // TMIO1 to SHA.
	NDMA_START_TMIO3_SHA    = NDMA_START_DEV2DEV | 13u, // TMIO3 to SHA.
	NDMA_START_SHA_TMIO1    = NDMA_START_DEV2DEV | 14u, // SHA to TMIO1.
	NDMA_START_SHA_TMIO3    = NDMA_START_DEV2DEV | 15u, // SHA to TMIO3.
	NDMA_START_AES_SHA      = NDMA_START_DEV2DEV | 16u, // AES to SHA.
	NDMA_START_SHA_AES      = NDMA_START_DEV2DEV | 17u, // SHA to AES.
	// 18-31 unused?

	// This bit is technically not part of the startup modes.
	NDMA_START_IMMEDIATE    = 16u<<24
};



/**
 * @brief      Initializes all NDMA channels.
 */
void NDMA_init(void);

/**
 * @brief      Copies data using the NDMA engine.
 *
 * @param      dst   Pointer to destination memory. Must be 4 bytes aligned.
 * @param      src   Pointer to source data. Must be 4 bytes aligned.
 * @param[in]  size  The size of the data. Must be multiple of 4.
 */
void NDMA_copy(u32 *const dst, const u32 *const src, u32 size);

/**
 * @brief      Fills memory with the given value using the NDMA engine.
 *
 * @param      dst    Pointer to destination memory. Must be 4 bytes aligned.
 * @param[in]  value  The value each 32-bit word will be set to.
 * @param[in]  size   The size of the memory to fill. Must be multiple of 4.
 */
void NDMA_fill(u32 *const dst, const u32 value, u32 size);

#ifdef __cplusplus
} // extern "C"
#endif