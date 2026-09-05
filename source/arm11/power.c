/*
 *   This file is part of open_agb_firm
 *   Copyright (C) 2021 derrek, profi200
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
#include "util.h"
#include "system.h"
#include "ipc_handler.h"
#include "arm11/drivers/interrupt.h"
#include "arm11/drivers/mcu.h"
#include "arm11/drivers/pdn.h"
#include "arm11/drivers/scu.h"
#include "arm11/drivers/timer.h"
#include "drivers/cache.h"
#include "drivers/pxi.h"
#include "arm.h"



static vu32 g_powerSleepCore1Ready = 0u;
static vu32 g_powerSleepCore1Release = 0u;
static vu32 g_powerSleepCore1Resumed = 0u;



static void power_sleep_core1(void)
{
	Scu *const scu = getScuRegs();

	/* core1Standby() unregisters IPI1 before dispatching this function. */
	IRQ_registerIsr(IRQ_IPI1, 14, 0, (IrqIsr)NULL);

	g_powerSleepCore1Ready = 1u;
	__dsb();

	u32 cpuStat = scu->cpu_stat;
	cpuStat &= ~SCU_STAT_MASK(1u);
	cpuStat |= SCU_STAT_DORMANT(1u);
	scu->cpu_stat = cpuStat;
	__dsb();

	/* PDN wake resumes this execution context. */
	__wfi();

	cpuStat = scu->cpu_stat;
	cpuStat &= ~SCU_STAT_MASK(1u);
	cpuStat |= SCU_STAT_NORMAL(1u);
	scu->cpu_stat = cpuStat;
	__dsb();

	g_powerSleepCore1Resumed = 1u;
	__dsb();

	while(g_powerSleepCore1Release == 0u)
		__asm__ volatile("nop");

	IRQ_unregisterIsr(IRQ_IPI1);
	/* Return to libn3ds core1Standby(). */
}

static void power_safe_halt(void)
{
	PXI_sendCmd(IPC_CMD9_PREPARE_POWER, NULL, 0);

	cleanDCache();
}

bool power_sleep(void)
{
	Pdn *const pdn = getPdnRegs();
	Scu *const scu = getScuRegs();

	/* GFX_sleep() must have powered VRAM down before whole-system sleep. */
	if((pdn->cnt & PDN_CNT_VRAM_OFF) == 0u)
		return false;

	const u32 wakeEnableBefore = pdn->wake_enable;

	/* Clear deferred MCU/PDN events before arming the system wake path. */
	(void)MCU_getIrqs(0xFFFFFFFFu);
	const u32 staleReason = pdn->wake_reason;
	if(staleReason != 0u)
		pdn->wake_reason = staleReason;
	pdn->wake_enable = PDN_WAKE_SHELL_OPENED | PDN_WAKE_MCU;

	/* Tell the MCU that the application processor is entering sleep. */
	if(!MCU_writeReg(MCU_REG_SYS_PWR, BIT(4)))
	{
		pdn->wake_enable = wakeEnableBefore;
		return false;
	}

	g_powerSleepCore1Ready = 0u;
	g_powerSleepCore1Release = 0u;
	g_powerSleepCore1Resumed = 0u;
	__dsb();

	/* Core 1 is idle here and can enter dormant state in parallel with Core 0. */
	__systemBootCore1(power_sleep_core1);
	while(g_powerSleepCore1Ready == 0u)
		__asm__ volatile("nop");
	while((scu->cpu_stat & SCU_STAT_MASK(1u)) != SCU_STAT_DORMANT(1u))
		__asm__ volatile("nop");

	u32 cpuStat = scu->cpu_stat;
	cpuStat &= ~SCU_STAT_MASK(0u);
	cpuStat |= SCU_STAT_DORMANT(0u);
	scu->cpu_stat = cpuStat;
	__dsb();

	/* Preserve the hardware VRAM-off status while asserting only SLEEP. */
	pdn->cnt = (u16)((pdn->cnt & PDN_CNT_VRAM_OFF) | PDN_CNT_SLEEP);
	__dsb();
	__isb();
	__wfi();

	/* Core 0 resumed in the same context. Rejoin coherency first. */
	cpuStat = scu->cpu_stat;
	cpuStat &= ~SCU_STAT_MASK(0u);
	cpuStat |= SCU_STAT_NORMAL(0u);
	scu->cpu_stat = cpuStat;
	__dsb();

	/* Release Core 1 whether PDN already woke it or it is still in WFI. */
	g_powerSleepCore1Release = 1u;
	__dsb();
	IRQ_softInterrupt(IRQ_IPI1, BIT(1));
	while(g_powerSleepCore1Resumed == 0u)
		__asm__ volatile("nop");

	const u32 wakeReason = pdn->wake_reason;
	if(wakeReason != 0u)
		pdn->wake_reason = wakeReason;
	pdn->wake_enable = wakeEnableBefore;

	return true;
}

[[noreturn]] void power_off(void)
{
	power_safe_halt();

	MCU_sysPowerOff();

	__cpsid(aif);
	while(1) __wfi();
}

[[noreturn]] void power_reboot(void)
{
	power_safe_halt();

	MCU_sysReboot();

	__cpsid(aif);
	while(1) __wfi();
}
