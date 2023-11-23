#include "types.h"
#include "arm11/drivers/lgyfb.h"
#include "arm11/drivers/interrupt.h"
#include "drivers/cache.h"
#include "drivers/corelink_dma-330.h"
#include "arm11/drivers/gx.h"
#include "kevent.h"


static KHandle g_frameReadyEvent = 0;

// DMA330 docs don't tell you the recommended alignment so we assume it's bus width.
alignas(8) static u8 g_gbaFrameDmaProg[42] =
{
	// All transfer settings for RGB8 at 360x240 to a 512x512 texture.
	0xBC, 0x01, 0xE6, 0xC2, 0xB9, 0x00, // MOV CCR, SB15 SS64 SAF SP2 DB15 DS64 DAI DP2
	0xBC, 0x00, 0x00, 0x10, 0x31, 0x10, // MOV SAR, 0x10311000
	0xBC, 0x02, 0x00, 0x00, 0x20, 0x18, // MOV DAR, 0x18200000
	0x35, 0x70,                         // FLUSHP 14
	0x20, 0x1D,                         // LP 30
	0x32, 0x70,                         // WFP 14, burst
	0x22, 0x46,                         // LP 71
	0x04,                               // LD
	0x08,                               // ST
	0x3C, 0x02,                         // LPEND
	0x27, 0x70,                         // LDPB 14
	0x08,                               // ST
	0x56, 0x40, 0x0E,                   // ADDH DAR, 0xE40
	0x38, 0x0E,                         // LPEND
	0x13,                               // WMB
	0x34, 0x00,                         // SEV 0
	0x00                                // END
};



static void gbaDmaIrqHandler(UNUSED u32 intSource)
{
	DMA330_ackIrq(0);
	DMA330_run(0, g_gbaFrameDmaProg);

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

	signalEvent(g_frameReadyEvent, false);
}

static void patchDmaProg(const bool is240x160)
{
	if(is240x160)
	{
		u8 *const prog = g_gbaFrameDmaProg;

		// Adjust bursts. Needs to be 16 transfers for 240x160.
		prog[2] = 0xF6;
		prog[4] = 0xBD;

		// Adjust outer loop count.
		prog[21] = 20 - 1;

		// Adjust inner loop count.
		prog[25] = 44 - 1;

		// Adjust gap skip.
		*((u16*)&prog[34]) = 0x1980;

		// Make sure the DMA controller can see the code.
		flushDCacheRange(prog, sizeof(g_gbaFrameDmaProg));
	}
	// Else 360x240. Nothing to do.
}

/*
 * Scale matrix limitations:
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
static void setScaleMatrix(LgyFbScaler *const scaler, const u32 len, const u32 patt,
                           const s16 *in, const u8 inBits, const u8 outBits)
{
	scaler->len  = len - 1;
	scaler->patt = patt;

	// Calculate the maximum values for input and output sub pixels.
	s32 inMax = (0xFF00u>>inBits) & 0xFFu; // Input bits in upper part.
	inMax = (inMax == 0 ? 1 : inMax);
	s32 outMax = (1u<<outBits) - 1;
	outMax = (outMax == 0 ? 1 : outMax);

	vu32 *out = &scaler->matrix[0][0];
	const vu32 *const outEnd = out + 6 * 8;
	do
	{
		// Correct the color range using the scale matrix hardware.
		// For example when converting RGB555 to RGB8 LgyFb lazily shifts the 5 bits up
		// so 0b00011111 becomes 0b11111000. But for maximum pixel brightness on the
		// input we also want the maximum on the output (0b11111000 --> 0b11111111).
		// This will fix it and distribute the colors evenly across the output range.
		// Also round up because the hardware will ignore the first 4 bits.
		const s32 mEntry = *in++;
		*out++ = mEntry * outMax / inMax + 8;
	} while(out < outEnd);
}

KHandle LGYFB_init(/*const bool isTop,*/ const ScalerCfg *const cfg)
{
	// TODO: Support TWL sized frames and using both LgyFb engines.
	const bool is240x160 = cfg->w == 240 && cfg->h == 160;

	patchDmaProg(is240x160);
	if(DMA330_run(0, g_gbaFrameDmaProg)) return 0;

	// Create KEvent for frame ready signal.
	KHandle frameReadyEvent = createEvent(false);
	g_frameReadyEvent = frameReadyEvent;

	LgyFb *const lgyFb = getLgyFbRegs(true);
	lgyFb->size  = LGYFB_SIZE(cfg->w, cfg->h);
	lgyFb->stat  = LGYFB_IRQ_MASK;
	lgyFb->irq   = 0;
	lgyFb->alpha = 0xFF;

	// TODO: Can't we just always do color corrections on h? Output differs between the 2 when both are active.
	if(is240x160)
	{
		setScaleMatrix(&lgyFb->h, cfg->hLen, cfg->hPatt, cfg->hMatrix, 5, 8);
	}
	else
	{
		setScaleMatrix(&lgyFb->v, cfg->vLen, cfg->vPatt, cfg->vMatrix, 5, 8);
		setScaleMatrix(&lgyFb->h, cfg->hLen, cfg->hPatt, cfg->hMatrix, 0, 0);
	}

	// With RGB8 output solid red and blue are converted to 0xF8 and green to 0xFA.
	// The green bias exists on the whole range of green colors.
	// Some results:
	// RGBA8:   Same as RGB8 but with useless alpha component.
	// RGB8:    Observed best format. Invisible dithering and best color accuracy.
	// RGB565:  A little visible dithering. Good color accuracy.
	// RGB5551: Lots of visible dithering. Good color accuracy (a little worse than 565).
	u32 cnt = LGYFB_DMA_EN | LGYFB_OUT_SWIZZLE | LGYFB_OUT_FMT_8880 | LGYFB_HSCALE_EN | LGYFB_EN;
	if(!is240x160) cnt |= LGYFB_VSCALE_EN;
	lgyFb->cnt = cnt;

	IRQ_registerIsr(IRQ_CDMA_EVENT0, 13, 0, gbaDmaIrqHandler);

	return frameReadyEvent;
}

void LGYFB_deinit(/*const bool isTop*/ void)
{
	// Disable LgyFb engine, acknowledge IRQs (if any), kill DMA channel and flush the FIFO.
	LgyFb *const lgyFb = getLgyFbRegs(true);
	lgyFb->cnt = 0;
	// Acknowledge IRQs here. Nothing to do since none are enabled.
	DMA330_kill(0);
	lgyFb->flush = 0;

	// Unregister isr and delete event (if any was created).
	IRQ_unregisterIsr(IRQ_CDMA_EVENT0);
	if(g_frameReadyEvent != 0)
		deleteEvent(g_frameReadyEvent);
	g_frameReadyEvent = 0;
}

void LGYFB_stop(/*const bool isTop*/ void)
{
	// Disable LgyFb engine, acknowledge IRQs (if any), kill DMA channel and flush the FIFO.
	LgyFb *const lgyFb = getLgyFbRegs(true);
	lgyFb->cnt &= ~LGYFB_EN;
	// Acknowledge IRQs here. Nothing to do since none are enabled.
	DMA330_kill(0);
	lgyFb->flush = 0;

	// Clear event to prevent issues since we just disabled LgyFb.
	clearEvent(g_frameReadyEvent);
}

void LGYFB_start(/*const bool isTop*/ void)
{
	// Restart DMA and then LgyFb.
	if(DMA330_run(0, g_gbaFrameDmaProg)) return;
	getLgyFbRegs(true)->cnt |= LGYFB_EN;
}
