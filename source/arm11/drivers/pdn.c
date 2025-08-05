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
#include "arm11/drivers/pdn.h"
#include "arm11/drivers/cfg11.h"
#include "arm.h"
#include "arm11/drivers/gic.h"
#include "arm11/drivers/interrupt.h"
#include "arm11/start.h"
#include "util.h"
#include "arm11/drivers/scu.h"
#include "kevent.h"
#include "arm11/drivers/gx.h"


// TODO: Before nabling this fix system.c to support core 2/3.
//#define CORE123_INIT (1)



#ifdef CORE123_INIT
static void NAKED core23Entry(void)
{
	__cpsid(aif);
	Gicc *const gicc = getGiccRegs();
	gicc->ctrl = 1;

	// Tell core 0 we are here.
	const u32 cpuId = __getCpuId();
	Pdn *const pdn = getPdnRegs();
	if(cpuId == 3) pdn->lgr_cpu_cnt[3] = LGR_CPU_CNT_NORST;
	else           pdn->lgr_cpu_cnt[2] = LGR_CPU_CNT_NORST;

	// Wait for IPI 2 (core 2) or IPI 3 (core 3).
	u32 tmp;
	do
	{
		__wfi();
		tmp = gicc->intack;
		gicc->eoi = tmp;
	} while(tmp != cpuId);

	// Jump to real entrypoint.
	_start();
}
#endif

// This must be called before the MMU is enabled on any core or it will hang!
void PDN_core123Init(void)
{
	Cfg11 *const cfg11 = getCfg11Regs();
	Gicd *const gicd = getGicdRegs();
	if(cfg11->socinfo & SOCINFO_LGR1)
	{
		getGiccRegs()->ctrl = BIT(0);
		for(u32 i = 0; i < 4; i++) gicd->enable_clear[i] = 0xFFFFFFFFu; // Disable all interrupts.
		gicd->pending_clear[2] = BIT(24); // Clear interrupt ID 88.
		gicd->pri[22] = 0;                // Id 88 highest priority.
		gicd->target[22] = BIT(0);        // Id 88 target core 0.
		gicd->enable_set[2] = BIT(24);    // Enable interrupt ID 88.

		// Certain bootloaders leave the ack bit set. Clear it.
		Pdn *const pdn = getPdnRegs();
		pdn->lgr_socmode = pdn->lgr_socmode;

#ifdef CORE123_INIT
		// Use 804 MHz for LGR2 and 536 for LGR1.
		u16 socmode;
		if(cfg11->socinfo & SOCINFO_LGR2) socmode = SOCMODE_LGR2_804MHZ;
		else                              socmode = SOCMODE_LGR1_536MHZ;

		if((pdn->lgr_socmode & SOCMODE_MASK) != socmode)
		{
			// Enable L2 cache and/or extra WRAM.
			if(cfg11->socinfo & SOCINFO_LGR2) pdn->lgr_cnt = PDN_LGR_CNT_L2C_EN | PDN_LGR_CNT_WRAM_EXT_EN;
			else                              pdn->lgr_cnt = PDN_LGR_CNT_WRAM_EXT_EN;

			// Necessary delay.
			wait_cycles(403);

			PDN_setSocmode(socmode);
			gicd->pending_clear[2] = BIT(24); // Clear interrupt ID 88.

			// Fixes for the GPU to work in non-CTR mode.
			cfg11->gpu_n3ds_cnt = GPU_N3DS_CNT_TEX_FIX | GPU_N3DS_CNT_N3DS_MODE;
		}

		cfg11->cdma_peripherals = CDMA_PERIPHERALS_ALL; // Redirect all to CDMA2.

		Scu *const scu = getScuRegs();
		if((scu->config & SCU_CPU_NUM_MASK) == SCU_CPU_NUM_4)
		{
			// Set core 2/3 to normal mode (running) in the SCU.
			scu->cpu_stat = (scu->cpu_stat & ~(SCU_STAT_MASK(3) | SCU_STAT_MASK(2))) |
			                SCU_STAT_NORMAL(3) | SCU_STAT_NORMAL(2);

			// Temporarily switch to 268 MHz for core 2/3 bringup.
			u16 tmpSocmode;
			if(cfg11->socinfo & SOCINFO_LGR2) tmpSocmode = SOCMODE_LGR2_268MHZ;
			else                              tmpSocmode = SOCMODE_LGR1_268MHZ;

			if(socmode != tmpSocmode)
			{
				PDN_setSocmode(tmpSocmode);
				gicd->pending_clear[2] = BIT(24); // Clear interrupt ID 88.
			}

			cfg11->bootrom_overlay_cnt = BOOTROM_OVERLAY_CNT_EN;
			cfg11->bootrom_overlay_val = (u32)core23Entry;
			// If not already done enable instruction and data overlays.
			if(!(pdn->lgr_cpu_cnt[2] & LGR_CPU_CNT_RST_STAT))
			{
				pdn->lgr_cpu_cnt[2] = LGR_CPU_CNT_D_OVERL_EN | LGR_CPU_CNT_NORST;
			}
			if(!(pdn->lgr_cpu_cnt[3] & LGR_CPU_CNT_RST_STAT))
			{
				pdn->lgr_cpu_cnt[3] = LGR_CPU_CNT_D_OVERL_EN | LGR_CPU_CNT_NORST;
			}
			// Wait for core 2/3 to jump out of boot11.
			while((pdn->lgr_cpu_cnt[2] & (LGR_CPU_CNT_RST_STAT | LGR_CPU_CNT_D_OVERL_EN))
			      != LGR_CPU_CNT_RST_STAT);
			while((pdn->lgr_cpu_cnt[3] & (LGR_CPU_CNT_RST_STAT | LGR_CPU_CNT_D_OVERL_EN))
			      != LGR_CPU_CNT_RST_STAT);
			cfg11->bootrom_overlay_cnt = 0; // Disable all overlays.

			// Switch back to highest clock.
			if(socmode != tmpSocmode) PDN_setSocmode(socmode);
		}

		gicd->enable_clear[2] = BIT(24); // Clear interrupt ID 88.

		// Wakeup core 2/3 and let them jump to their entrypoint.
		IRQ_softInterrupt(2, BIT(2));
		IRQ_softInterrupt(3, BIT(3));
#else
		// Make sure we are in CTR mode (if not already).
		if((pdn->lgr_socmode & SOCMODE_MASK) != SOCMODE_CTR_268MHZ)
		{
			PDN_setSocmode(SOCMODE_CTR_268MHZ);
			gicd->enable_clear[2] = BIT(24); // Clear interrupt ID 88.
		}
#endif
	}

	// Wakeup core 1
	*((vu32*)0x1FFFFFDC) = (u32)_start;  // Core 1 entrypoint.
	IRQ_softInterrupt(1, BIT(1));
}

