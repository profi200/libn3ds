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
#include "arm9/drivers/irq9.h"
#include "arm9/drivers/interrupt.h"


IrqIsr g_irqIsrTable[32] = {0};



void IRQ_init(void)
{
	getIrq9Regs()->ie_if = UINT64_C(0xFFFFFFFF)<<32;
}

void IRQ_registerIsr(Interrupt id, IrqIsr isr)
{
	const u32 oldState = enterCriticalSection();

	g_irqIsrTable[id] = isr;
	getIrq9Regs()->ie |= BIT(id);

	leaveCriticalSection(oldState);
}

void IRQ_unregisterIsr(Interrupt id)
{
	const u32 oldState = enterCriticalSection();

	getIrq9Regs()->ie &= ~BIT(id);
	g_irqIsrTable[id] = (IrqIsr)NULL;

	leaveCriticalSection(oldState);
}