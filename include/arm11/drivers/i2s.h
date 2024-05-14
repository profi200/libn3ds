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

#define I2S_REGS_BASE  (IO_COMMON_BASE + 0x45000)

typedef struct
{
	vu16 i2s1_cnt; // 0x0
	vu16 i2s2_cnt; // 0x2
} I2sRegs;
static_assert(offsetof(I2sRegs, i2s2_cnt) == 2, "Error: Member i2s2_cnt of I2sRegs is not at offset 2!");

ALWAYS_INLINE I2sRegs* getI2sRegs(void)
{
	return (I2sRegs*)I2S_REGS_BASE;
}


// I2S1_CNT
// Note: Bits 12-14 are only writable if the enable bit is 0.
#define I2S1_DSP_VOL(v)   ((v) & 0x3Fu)      // DSP master volume. 32 is 100%.
#define I2S1_LGY_VOL(v)   (((v) & 0x3Fu)<<6) // GBA and DS(i) hardware master volume. 32 is 100%.
#define I2S1_UNK12        (1u<<12)           // Enables the microphone input? Some kind of input/output select.
#define I2S1_FREQ_32KHZ   (0u)               // 268111856 / 8192 = 32728.498046875 Hz.
#define I2S1_FREQ_47KHZ   (1u<<13)           // 268111856 / 5632 = 47605.088068181818181818182 Hz.
#define I2S1_MCLK1_8MHZ   (0u)               // CODEC MCLK1. 268111856 / 32 / (1000 * 1000) = 8.3784955 MHz.
#define I2S1_MCLK1_16MHZ  (1u<<14)           // CODEC MCLK1. 268111856 / 16 / (1000 * 1000) = 16.756991 MHz.
#define I2S1_EN           (1u<<15)           // Enable I2S1 interface.

// I2S2_CNT
// Note: Bits 13, 14 are only writable if the enable bit is 0.
#define I2S2_FREQ_32KHZ   (0u)     // 268111856 / 8192 = 32728.498046875 Hz.
#define I2S2_FREQ_47KHZ   (1u<<13) // 268111856 / 5632 = 47605.088068181818181818182 Hz.
#define I2S2_MCLK2_8MHZ   (0u)     // CODEC MCLK2. 268111856 / 32 / (1000 * 1000) = 8.3784955 MHz.
#define I2S2_MCLK2_16MHZ  (1u<<14) // CODEC MCLK2. 268111856 / 16 / (1000 * 1000) = 16.756991 MHz.
#define I2S2_EN           (1u<<15) // Enable I2S2 interface.



static inline void I2S1_setDspVolume(const u8 vol)
{
	I2sRegs *const i2s = getI2sRegs();
	i2s->i2s1_cnt = (i2s->i2s1_cnt & ~I2S1_DSP_VOL(0x3Fu)) | I2S1_DSP_VOL(vol);
}

static inline void I2S1_setLgyVolume(const u8 vol)
{
	I2sRegs *const i2s = getI2sRegs();
	i2s->i2s1_cnt = (i2s->i2s1_cnt & ~I2S1_LGY_VOL(0x3Fu)) | I2S1_LGY_VOL(vol);
}

#ifdef __cplusplus
} // extern "C"
#endif