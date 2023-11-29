#pragma once

/*
 *   This file is part of open_agb_firm
 *   Copyright (C) 2023 profi200
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


#define LGY9_ARM7_STUB_LOC   (0x3007E00u)
#define LGY9_ARM7_STUB_LOC9  (AHB_RAM_BASE + 0xBFE00u)
#define LGY9_SAVE_LOC        (AHB_RAM_BASE + 0x80000u)


#define LGY9_REGS_BASE  (IO_AHB_BASE + 0x18000)

typedef struct
{
	vu16 mode;
	u8 _0x2[0x7e];
	vu32 a7_vector[8];     // ARM7 vector override 32 bytes. Writable at runtime.
	u8 _0xa0[0x60];
	vu16 gba_save_type;
	u8 _0x102[2];
	vu8 gba_save_map;      // Remaps the GBA save to ARM9 if set to 1.
	u8 _0x105[3];
	vu16 gba_rtc_cnt;
	u8 _0x10a[6];
	vu32 gba_rtc_bcd_date;
	vu32 gba_rtc_bcd_time;
	vu32 gba_rtc_toffset;  // Writing bit 7 completely hangs all(?) GBA hardware.
	vu32 gba_rtc_doffset;
	vu32 gba_save_timing[4];
} Lgy9;
static_assert(offsetof(Lgy9, gba_save_timing) == 0x120, "Error: Member gba_save_timing of Lgy9 is not at offset 0x120!");

ALWAYS_INLINE Lgy9* getLgy9Regs(void)
{
	return (Lgy9*)LGY9_REGS_BASE;
}


// REG_LGY9_MODE
// See lgy_common.h LGY_MODE_...

// REG_LGY9_GBA_SAVE_TYPE
// See lgy_common.h SAVE_TYPE_...

// REG_LGY9_GBA_SAVE_MAP
#define LGY9_SAVE_MAP_7         (0u)
#define LGY9_SAVE_MAP_9         (1u)

// REG_LGY9_GBA_RTC_CNT
#define LGY9_RTC_CNT_WR         (1u)     // Write date and time.
#define LGY9_RTC_CNT_RD         (1u<<1)  // Read date and time.
#define LGY9_RTC_CNT_WR_ERR     (1u<<14) // Write error (wrong date/time).
#define LGY9_RTC_CNT_BUSY       (1u<<15)

// REG_LGY9_GBA_RTC_BCD_DATE
// Shifts
#define LGY9_RTC_BCD_Y_SHIFT    (0u)
#define LGY9_RTC_BCD_MON_SHIFT  (8u)
#define LGY9_RTC_BCD_D_SHIFT    (16u)
#define LGY9_RTC_BCD_W_SHIFT    (24u)
// Masks
#define LGY9_RTC_BCD_Y_MASK     (0xFFu<<LGY_RTC_BCD_Y_SHIFT)
#define LGY9_RTC_BCD_MON_MASK   (0x1Fu<<LGY_RTC_BCD_M_SHIFT)
#define LGY9_RTC_BCD_D_MASK     (0x3Fu<<LGY_RTC_BCD_D_SHIFT)
#define LGY9_RTC_BCD_W_MASK     (0x07u<<LGY_RTC_BCD_W_SHIFT)

// REG_LGY9_GBA_RTC_BCD_TIME
// Shifts
#define LGY9_RTC_BCD_H_SHIFT    (0u)
#define LGY9_RTC_BCD_MIN_SHIFT  (8u)
#define LGY9_RTC_BCD_S_SHIFT    (16u)
// Masks
#define LGY9_RTC_BCD_H_MASK     (0x3Fu<<LGY_RTC_BCD_H_SHIFT)
#define LGY9_RTC_BCD_MIN_MASK   (0x7Fu<<LGY_RTC_BCD_MIN_SHIFT)
#define LGY9_RTC_BCD_S_MASK     (0x7Fu<<LGY_RTC_BCD_S_SHIFT)

// REGs_LGY9_GBA_SAVE_TIMING
