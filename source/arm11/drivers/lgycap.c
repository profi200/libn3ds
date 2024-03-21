/*
 *   This file is part of open_agb_firm
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

#include <string.h>
#include "types.h"
#include "arm11/drivers/lgycap.h"
#include "arm11/drivers/interrupt.h"
#include "drivers/cache.h"
#include "drivers/corelink_dma-330.h"
#include "arm11/drivers/gx.h"
#include "kevent.h"


static KHandle g_frameReadyEvents[2] = {0, 0};

// DMA330 docs don't tell you the recommended alignment so we assume it's bus width.
alignas(8) static u8 g_lgyCapDmaProg[44] =
{
	// All transfer settings for A1BGR5 at 360x240 to a 512x512 texture.
	0xBC, 0x01, 0xF6, 0xC2, 0xBD, 0x00, // MOV CCR, SB16 SS64 SAF SP2 DB16 DS64 DAI DP2
	0xBC, 0x00, 0x00, 0x10, 0x31, 0x10, // MOV SAR, 0x10311000
	0x35, 0x70,                         // FLUSHP 14
	0x20, 0x01,                         // LP 2
	0xBC, 0x02, 0x00, 0x00, 0x20, 0x18, // MOV DAR, 0x18200000
	                                    // LPFE
	0x31, 0x70,                         // WFP 14, periph
	0x22, 0x2B,                         // LP 44
	0x04,                               // LD
	0x08,                               // ST
	0x3C, 0x02,                         // LPEND
	0x27, 0x70,                         // LDPB 14
	0x08,                               // ST
	0x56, 0x80, 0x09,                   // ADDH DAR, 0x980
	0x28, 0x0E,                         // LPEND
	0x13,                               // WMB
	0x34, 0x08,                         // SEV 1
	0x38, 0x1B,                         // LPEND ; Modified to jump to "LP 2" so the loop can never exit.
	0x00                                // END
};



static void dmaIrqHandler(const u32 intSource)
{
	// Acknowledge the IRQ from CDMA.
	static_assert(IRQ_CDMA_EVENT0 + 1 == IRQ_CDMA_EVENT1);
	const u32 dev = intSource - IRQ_CDMA_EVENT0;
	DMA330_ackIrq(dev);

	// We can't match the GBA refreshrate exactly so keep the LCDs around 90%
	// ahead of the GBA output which gives us a time window of around 1.6 ms to
	// render the frame and hopefully reduces output lag as much as possible.
	Pdc *const pdc0 = &getGxRegs()->pdc0;
	u32 vtotal;
	if(pdc0->v_count > 414 - 41) vtotal = 415; // Slower than GBA.
	else                         vtotal = 414; // Faster than GBA.
	pdc0->v_total = vtotal;
	// Wide mode:
	/*u32 vtotal;
	if(pdc0->v_count > 828 - 82) vtotal = 830; // Slower than GBA.
	else                         vtotal = 829; // Faster than GBA.
	pdc0->v_total = vtotal;*/

	signalEvent(g_frameReadyEvents[dev], false);
}

// This is a temporary solution and will be replaced by a DMA JIT eventually.
static bool patchDmaProg(const u16 width, const u32 pixelSize)
{
	u8 *const prog = g_lgyCapDmaProg;

	// Test if we can divide the size of 8 lines by DMA burst size.
	const u32 bytesPer8Lines = width * pixelSize * 8;
	u32 transfers = 16;
	if(bytesPer8Lines % (16 * 8) == 0)
	{
		// Patch DMA prog to do 16 transfers of 8 bytes each.
		prog[2] = 0xF6; // SB16.
		prog[4] = 0xBD; // DB16.
	}
	else if(bytesPer8Lines % (15 * 8) == 0)
	{
		// Patch DMA prog to do 15 transfers of 8 bytes each.
		transfers = 15;
		prog[2] = 0xE6; // SB15.
		prog[4] = 0xB9; // DB15.
	}
	else return false;

	// Adjust inner loop count. The last transfer is not part of the loop.
	prog[25] = (bytesPer8Lines / (transfers * 8)) - 2;

	// Skip padding pixels at the right side of the sub texture.
	const u16 gapSize = (512u - width) * pixelSize * 8;
	memcpy(&prog[34], &gapSize, 2);

	// Make sure the DMA controller can see the code.
	flushDCacheRange(prog, sizeof(g_lgyCapDmaProg));

	return true;
}

/*
 * Scaler matrix limitations:
 * First pattern bit must be 1 and last 0 (for V-scale) or it loses sync with the DS/GBA input.
 * Vertical scaling is fucked with identity matrix.
 *
 * Matrix ranges:
 * in[-3] -1024-1023 (0xFC00-0x03FF)
 * in[-2] -4096-4095 (0xF000-0x0FFF)
 * in[-1] -32768-32767 (0x8000-0x7FFF)
 * in[0]  -32768-32767 (0x8000-0x7FFF)
 * in[1]  -4096-4095 (0xF000-0x0FFF)
 * in[2]  -1024-1023 (0xFC00-0x03FF)
 *
 * Note: At scanline start the in FIFO is all filled with the first pixel.
 * Note: The first column only allows 1 non-zero entry.
 * Note: Bits 0-3 of each entry are ignored by the hardware.
 * Note: 16384 (0x4000) is the maximum brightness of a pixel.
 *       The sum of all entries in a column should be 16384 or clipping will occur.
 * Note: The window of (the 6) input pixels is post-increment.
 *       When the matching pattern bit is 0 it does not move forward.
 */
