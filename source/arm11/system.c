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
#include "arm11/drivers/scu.h"
#include "drivers/pxi.h"
#include "arm11/start.h"
#include "arm11/drivers/interrupt.h"
#include "arm11/drivers/timer.h"
#include "kernel.h"
#include "drivers/corelink_dma-330.h"
#include "arm11/drivers/i2c.h"
#include "arm11/drivers/mcu.h"
#include "arm11/drivers/hid.h"
#include "arm11/drivers/hw_cal.h"
#include "arm.h"
#include "fs.h"



// NAKED to prevent unnecessary register pushes to the stack.
[[noreturn]] NAKED static void core1Standby(void)
{
	while(1)
	{
		// Abuse SCU monitor counter register 3 for temporary pointer storage.
		// This is a little cleaner than writing pointers to RAM reserved for heap.
		Scu *const scu = getScuRegs();
		scu->mn3 = (u32)NULL;

		// Register IPI1 IRQ without a handler.
		IRQ_registerIsr(IRQ_IPI1, 14, 0, (IrqIsr)NULL);

		// Wait for IPI1 IRQ and a valid entry pointer.
		void (*entry)(void);
		do
		{
			__wfi();
			entry = (void (*)(void))scu->mn3;
		} while(entry == NULL);

		// Unregister IPI1 IRQ.
		IRQ_unregisterIsr(IRQ_IPI1);

		// Jump to entrypoint.
		entry();
	}
}

void WEAK __systemInit(void)
{
	IRQ_init();
	__cpsie(i);   // Enables interrupts.
	TIMER_init();

	if(__getCpuId() == 0) // Core 0.
	{
		kernelInit(2);
		DMA330_init();
		PXI_init();

		// Load hardware calibration.
		if(fMount(FS_DRIVE_SDMC) == RES_OK)
		{
			HWCAL_load(); // TODO: Checks?
		}

		I2C_init();
		MCU_init();
		hidInit();
	}
	else // Any other core.
	{
		// TODO: When we enable core 2/3 support this needs to fixed.
		core1Standby();
	}
}

void __systemBootCore1(void (*entry)(void))
{
	// Set core 1 entrypoint.
	getScuRegs()->mn3 = (u32)entry;

	// Wake core 1 up.
	IRQ_softInterrupt(IRQ_IPI1, BIT(1));
}

void WEAK __systemDeinit(void)
{
	fUnmount(FS_DRIVE_SDMC); // TODO: Checks?

	PXI_deinit();
	DMA330_init();
	__cpsid(if);
	IRQ_init();
}