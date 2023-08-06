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

#include <assert.h>
#include <string.h>
#include "types.h"
#include "arm11/drivers/lgy11.h"
#include "drivers/lgy_common.h"
#include "drivers/pxi.h"
#include "ipc_handler.h"
#include "util.h"
#include "arm11/drivers/hid.h"
#include "arm11/drivers/interrupt.h"
#include "drivers/cache.h"
#include "arm11/drivers/pdn.h"
#include "arm11/drivers/mcu.h"



static void lgySleepIsr(u32 intSource)
{
	Lgy11 *const lgy11 = getLgy11Regs();
	if(intSource == IRQ_LGY_SLEEP)
	{
		// Workaround for The Legend of Zelda - A Link to the Past.
		// This game doesn't set the IRQ enable bit so we force it
		// on the 3DS side. Unknown if other games have this bug.
		REG_HID_PADCNT = lgy11->padcnt | 1u<<14;
	}
	else // IRQ_HID_PADCNT
	{
		// TODO: Synchronize with LCD VBlank.
		REG_HID_PADCNT = 0;
		lgy11->sleep |= 1u; // Acknowledge and wakeup.
	}
}

static void powerDownFcramForLegacy(u8 mode)
{
	flushDCache();
	// TODO: Unmap FCRAM.
	const Lgy11 *const lgy11 = getLgy11Regs();
	while(!lgy11->mode); // Wait until legacy mode is ready.

	// For GBA mode we need to additionally apply a bug fix and reset FCRAM.
	Pdn *const pdn = getPdnRegs();
	if(mode == LGY_MODE_AGB)
	{
		*((vu32*)0x10201000) &= ~1u;                  // Bug fix for the GBA cart emu?
		pdn->fcram_cnt = PDN_FCRAM_CNT_CLK_EN;        // Set reset low (active) but keep clock on.
	}
	pdn->fcram_cnt = PDN_FCRAM_CNT_NORST;             // Take it out of reset but disable clock.
	while(pdn->fcram_cnt & PDN_FCRAM_CNT_CLK_EN_ACK); // Wait until clock is disabled.
}

// https://en.wikipedia.org/wiki/Determination_of_the_day_of_the_week#Sakamoto's_methods
static void calcDayOfWeek(GbaRtc *const rtc)
{
	u32 y = bcd2dec(rtc->y) + 2000u;
	u8 m = bcd2dec(rtc->mon);
	u8 d = bcd2dec(rtc->d);

	static const u8 t[12] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
	if(m < 3) y--;
	rtc->dow = (y + (y / 4u) - (y / 100u) + (y / 400u) + t[m - 1] + d) % 7u;
}

// Extra safety check just in case.
static_assert(sizeof(GbaRtc) >= sizeof(RtcTimeDate), "GbaRtc is not bigger or same size as RtcTimeDate!");
static void mcuTimeDateToGbaRtc(GbaRtc *const rtc)
{
	// Lazy conversion.
	MCU_getRtcTimeDate((RtcTimeDate*)rtc);
	rtc->time = __builtin_bswap32(rtc->time)>>8;
	rtc->date = __builtin_bswap32(rtc->date)>>8;
	calcDayOfWeek(rtc);
}

Result LGY_prepareGbaMode(bool directBoot, u16 saveType, const char *const savePath)
{
	u32 cmdBuf[4];
	cmdBuf[0] = (u32)savePath;
	cmdBuf[1] = strlen(savePath) + 1;
	cmdBuf[2] = directBoot;
	cmdBuf[3] = saveType;
	Result res = PXI_sendCmd(IPC_CMD9_PREPARE_GBA, cmdBuf, 4);
	if(res != RES_OK) return res;

	// Setup GBA Real-Time Clock.
	GbaRtc rtc;
	mcuTimeDateToGbaRtc(&rtc);
	LGY_setGbaRtc(rtc);

	// Setup FCRAM for GBA mode.
	powerDownFcramForLegacy(LGY_MODE_AGB);

	// Setup IRQ handlers and sleep mode handling.
	getLgy11Regs()->sleep = 1u<<15;
	IRQ_registerIsr(IRQ_LGY_SLEEP, 14, 0, lgySleepIsr);
	IRQ_registerIsr(IRQ_HID_PADCNT, 14, 0, lgySleepIsr);

	return RES_OK;
}

Result LGY_setGbaRtc(const GbaRtc rtc)
{
	return PXI_sendCmd(IPC_CMD9_SET_GBA_RTC, (u32*)&rtc, sizeof(GbaRtc) / 4);
}

Result LGY_getGbaRtc(GbaRtc *const out)
{
	const u32 cmdBuf[2] = {(u32)out, sizeof(GbaRtc)};
	return PXI_sendCmd(IPC_CMD9_GET_GBA_RTC, cmdBuf, 2);
}

Result LGY_backupGbaSave(void)
{
	return PXI_sendCmd(IPC_CMD9_BACKUP_GBA_SAVE, NULL, 0);
}

void LGY11_switchMode(void)
{
	getLgy11Regs()->mode = LGY_MODE_START;
}

void LGY11_deinit(void)
{
	LGY_backupGbaSave();

	IRQ_unregisterIsr(IRQ_LGY_SLEEP);
	IRQ_unregisterIsr(IRQ_HID_PADCNT);
}
