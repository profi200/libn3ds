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


#ifdef __cplusplus
extern "C"
{
#endif

#define FCRAM_REGS_BASE  (IO_AXI_BASE + 0x1000)

// FCRAM configuration registers. Only takes effect after a reset.
typedef struct
{
	vu32 cfg0;     // 0x00 (A)synchronous mode config, signal slew rate and driver strength?
	u8 _0x4[0xc];
	vu32 cfg1;     // 0x10 Latency settings?
	u8 _0x14[0xc];
	vu32 cfg2;     // 0x20 Clock settings?
} Fcram;
static_assert(offsetof(Fcram, cfg2) == 0x20, "Error: Member cfg2 of Fcram is not at offset 0x20!");

ALWAYS_INLINE Fcram* getFcramRegs(void)
{
	return (Fcram*)FCRAM_REGS_BASE;
}


// REG_FCRAM_CFG0
#define FCRAM_CFG0_MODE_ASYNC     (0u)   // Switches the FCRAM into asynchronous mode? Fixed timings. Slow.
#define FCRAM_CFG0_MODE_SYNC      BIT(0) // Switches the FCRAM into synchronous mode? Varying timings. Fast.
#define FCRAM_CFG0_SLEW_FAST      (0u)   // Probably fast slew rate. Described as "Pre Driver Strength" in Fujitsu datasheets.
#define FCRAM_CFG0_SLEW_SLOW      BIT(1) // Probably slow slew rate.
#define FCRAM_CFG0_STRENGTH_NORM  (0u)   // Probably normal driver strength according to Fujitsu datasheets.
#define FCRAM_CFG0_STRENGTH_WEAK  BIT(2) // // Probably weak driver strength.
// Bits 3 and 4 unknown. Another timing setting.

// REG_FCRAM_CFG1
// Bits 0 and 1 unknown delay value.
// Bits 16-18 another unknown delay value. Smaller is faster. Default 2.

// REG_FCRAM_CFG2
// Bits 0-3 unknown clock related value. Delay?
// Bits 8-11 another unknown clock related value. Delay?
// Bit 31 clock select maybe?

#ifdef __cplusplus
} // extern "C"
#endif