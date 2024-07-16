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
#include "mem_map.h"
#include "kevent.h"


#ifdef __cplusplus
extern "C"
{
#endif

#define LGYCAP_TOP_REGS_BASE  (IO_COMMON_BASE + 0x11000)
#define LGYCAP_BOT_REGS_BASE  (IO_COMMON_BASE + 0x10000)

#define LGYCAP_TOP_FIFO       *((const vu32*)(LGYCAP_TOP_REGS_BASE + 0x200000)) // Data abort on CPU access.
#define LGYCAP_BOT_FIFO       *((const vu32*)(LGYCAP_BOT_REGS_BASE + 0x200000)) // Data abort on CPU access.

typedef struct
{
	vu32 len;          // 0x00
	vu32 patt;         // 0x04
	u8 _0x8[0x38];
	vu32 matrix[6][8]; // 0x40
} LgyCapScaler;
static_assert(offsetof(LgyCapScaler, matrix[5][7]) == 0xFC, "Error: Member matrix[5][7] of LgyCapScaler is not at offset 0xFC!");

typedef struct
{
	vu32 cnt;             // 0x000
	vu32 dim;             // 0x004 Capture dimensions.
	vu32 stat;            // 0x008
	vu32 irq;             // 0x00C
	vu32 flush;           // 0x010 Write 0 to flush LgyCap FIFO.
	u8 _0x14[0xc];
	vu32 alpha;           // 0x020 8 bit alpha for all pixels. Only bit 7 used for A1BGR5.
	u8 _0x24[0xcc];
	vu32 unkF0;           // 0x0F0
	u8 _0xf4[0xc];
	vu32 dither_patt0;    // 0x100 4x2 pattern bits (mask 0xCCCC). 0xCCCC disables dithering.
	u8 _0x104[4];
	vu32 dither_patt1;    // 0x108 4x2 pattern bits (mask 0xCCCC). 0xCCCC disables dithering.
	u8 _0x10c[4];
	vu32 dither_patt2;    // 0x110 4x2 pattern bits (mask 0xCCCC). 0xCCCC disables dithering.
	u8 _0x114[4];
	vu32 dither_patt3;    // 0x118 4x2 pattern bits (mask 0xCCCC). 0xCCCC disables dithering.
	u8 _0x11c[0xe4];
	LgyCapScaler vscaler; // 0x200
	LgyCapScaler hscaler; // 0x300
} LgyCap;
static_assert(offsetof(LgyCap, hscaler.matrix[5][7]) == 0x3FC, "Error: Member hscaler.matrix[5][7] of LgyCap is not at offset 0x3FC!");

ALWAYS_INLINE LgyCap* getLgyCapRegs(const u8 dev)
{
	return (LgyCap*)(dev == 0 ? LGYCAP_BOT_REGS_BASE : LGYCAP_TOP_REGS_BASE);
}


// REG_LGYCAP_CNT
#define LGYCAP_EN             BIT(0)   // LgyCap capture enable.
#define LGYCAP_VSCALE_EN      BIT(1)   // Vertical hardware scaling enable.
#define LGYCAP_HSCALE_EN      BIT(2)   // Horizontal hardware scaling enable.
#define LGYCAP_UNK_BIT4       BIT(4)   // Broken spatial dither enable? Doesn't do anything.
#define LGYCAP_UNK_BIT5       BIT(5)   // Broken temporal dither enable? Doesn't do anything.
#define LGYCAP_FMT_ABGR8      (0u)     // {0xAA, 0xBB, 0xGG, 0xRR} in memory from lowest to highest address.
#define LGYCAP_FMT_BGR8       (1u<<8)  // {0xBB, 0xGG, 0xRR} in memory from lowest to highest address.
#define LGYCAP_FMT_A1BGR5     (2u<<8)  // {0bGGBBBBBA, 0bRRRRRGGG} in memory from lowest to highest address.
#define LGYCAP_FMT_BGR565     (3u<<8)  // {0bGGGBBBBB, 0bRRRRRGGG} in memory from lowest to highest address.
#define LGYCAP_ROT_NONE       (0u)     // No block rotation.
#define LGYCAP_ROT_90CW       (1u<<10) // Rotate 8x8 blocks 90° clockwise.
#define LGYCAP_ROT_180CW      (2u<<10) // Rotate 8x8 blocks 180° clockwise.
#define LGYCAP_ROT_270CW      (3u<<10) // Rotate 8x8 blocks 270° clockwise.
#define LGYCAP_SWIZZLE        BIT(12)  // Morton swizzled output.
#define LGYCAP_DMA_EN         BIT(15)  // DMA request enable.
#define LGYCAP_IN_FMT         BIT(16)  // Use input format but this bit does nothing?

// REG_LGYCAP_DIM width and height
#define LGYCAP_DIM(w, h)      (((h) - 1)<<16 | ((w) - 1))

// REG_LGYCAP_STAT and REG_LGYCAP_IRQ
// IRQ flags in STAT reg are acknowledged on write.
#define LGYCAP_IRQ_DMA_REQ    BIT(0) // Fires for every 8 lines in the FIFO.
#define LGYCAP_IRQ_BUF_ERR    BIT(1) // FIFO overrun?
#define LGYCAP_IRQ_VBLANK     BIT(2)
#define LGYCAP_IRQ_MASK       (LGYCAP_IRQ_VBLANK | LGYCAP_IRQ_BUF_ERR | LGYCAP_IRQ_DMA_REQ)
#define LGYCAP_OUT_LINE(reg)  ((reg)>>16) // STAT only. Current output line in the FIFO.


typedef struct
{
	u32 cnt; // REG_LGYCAP_CNT settings.
	u16 w;
	u16 h;
	u8 vLen;
	u8 vPatt;
	s16 vMatrix[8 * 6];
	// TODO: DMA output address?
	u8 hLen;
	u8 hPatt;
	s16 hMatrix[8 * 6];
	// TODO: DMA output address?
} LgyCapCfg;

typedef enum
{
	LGYCAP_DEV_BOT = 0u,
	LGYCAP_DEV_TOP = 1u
} LgyCapDev;



KHandle LGYCAP_init(const LgyCapDev dev, const LgyCapCfg *const cfg); // Returns frame ready KEvent.
void LGYCAP_deinit(const LgyCapDev dev);
void LGYCAP_stop(const LgyCapDev dev);
void LGYCAP_start(const LgyCapDev dev);

#ifdef __cplusplus
} // extern "C"
#endif