static void setScalerMatrix(LgyCapScaler *const scaler, const u32 len, const u32 patt, const s16 *in)
{
	scaler->len  = len - 1;
	scaler->patt = patt;

	vu32 *out = &scaler->matrix[0][0];
	unsigned i = 0;
	do
	{
		*out++ = *in++;
	} while(++i < 6 * 8);
}

static inline u32 getPixelSize(const u32 lgyfb_cnt)
{
	const u32 fmt = (lgyfb_cnt>>8) & 3u;
	return (fmt == (LGYCAP_OUT_FMT_BGR565>>8) ? 2 : 4 - fmt);
}

KHandle LGYCAP_init(const LgyCapDev dev, const LgyCapCfg *const cfg)
{
	if(!patchDmaProg(cfg->w, getPixelSize(cfg->cnt))) return 0;
	if(DMA330_run(dev, g_lgyCapDmaProg)) return 0;

	// Create KEvent for frame ready signal.
	KHandle frameReadyEvent = createEvent(false);
	g_frameReadyEvents[dev] = frameReadyEvent;

	LgyCap *const lgyCap = getLgyCapRegs(dev);
	lgyCap->dim   = LGYCAP_DIM(cfg->w, cfg->h);
	lgyCap->stat  = LGYCAP_IRQ_MASK; // Acknowledge all IRQs.
	lgyCap->irq   = 0;
	lgyCap->alpha = 0xFF;

	// Hardware bug:
	// The dither pattern *must* be initialized.
	// The dither enable bits in REG_LGYCAP_CNT are broken and do nothing.
	// 0xCCCC will force the hardware to apply no dithering.
	lgyCap->dither_patt0 = 0xCCCC;
	lgyCap->dither_patt1 = 0xCCCC;
	lgyCap->dither_patt2 = 0xCCCC;
	lgyCap->dither_patt3 = 0xCCCC;

	// Setup hardware scaling.
	setScalerMatrix(&lgyCap->vscaler, cfg->vLen, cfg->vPatt, cfg->vMatrix);
	setScalerMatrix(&lgyCap->hscaler, cfg->hLen, cfg->hPatt, cfg->hMatrix);

	// Color accuracy notes:
	// In GBA mode with BGR8 output format color channels are converted with "val<<3".
	// The green channel is special and uses "(val<<3) + 2" instead.
	// DS mode is untested but likely "val<<2" (green offset unknown).
	// A1BGR5 is ideal for GBA mode since we get the 5 bits per channel we want directly.
	lgyCap->cnt = LGYCAP_DMA_EN | cfg->cnt | LGYCAP_EN;

	IRQ_registerIsr(IRQ_CDMA_EVENT0 + dev, 13, 0, dmaIrqHandler);

	return frameReadyEvent;
}

void LGYCAP_deinit(const LgyCapDev dev)
{
	// Disable LgyCap engine.
	LgyCap *const lgyCap = getLgyCapRegs(dev);
	lgyCap->cnt = 0;

	// Acknowledge all IRQs.
	//lgyCap->stat  = LGYCAP_IRQ_MASK;

	// Kill the DMA channel and flush the FIFO.
	DMA330_kill(dev);
	lgyCap->flush = 0;

	// Unregister interrupt service routine.
	IRQ_unregisterIsr(IRQ_CDMA_EVENT0 + dev);

	// Delete event if one was created previously.
	if(g_frameReadyEvents[dev] != 0)
	{
		deleteEvent(g_frameReadyEvents[dev]);
	}
	g_frameReadyEvents[dev] = 0;
}

void LGYCAP_stop(const LgyCapDev dev)
{
	// Disable LgyCap engine.
	LgyCap *const lgyCap = getLgyCapRegs(dev);
	lgyCap->cnt &= ~LGYCAP_EN;

	// Acknowledge all IRQs.
	//lgyCap->stat  = LGYCAP_IRQ_MASK;

	// Kill the DMA channel and flush the FIFO.
	DMA330_kill(dev);
	lgyCap->flush = 0;

	// Clear event to prevent issues since we just disabled LgyCap.
	if(g_frameReadyEvents[dev] != 0)
	{
		clearEvent(g_frameReadyEvents[dev]);
	}
}

void LGYCAP_start(const LgyCapDev dev)
{
	// Restart DMA followed by LgyCap.
	if(DMA330_run(dev, g_lgyCapDmaProg)) return;
	getLgyCapRegs(dev)->cnt |= LGYCAP_EN;
}