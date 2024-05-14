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


#ifdef __cplusplus
extern "C"
{
#endif

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
#define LGY9_SAVE_MAP_7              (0u)
#define LGY9_SAVE_MAP_9              (1u)

// REG_LGY9_GBA_RTC_CNT
#define LGY9_RTC_CNT_WR              (1u)     // Write date and time.
#define LGY9_RTC_CNT_RD              (1u<<1)  // Read date and time.
#define LGY9_RTC_CNT_WR_ERR          (1u<<14) // Write error (wrong date/time).
#define LGY9_RTC_CNT_BUSY            (1u<<15)

// REG_LGY9_GBA_RTC_BCD_DATE
// Shifts
#define LGY9_RTC_BCD_Y_SHIFT         (0u)
#define LGY9_RTC_BCD_MON_SHIFT       (8u)
#define LGY9_RTC_BCD_D_SHIFT         (16u)
#define LGY9_RTC_BCD_W_SHIFT         (24u)
// Masks
#define LGY9_RTC_BCD_Y_MASK          (0xFFu<<LGY_RTC_BCD_Y_SHIFT)
#define LGY9_RTC_BCD_MON_MASK        (0x1Fu<<LGY_RTC_BCD_M_SHIFT)
#define LGY9_RTC_BCD_D_MASK          (0x3Fu<<LGY_RTC_BCD_D_SHIFT)
#define LGY9_RTC_BCD_W_MASK          (0x07u<<LGY_RTC_BCD_W_SHIFT)

// REG_LGY9_GBA_RTC_BCD_TIME
// Shifts
#define LGY9_RTC_BCD_H_SHIFT         (0u)
#define LGY9_RTC_BCD_MIN_SHIFT       (8u)
#define LGY9_RTC_BCD_S_SHIFT         (16u)
// Masks
#define LGY9_RTC_BCD_H_MASK          (0x3Fu<<LGY_RTC_BCD_H_SHIFT)
#define LGY9_RTC_BCD_MIN_MASK        (0x7Fu<<LGY_RTC_BCD_MIN_SHIFT)
#define LGY9_RTC_BCD_S_MASK          (0x7Fu<<LGY_RTC_BCD_S_SHIFT)

// REG_LGY9_GBA_RTC_TOFFSET
// Useful reference: Seiko S-3511A Real-Time Clock. The bit order is different and it has time data mixed in.
// Shifts
#define LGY9_RTC_TOFFS_S_SHIFT       (0u)
#define LGY9_RTC_TOFFS_POWER_SHIFT   (7u)
#define LGY9_RTC_TOFFS_MIN_SHIFT     (8u)
#define LGY9_RTC_TOFFS_12H24H_SHIFT  (15u)
#define LGY9_RTC_TOFFS_H_SHIFT       (16u)
#define LGY9_RTC_TOFFS_DOW_SHIFT     (24u)
#define LGY9_RTC_TOFFS_INTFE_SHIFT   (28u)
#define LGY9_RTC_TOFFS_INTME_SHIFT   (29u)
#define LGY9_RTC_TOFFS_INTAE_SHIFT   (30u)
#define LGY9_RTC_TOFFS_UNK31_SHIFT   (31u)
// Bits
#define LGY9_RTC_TOFFS_POWER_LOST    (1u<<LGY9_RTC_TOFFS_POWER_SHIFT)  // RTC power lost flag (Seiko/GBA RO, 3DS RW/set once). The RTC will generate IRQs when this is set which can trigger cart removal detection in some games.
#define LGY9_RTC_TOFFS_12H           (0u)                              // Format 12h fomrat.
#define LGY9_RTC_TOFFS_24H           (1u<<LGY9_RTC_TOFFS_12H24H_SHIFT) // Format 24h fomrat.
#define LGY9_RTC_TOFFS_INTFE_1       (1u<<LGY9_RTC_TOFFS_INTFE_SHIFT)  // 0 = alarm/per-minute edge IRQ, 1 = 50% duty cycle (minute) or frequency (if INTME/INTAE = 0) IRQ.
#define LGY9_RTC_TOFFS_INTME_1       (1u<<LGY9_RTC_TOFFS_INTME_SHIFT)  // 0 = other modes, 1 = per-minute IRQ.
#define LGY9_RTC_TOFFS_INTAE_1       (1u<<LGY9_RTC_TOFFS_INTAE_SHIFT)  // 0 = alarm IRQ disabled, 1 = alarm IRQ enabled (if INTME/INTFE = 0).
#define LGY9_RTC_TOFFS_UNK31         (1u<<LGY9_RTC_TOFFS_UNK31_SHIFT)  // Unknown. Maybe time/date offset format (decimal or BCD)?
// Masks
#define LGY9_RTC_TOFFS_S_MASK        (0x7F<<LGY9_RTC_TOFFS_S_SHIFT)    // Seconds offset mask. Not on Seiko chip.
#define LGY9_RTC_TOFFS_MIN_MASK      (0x7F<<LGY9_RTC_TOFFS_MIN_SHIFT)  // Minutes offset mask. Not on Seiko chip.
#define LGY9_RTC_TOFFS_H_MASK        (0x3F<<LGY9_RTC_TOFFS_H_SHIFT)    // Hours offset mask. Not on Seiko chip.
#define LGY9_RTC_TOFFS_DOW_MASK      (0xF<<LGY9_RTC_TOFFS_DOW_SHIFT)   // Day of week offset mask. Not on Seiko chip.

// REG_LGY9_GBA_RTC_DOFFSET
#define LGY9_RTC_DOFFS_D_MASK        (0xFFFFu) // Day offset mask. Allowed range: 0-0x8EAC. 100 years.

// REGs_LGY9_GBA_SAVE_TIMING

#ifdef __cplusplus
} // extern "C"
#endif