void PDN_setSocmode(PdnSocmode socmode)
{
	Pdn *const pdn = getPdnRegs();

	pdn->lgr_socmode = socmode;

	do
	{
		__wfi();
	} while(!(pdn->lgr_socmode & PDN_LGR_SOCMODE_ACK));

	pdn->lgr_socmode = pdn->lgr_socmode; // Acknowledge (PDN_LGR_SOCMODE_ACK bit set).
}

void PDN_poweroffCore23(void)
{
	Cfg11 *const cfg11 = getCfg11Regs();
	if(cfg11->socinfo & SOCINFO_LGR1)
	{
		Pdn *const pdn = getPdnRegs();
		pdn->lgr_cpu_cnt[2] = 0;
		pdn->lgr_cpu_cnt[3] = 0;

		cfg11->cdma_peripherals = 0;
		cfg11->gpu_n3ds_cnt = 0;

		pdn->lgr_cnt = 0;
		if(cfg11->socinfo & SOCINFO_LGR2) PDN_setSocmode(SOCMODE_LGR2_268MHZ);
		else                              PDN_setSocmode(SOCMODE_LGR1_268MHZ);

		Scu *const scu = getScuRegs();
		scu->cpu_stat = (scu->cpu_stat & ~(SCU_STAT_MASK(3) | SCU_STAT_MASK(2))) |
		                SCU_STAT_PWROFF(3) | SCU_STAT_PWROFF(2);

		PDN_setSocmode(SOCMODE_CTR_268MHZ);
	}
}

// TODO: gcc generates very odd code for this function.
void PDN_controlGpu(const bool enableClk, const bool resetPsc, const bool resetOther)
{
	u32 reg = (enableClk ? PDN_GPU_CNT_CLK_EN : 0);
	reg |= (resetPsc ? 0 : PDN_GPU_CNT_NORST_REGS);
	reg |= (resetOther ? 0 : (PDN_GPU_CNT_NORST_ALL & ~PDN_GPU_CNT_NORST_REGS));

	Pdn *const pdn = getPdnRegs();
	pdn->gpu_cnt = reg;
	if(resetPsc || resetOther)
	{
		wait_cycles(12); // 4x the needed cycles in case we are in LGR2 mode.
		pdn->gpu_cnt = reg | PDN_GPU_CNT_NORST_ALL;
	}
}

static void pdn_isr(UNUSED u32 intSource) 
{
	getPdnRegs()->wake_enable = 0;
	getPdnRegs()->wake_reason = PDN_WAKE_SHELL_OPENED;
}

void PDN_sleep(void)
{
	IRQ_registerIsr(IRQ_PDN, 14, 0, pdn_isr);
	getPdnRegs()->wake_enable = PDN_WAKE_SHELL_OPENED;

	if (getPdnRegs()->cnt & PDN_CNT_VRAM_OFF)
	{
		// Disable VRAM banks. This is needed for PDN sleep mode.
		GxRegs *const gx = getGxRegs();
		gx->psc_vram |= PSC_VRAM_BANK_DIS_ALL;
	}

	getPdnRegs()->cnt |= PDN_CNT_SLEEP;

	// turning off Gpu needs to be done after sleeping
	PDN_controlGpu(false, false, false);
	__wfi();
}

void PDN_wakeup(void)
{
	PDN_controlGpu(true, true, false);
	GxRegs *const gx = getGxRegs();
	gx->psc_vram &= ~PSC_VRAM_BANK_DIS_ALL;

}