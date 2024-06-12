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

#define IRQ9_REGS_BASE  (IO_AHB_BASE + 0x1000)

typedef union
{
	struct
	{
		vu32 ie;  // 0x0
		vu32 _if; // 0x4
	};
	vu64 ie_if; // 0x0
} Irq9;
static_assert(offsetof(Irq9, _if) == 4, "Error: Member _if of Irq9 is not at offset 4!");

ALWAYS_INLINE Irq9* getIrq9Regs(void)
{
	return (Irq9*)IRQ9_REGS_BASE;
}

#ifdef __cplusplus
} // extern "C"
#endif