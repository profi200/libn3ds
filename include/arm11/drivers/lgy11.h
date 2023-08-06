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

#include <assert.h>
#include "types.h"
#include "mem_map.h"


#define LGY11_REGS_BASE  (IO_MEM_ARM9_ARM11 + 0x41100)

typedef struct
{
	vu16 mode;         // 0x00
	u8 _0x2[2];
	vu16 sleep;        // 0x04
	u8 _0x6[2];
	const vu16 unk08;  // 0x08 IRQ related?
	const vu16 padcnt; // 0x0A Read-only mirror of ARM7 "KEYCNT".
	u8 _0xc[4];
	vu16 pad_sel;      // 0x10 Select which keys to override. 1 = selected.
	vu16 pad_val;      // 0x12 Override value. Each bit 0 = pressed.
	vu16 gpio_sel;     // 0x14 Select which GPIOs to override. 1 = selected.
	vu16 gpio_val;     // 0x16 Override value.
	vu8 unk18;         // 0x18 DSi gamecard detection select?
	vu8 unk19;         // 0x19 DSi gamecard detection value?
	u8 _0x1a[6];
	const vu8 unk20;   // 0x20
} Lgy11;
static_assert(offsetof(Lgy11, unk20) == 0x20, "Error: Member unk20 of Lgy11 is not at offset 0x20!");

ALWAYS_INLINE Lgy11* getLgy11Regs(void)
{
	return (Lgy11*)LGY11_REGS_BASE;
}



void LGY11_switchMode(void);

static inline void LGY11_setInputState(const u16 pressed)
{
	Lgy11 *const lgy11 = getLgy11Regs();
	lgy11->pad_val = ~pressed;
}

static inline void LGY11_selectInput(const u16 inputSelect)
{
	Lgy11 *const lgy11 = getLgy11Regs();
	lgy11->pad_sel = inputSelect;
}

void LGY11_deinit(void);
