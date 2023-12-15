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
#include "drivers/corelink_dma-330.h"
#include "drivers/cache.h"


#ifdef __ARM11__
#ifdef USE_NEW_CDMA
#error "TODO: New3DS CDMA"
#else
#define CHANNELS   (8u)
#define PERIPHALS  (18u)
#define IRQ_LINES  (9u) // The controller reports 16 but we only have 9 physical lines.
#endif // ifdef USE_NEW_CDMA
#elif __ARM9__
#define CHANNELS   (4u)
#define PERIPHALS  (8u)
#define IRQ_LINES  (12u)
#endif // ifdef __ARM11__

#define INTEN_VAL  ((1u<<IRQ_LINES) - 1) // Not 32 bit safe!



static inline void waitChannelStatus(const vu32 *const channel_csr, u8 status)
{
	while((*channel_csr & CSR_STAT_MASK) != status);
}

static inline void waitDebugBusy(const Dma330 *const dma330)
{
	while(dma330->dbgstatus & DBGSTATUS_BUSY);
}

static void sendDebugCmd(Dma330 *const dma330, u32 inst0, u32 inst1)
{
	waitDebugBusy(dma330);
	dma330->dbginst0 = inst0;
	dma330->dbginst1 = inst1;
	dma330->dbgcmd = DBGCMD_EXECUTE;
}

void DMA330_init(void)
{
	static bool inited = false;
	if(inited) return;
	inited = true;

	Dma330 *const dma330 = getDma330Regs();
	// Kill manager thread.
	sendDebugCmd(dma330, DBGINST0(0x01u, 0, DBGINST0_THR_MGR), 0);

	// Kill all channels.
	for(u32 i = 0; i < CHANNELS; i++)
	{
		// DMAKILL channel.
		sendDebugCmd(dma330, DBGINST0(0x01u, i, DBGINST0_THR_CH), 0);
	}
	waitChannelStatus(&dma330->chStat[CHANNELS - 1].csr, CSR_STAT_STOPPED);

	dma330->inten  = INTEN_VAL;
	dma330->intclr = 0xFFFFFFFF; // Clear all interrupts.
	dma330->wd     = 0;          // Watchdog aborts hanging channels.

	if(PERIPHALS > 0)
	{
#ifdef __ARM11__
		u16 progBuf[33]; // Max 32 periphals + 1 for DMAEND.
#elif __ARM9__
		u16 *progBuf = (u16*)(AHB_RAM_BASE + AHB_RAM_SIZE - 33 * 2); // ARM9 DTCM stack workaround.
#endif // ifdef __ARM11__
		for(u32 i = 0; i < PERIPHALS; i++)
		{
			// DMAFLUSHP i.
			progBuf[i] = i<<11 | 0x35u;
		}
		progBuf[PERIPHALS] = 0; // DMAEND.
		cleanDCacheRange(progBuf, 33 * 2);

		// DMAGO channel 0 non-secure.
		sendDebugCmd(dma330, DBGINST0(0u<<8 | 0xA2u, 0, DBGINST0_THR_MGR), (u32)progBuf);
		waitChannelStatus(&dma330->chStat[0].csr, CSR_STAT_STOPPED); // Wait for IRQ instead?
	}
}

u8 DMA330_run(u8 ch, const u8 *const prog)
{
	Dma330 *const dma330 = getDma330Regs();

	u8 status;
	if((status = (dma330->chStat[ch].csr & CSR_STAT_MASK)) != CSR_STAT_STOPPED)
		return status;

	// DMAGO non-secure.
	sendDebugCmd(dma330, DBGINST0(ch<<8 | 0xA2u, 0, DBGINST0_THR_MGR), (u32)prog);

	return status;
}

u8 DMA330_status(u8 ch)
{
	return getDma330Regs()->chStat[ch].csr & CSR_STAT_MASK;
}

void DMA330_ackIrq(u8 eventIrq)
{
	getDma330Regs()->intclr = INTCLR_IRQ_CLR(eventIrq);
}

void DMA330_sev(u8 event)
{
	// DMASEV.
	sendDebugCmd(getDma330Regs(), DBGINST0(event<<11 | 0x34u, 0, DBGINST0_THR_MGR), 0);
}

void DMA330_kill(u8 ch)
{
	Dma330 *const dma330 = getDma330Regs();

	if((dma330->chStat[ch].csr & CSR_STAT_MASK) != CSR_STAT_STOPPED)
	{
		sendDebugCmd(dma330, DBGINST0(0x01u, ch, DBGINST0_THR_CH), 0);
		waitChannelStatus(&dma330->chStat[ch].csr, CSR_STAT_STOPPED);
	}
}

/*#ifdef __ARM11__
#include "arm11/fmt.h"
void DMA330_dbgPrint(void)
{
	Dma330 *const dma330 = getDma330Regs();

	ee_printf("DSR: %08lX FTRD: %08lX\n", dma330->dsr, dma330->ftrd);
	for(u32 i = 0; i < CHANNELS; i++)
	{
		ee_printf(" CSR/FTR%lu: %08lX %08lX\n", i, dma330->chStat[i].csr, dma330->ftr[i]);
	}
}
#endif*/ // ifdef __ARM11__
