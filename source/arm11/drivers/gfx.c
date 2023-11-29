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

#include <string.h>
#include "types.h"
#include "fb_assert.h"
#include "drivers/gfx.h"
#include "arm11/drivers/cfg11.h"
#include "arm11/drivers/pdn.h"
#include "arm11/drivers/gx.h"
#include "arm11/drivers/pdc_presets.h"
#include "arm11/drivers/lcd.h"
#include "arm11/drivers/gpu_regs.h"
#include "mem_map.h"
#include "mmio.h"
#include "arm11/drivers/i2c.h"
#include "arm11/drivers/mcu.h"
#include "arm11/debug.h"
#include "arm11/drivers/interrupt.h"
#include "arm11/drivers/timer.h"
#include "arm.h"
#include "util.h"
#include "arm11/allocator/vram.h"
#include "kevent.h"
#include "drivers/cache.h"


#ifndef LIBN3DS_LEGACY
#define GFX_PDC0_IRQS     (PDC_CNT_NO_IRQ_ERR | PDC_CNT_NO_IRQ_H)
#define GFX_PDC1_IRQS     (GFX_PDC0_IRQS)
#else
#define GFX_PDC0_IRQS     (PDC_CNT_NO_IRQ_ERR | PDC_CNT_NO_IRQ_H)
#define GFX_PDC1_IRQS     (PDC_CNT_NO_IRQ_ALL)
#endif // #ifndef LIBN3DS_LEGACY


typedef struct
{
	u8 *bufs[4];   // PDC frame buffer pointers in order: A0, B0, A1, B1.
	u32 fb_fmt;    // PDC frame buffer format.
	u32 fb_stride; // PDC frame buffer stride.
} LcdState;

typedef struct
{
	KHandle events[6]; // Eevents in order: PSC0, PSC1, PDC0, PDC1, PPF, P3D.
	u8 swapMask;       // Double buffering masks for top and bottom. Bit 0 top, 1 bottom.
	u8 swap;           // Currently active frame buffer. Bit 0 top, 1 bottom.
	u8 mcuLcdState;    // Note: We use the power off bits to track power on.
	//GfxTopMode mode;   // Current topscreen mode. TODO
	LcdState lcds[2];  // 0 top, 1 bottom.
	u32 lcdLum;        // Current LCD luminance for both LCDs.
} GfxState;

static GfxState g_gfxState = {0};



static void allocateFramebufs(const GfxFmt fmtTop, const GfxFmt fmtBot, const GfxTopMode mode)
{
	GfxState *const state = &g_gfxState;
	const u8 topPixelSize = GFX_getPixelSize(fmtTop);
	const u8 botPixelSize = GFX_getPixelSize(fmtBot);
	state->lcds[GFX_LCD_TOP].fb_stride = LCD_WIDTH_TOP * topPixelSize;
	state->lcds[GFX_LCD_BOT].fb_stride = LCD_WIDTH_BOT * botPixelSize;

	const u32 topSize = LCD_WIDTH_TOP * LCD_WIDE_HEIGHT_TOP * topPixelSize; // TODO: Smaller allocation in 2D mode (240x400)?
	const u32 botSize = LCD_WIDTH_BOT * LCD_HEIGHT_BOT * botPixelSize;

	// Frame buffer layout in memory unless the allocator puts them elsewhere:
	// [top A0 (3D left)] [top B0 (3D right)] [bot A0] [top A1 (3D left)] [top B1 (3D right)] [bot A1]
	// The left/right buffers are always allocated at once to allow easy 2D/3D mode switching.

	// First top left/right frame buffers.
	u8 *topBuf = vramAlloc(topSize);
	u8 *topRightBuf = topBuf + (topSize / 2);
	state->lcds[GFX_LCD_TOP].bufs[0] = topBuf;
	state->lcds[GFX_LCD_TOP].bufs[1] = topRightBuf;

	// First bottom frame buffer.
	u8 *botBuf = vramAlloc(botSize);
	state->lcds[GFX_LCD_BOT].bufs[0] = botBuf;
	state->lcds[GFX_LCD_BOT].bufs[1] = botBuf;

	// Second top left/right frame buffers.
	topBuf = vramAlloc(topSize);
	topRightBuf = topBuf + (topSize / 2);
	state->lcds[GFX_LCD_TOP].bufs[2] = topBuf;
	state->lcds[GFX_LCD_TOP].bufs[3] = topRightBuf;

	// Second bottom frame buffer.
	botBuf = vramAlloc(botSize);
	state->lcds[GFX_LCD_BOT].bufs[2] = botBuf;
	state->lcds[GFX_LCD_BOT].bufs[3] = botBuf;

	u32 outModeTop;
	switch(mode)
	{
		case GFX_TOP_2D:
			outModeTop = PDC_FB_DOUBLE_V | PDC_FB_OUT_A;
			break;
		case GFX_TOP_WIDE:
			outModeTop = PDC_FB_OUT_A;
			break;
		default: // 3D mode.
			outModeTop = PDC_FB_OUT_AB;
	}

	// TODO: For FCRAM buffers we need burst size 6/8?
	state->lcds[GFX_LCD_TOP].fb_fmt = PDC_FB_DMA_INT(8u) | PDC_FB_BURST_24_32 |
	                                  outModeTop | PDC_FB_FMT(fmtTop);
	state->lcds[GFX_LCD_BOT].fb_fmt = PDC_FB_DMA_INT(8u) | PDC_FB_BURST_24_32 |
	                                  PDC_FB_OUT_A | PDC_FB_FMT(fmtBot);
}

static void freeFramebufs(void)
{
	GfxState *const state = &g_gfxState;
	vramFree(state->lcds[GFX_LCD_BOT].bufs[2]);
	vramFree(state->lcds[GFX_LCD_TOP].bufs[2]);
	vramFree(state->lcds[GFX_LCD_BOT].bufs[0]);
	vramFree(state->lcds[GFX_LCD_TOP].bufs[0]);

	//memset(state->lcds[GFX_LCD_TOP].bufs, 0, sizeof(state->lcds[GFX_LCD_TOP].bufs));
	//memset(state->lcds[GFX_LCD_BOT].bufs, 0, sizeof(state->lcds[GFX_LCD_BOT].bufs));
}

static void hardwareReset(void)
{
	// Give the GPU access to all memory.
	// TODO: This should be configurable depending on what libn3ds is used for.
	getCfg11Regs()->gpuprot = GPUPROT_NO_PROT;

	// Reset all blocks including PSC, PDC, PPF and GPU.
	PDN_controlGpu(true, true, true);

	// Setup clock related stuff, stop and acknowledge PSC fill and set some priorities.
	GxRegs *const gx = getGxRegs();
	gx->gpu_clk  = 0x70100;                 // TwlBg seems to | 0x300 but bit 9 is never set??
	gx->psc_vram &= ~PSC_VRAM_BANK_DIS_ALL; // All VRAM banks enabled.
	gx->psc_fill0.cnt = 0;                  // Stop PSC fill engine and clear its IRQ bit.
	gx->psc_fill1.cnt = 0;                  // Stop PSC fill engine and clear its IRQ bit.
	gx->psc_dma_prio0 = PSC_DMA_PRIO0(2, 2, 2, 2, 1, 2, 0, 0);
	gx->psc_dma_prio1 = PSC_DMA_PRIO1(15, 15, 2);

	// Stop PPF DMA engine and clear its IRQ bit.
	gx->ppf.cnt = 0;

	// Initialize P3D (GPU).
	gx->p3d[GPUREG_IRQ_ACK] = 0;
	gx->p3d[GPUREG_IRQ_CMP] = 0x12345678;
	gx->p3d[GPUREG_IRQ_MASK] = 0xFFFFFFF0;
	gx->p3d[GPUREG_IRQ_AUTOSTOP] = 1;

	// Not in gsp. We need to start off in configuration mode.
	// If not set the first command list will hang the GPU.
	gx->p3d[GPUREG_START_DRAW_FUNC0] = 1;
}

static void setPdcPresetAndBufs(const GfxLcd lcd, const GfxTopMode mode)
{
	const u32 presetIdx = (lcd == GFX_LCD_TOP ? mode : PDC_PRESET_IDX_BOT);
	const PdcPreset *const preset = &g_pdcPresets[presetIdx];
	Pdc *const pdc = (lcd == GFX_LCD_TOP ? &getGxRegs()->pdc0 : &getGxRegs()->pdc1);

	// Set LCD timings.
	LcdState *const state = &g_gfxState.lcds[lcd];
	iomemcpy(&pdc->h_total, &preset->h_total, offsetof(PdcPreset, pic_dim) - offsetof(PdcPreset, h_total));
	pdc->pic_dim      = preset->pic_dim;
	pdc->pic_border_h = preset->pic_border_h;
	pdc->pic_border_v = preset->pic_border_v;
	pdc->fb_stride    = state->fb_stride;
	pdc->latch_pos    = preset->latch_pos;

	// Set frame buffer addresses and format.
	pdc->fb_a0  = (u32)state->bufs[0];
	pdc->fb_a1  = (u32)state->bufs[2];
	pdc->fb_b0  = (u32)state->bufs[1];
	pdc->fb_b1  = (u32)state->bufs[3];
	pdc->fb_fmt = state->fb_fmt;
}

static void setupDisplayController(const GfxLcd lcd, const GfxTopMode mode)
{
	// Display timing and frame buffer setup.
	setPdcPresetAndBufs(lcd, mode);

	// Setup 1:1 color mapping for all channels.
	Pdc *const pdc = (lcd == GFX_LCD_TOP ? &getGxRegs()->pdc0 : &getGxRegs()->pdc1);
	pdc->color_lut_idx = 0;
	for(u32 i = 0; i < 256; i++)
	{
		pdc->color_lut_data = PDC_COLOR_RGB(1, 1, 1) * i;
	}
}

static void displayControllerInit(const GfxTopMode mode)
{
	// Setup display controller timings.
	// This must be done before LCD init.
	setupDisplayController(GFX_LCD_TOP, mode);
	setupDisplayController(GFX_LCD_BOT, (GfxTopMode)0);

	// Set PDC frame buffer index and start both controllers.
	const u8 swap = g_gfxState.swap;
	GxRegs *const gx = getGxRegs();
	gx->pdc0.swap = swap;    // Bit 1 is not writable.
	gx->pdc1.swap = swap>>1;
	gx->pdc0.cnt  = PDC_CNT_OUT_EN | GFX_PDC0_IRQS | PDC_CNT_EN;
	gx->pdc1.cnt  = PDC_CNT_OUT_EN | GFX_PDC1_IRQS | PDC_CNT_EN;
}

static void stopDisplayControllersSafe(void)
{
	// Hardware bug:
	// If we stop PDC in the middle of DMA it may hang the bus.
	// This has not been observed with VRAM but it frequently
	// happens with FCRAM frame buffers.
	// For this reason we need to wait until vertical border/blanking.
	// In legacy modes we only use VRAM frame buffers so we don't need this workaround.
	// TODO: If we start using threading here we have to try until
	//       we land within the border/blanking area.
	GxRegs *const gx = getGxRegs();
#ifndef LIBN3DS_LEGACY
	u32 v_total = gx->pdc0.v_total;
	u32 v_count = gx->pdc0.v_count;
	u32 bot_border = gx->pdc0.pic_border_v>>16;
	if(v_count < bot_border)
	{
		// vMul = (fb_fmt & PDC_FB_DOUBLE_V ? 1 : 2);
		// ((1000000ull / 8) * 24 * (h_total + 1) * (v_total + 1)) / (268111856u / 8 * vMul)
		// Unless the timing has been altered result is 16713.680875044929009032708 µs.
		const u32 frameDurationUs = 16713;
		TIMER_sleepUs((bot_border - v_count) * frameDurationUs / v_total);
	}
#endif // #ifndef LIBN3DS_LEGACY
	gx->pdc0.cnt  = PDC_CNT_NO_IRQ_ALL; // Stop.
	gx->pdc0.swap = PDC_SWAP_IRQ_ACK_ALL | PDC_SWAP_RST_FIFO;

#ifndef LIBN3DS_LEGACY
	v_total = gx->pdc1.v_total;
	v_count = gx->pdc1.v_count;
	bot_border = gx->pdc1.pic_border_v>>16;
	if(v_count < bot_border)
	{
		// ((1000000ull / 8) * 24 * (h_total + 1) * (v_total + 1)) / (268111856u / 8)
		// Unless the timing has been altered result is 16713.680875044929009032708 µs.
		const u32 frameDurationUs = 16713;
		TIMER_sleepUs((bot_border - v_count) * frameDurationUs / v_total);
	}
#endif // #ifndef LIBN3DS_LEGACY
	gx->pdc1.cnt  = PDC_CNT_NO_IRQ_ALL; // Stop.
	gx->pdc1.swap = PDC_SWAP_IRQ_ACK_ALL | PDC_SWAP_RST_FIFO;
}

static void oldBootloaderWorkaround(void)
{
	// Thanks to Sono for figuring this out.
	// If this code path is not taken ~163 µs.
	// Otherwise varies. ~454 to ~519 µs measured with screen init flag on Luma3DS bootloader.
	if(MCU_readReg(MCU_REG_LCD_PWR) != 0 && (MCU_readReg(MCU_REG_EX_HW_STAT) & 0x60u) == 0)
	{
		// Wait for backlight on events. Backlight always fires after LCD on.
		while((MCU_waitIrqs(MCU_LCD_IRQ_MASK) & (MCU_IRQ_TOP_BL_ON | MCU_IRQ_BOT_BL_ON)) == 0);

		// Reset LCDs in preparation for init.
		LcdRegs *const lcd = getLcdRegs();
		lcd->rst        = LCD_RST_RST;
		lcd->signal_cnt = SIGNAL_CNT_BOTH_DIS; // Stop H-/V-sync control signals.

		// Make sure the state is as expected.
		if((MCU_readReg(MCU_REG_EX_HW_STAT) & 0xE0u) != 0xE0) panic();

		// Power off LCDs. Also powers off backlights.
		MCU_setLcdPower(MCU_LCD_PWR_OFF);
		if(MCU_waitIrqs(MCU_LCD_IRQ_MASK) != MCU_IRQ_LCD_POWER_OFF) panic();

		// Disable backlight PWM.
		lcd->abl0.bl_pwm_cnt = 0;
		lcd->abl1.bl_pwm_cnt = 0;
	}

	// If LCD MCU events arrive before the above wait
	// we will just discard them.
	(void)MCU_getIrqs(MCU_LCD_IRQ_MASK);
}

void GFX_init(const GfxFmt fmtTop, const GfxFmt fmtBot, const GfxTopMode mode)
{
	// Initialize GFX state.
	GfxState *const state = &g_gfxState;
	const u8 mcuLcdState = MCU_LCD_PWR_TOP_BL_OFF | MCU_LCD_PWR_BOT_BL_OFF | MCU_LCD_PWR_OFF;
	const u32 lcdLum = 1; // TODO: Better default.
	state->swap = 0;
	state->swapMask = 3; // Double buffering enabled for both.
	state->mcuLcdState = mcuLcdState;
	state->lcdLum = lcdLum;

	// If the previous FIRM does not wait for LCD MCU events
	// we will get them unexpectedly and this will most likely trigger a panic().
	// TODO: If Luma3DS/fastboot3DS eventually fix this remove this workaround.
	oldBootloaderWorkaround();

	// Do a full hardware reset.
	hardwareReset();

	// Create IRQ events.
	// PSC0, PSC1, PDC0, PDC1, PPF, P3D.
	static_assert(IRQ_P3D - IRQ_PSC0 == 5);
	for(unsigned i = 0; i < 6; i++)
	{
		KHandle kevent = createEvent(false);
		bindInterruptToEvent(kevent, IRQ_PSC0 + i, 14);
		state->events[i] = kevent;
	}

	// Not in gsp. Clear entire VRAM.
	GX_memoryFill((u32*)VRAM_BANK0, PSC_FILL_32_BITS, VRAM_BANK_SIZE, 0,
	              (u32*)VRAM_BANK1, PSC_FILL_32_BITS, VRAM_BANK_SIZE, 0);

	// Hardware bug:
	// Not in gsp but HOS apps do this(?).
	// PPF is (sometimes) glitchy if the first transfer after reset is a texture copy.
	// A single dummy texture copy/display transfer fixes it.
	// For display transfer 64x64 is the minimum or display transfers will be permanently glitchy.
	// Note: Nintendos code does a 128x128 display transfer.
	// Note: 32x32 display transfer with same flags/format will always hang.
	//GX_displayTransfer((u32*)VRAM_BASE, PPF_DIM(128, 128), (u32*)(VRAM_BASE + 0x8000), PPF_DIM(128, 128),
	//                   PPF_O_FMT(GX_RGBA4) | PPF_I_FMT(GX_RGBA4) | PPF_NO_TILED_2_LINEAR);
	GX_textureCopy((u32*)VRAM_BASE, 0, (u32*)(VRAM_BASE + 16), 0, 16);

	// Allocate our frame buffers.
	allocateFramebufs(fmtTop, fmtBot, mode);

	// Get the display controllers up and running.
	// This needs to happen before LCD init (LCDs need some clock pulses to reset?).
	displayControllerInit(mode);

	// Initialize/power on the LCDs.
	// Note: This will also enable forced black output.
	LCD_init(mcuLcdState<<1, lcdLum);

	// Not in gsp. Ensure VRAM is cleared.
	// Also wait for the dummy display transfer/texture copy.
	GFX_waitForPSC0();
	GFX_waitForPSC1();
	GFX_waitForPPF();

	// Enable frame buffer output.
	GFX_setForceBlack(false, false);
}

void GFX_deinit(void)
{
	// Power off backlights and LCDs if on.
	GfxState *const state = &g_gfxState;
	LCD_deinit(state->mcuLcdState);
	state->mcuLcdState = 0;

	// Wait for PSC, PPF, P3D busy.
	// TODO

	// Stop the display controllers.
	TIMER_sleepMs(17); // ?? gsp: Only on deinitialize. Not just on stop.
	stopDisplayControllersSafe();
	TIMER_sleepMs(2); // ?? gsp: Only on deinitialize. Not just on stop.

	// Delete IRQ events.
	// PSC0, PSC1, PDC0, PDC1, PPF, P3D.
	for(unsigned i = 0; i < 6; i++)
	{
		unbindInterruptEvent(IRQ_PSC0 + i);
		deleteEvent(state->events[i]);
		state->events[i] = 0;
	}

	// Deallocate our frame buffers.
	freeFramebufs();

	// Some Homebrew FIRMs detect screen init by poking at this PDN reg.
	// To preserve compatibility we will set it to cold boot state.
	// Also keep VRAM enabled for bootloaders.
	getPdnRegs()->gpu_cnt = PDN_GPU_CNT_CLK_EN | PDN_GPU_CNT_NORST_REGS;
}

// TODO: Don't reallocate if the buffer sizes stay the same. Also always keep left/right pair.
void GFX_setFormat(const GfxFmt fmtTop, const GfxFmt fmtBot, const GfxTopMode mode)
{
	// Avoid glitches while we change the frame buffer format.
	GFX_setForceBlack(true, true);

	// Reallocate frame buffers to free up a little space.
	freeFramebufs();
	allocateFramebufs(fmtTop, fmtBot, mode);

	// Update PDC regs.
	setPdcPresetAndBufs(GFX_LCD_TOP, mode);
	setPdcPresetAndBufs(GFX_LCD_BOT, (GfxTopMode)0);

	// TODO: Should we leave disabling fill to the caller to avoid glitches?
	GFX_setForceBlack(false, false);
}

void GFX_powerOnBacklight(const GfxBl mask)
{
	g_gfxState.mcuLcdState |= mask;

	LCD_setBacklightPower(mask<<1);
}

void GFX_powerOffBacklight(const GfxBl mask)
{
	g_gfxState.mcuLcdState &= ~mask;

	LCD_setBacklightPower(mask);
}

void GFX_setLcdLuminance(const u32 lum)
{
	GfxState *const state = &g_gfxState;
	state->lcdLum = lum;

	LCD_setLuminance(lum);
}

void GFX_setForceBlack(const bool top, const bool bot)
{
	LCD_setForceBlack(top, bot);
}

void GFX_setDoubleBuffering(const GfxLcd lcd, const bool dBuf)
{
	// TODO: We may have to set PDC swap here too so exception printing works.
	GfxState *const state = &g_gfxState;
	state->swapMask = (state->swapMask & ~(1u<<lcd)) | dBuf<<lcd;
}

void* GFX_getBuffer(const GfxLcd lcd, const GfxSide side)
{
	GfxState *const state = &g_gfxState;
	u32 idx = (state->swap ^ state->swapMask)>>lcd & 1u;
	idx = idx * 2 + side;

	return state->lcds[lcd].bufs[idx];
}

void GFX_swapBuffers(void)
{
	GfxState *const state = &g_gfxState;
	u8 swap = state->swap;
	swap ^= state->swapMask;
	state->swap = swap;

	// Set next buffer index and acknowledge IRQs.
	GxRegs *const gx = getGxRegs();
	gx->pdc0.swap = PDC_SWAP_IRQ_ACK_ALL | swap;    // Bit 1 is not writable.
	gx->pdc1.swap = PDC_SWAP_IRQ_ACK_ALL | swap>>1;
}

void GFX_waitForEvent(const GfxEvent event)
{
	KHandle kevent = g_gfxState.events[event];

	if(event == GFX_EVENT_PDC0 || event == GFX_EVENT_PDC1)
	{
		clearEvent(kevent);
	}
	waitForEvent(kevent);
	clearEvent(kevent);
}

void GX_memoryFill(u32 *buf0a, u32 buf0v, u32 buf0Sz, u32 val0, u32 *buf1a, u32 buf1v, u32 buf1Sz, u32 val1)
{
	GxRegs *const gx = getGxRegs();
	if(buf0a != NULL)
	{
		gx->psc_fill0.s_addr = (u32)buf0a>>3;
		gx->psc_fill0.e_addr = ((u32)buf0a + buf0Sz)>>3;
		gx->psc_fill0.val    = val0;
		gx->psc_fill0.cnt    = buf0v | PSC_FILL_EN; // Pattern + start.
	}

	if(buf1a != NULL)
	{
		gx->psc_fill1.s_addr = (u32)buf1a>>3;
		gx->psc_fill1.e_addr = ((u32)buf1a + buf1Sz)>>3;
		gx->psc_fill1.val    = val1;
		gx->psc_fill1.cnt    = buf1v | PSC_FILL_EN; // Pattern + start.
	}
}

// Example: GX_displayTransfer(in, 160u<<16 | 240u, out, 160u<<16 | 240u, 2u<<12 | 2u<<8);
// Copy and unswizzle GBA sized frame in RGB565.
void GX_displayTransfer(const u32 *const src, const u32 inDim, u32 *const dst, const u32 outDim, const u32 flags)
{
	if(src == NULL || dst == NULL) return;

	GxRegs *const gx = getGxRegs();
	gx->ppf.in_addr = (u32)src>>3;
	gx->ppf.out_addr = (u32)dst>>3;
	gx->ppf.dt_indim = inDim;
	gx->ppf.dt_outdim = outDim;
	gx->ppf.flags = flags; // TODO: Do we need to set the crop bit?
	gx->ppf.unk14 = 0;
	gx->ppf.cnt = PPF_EN;
}

// Example: GX_textureCopy(in, (240 * 2)<<12 | (240 * 2)>>4, out, (240 * 2)<<12 | (240 * 2)>>4, 240 * 400);
// Copies every second line of a 240x400 framebuffer.
void GX_textureCopy(const u32 *const src, const u32 inDim, u32 *const dst, const u32 outDim, const u32 size)
{
	if(src == NULL || dst == NULL) return;

	GxRegs *const gx = getGxRegs();
	gx->ppf.in_addr = (u32)src>>3;
	gx->ppf.out_addr = (u32)dst>>3;
	gx->ppf.flags = PPF_TEXCOPY; // TODO: Do we need to set the crop bit?
	gx->ppf.len = size;
	gx->ppf.tc_indim = inDim;
	gx->ppf.tc_outdim = outDim;
	gx->ppf.cnt = PPF_EN;
}

void GX_processCommandList(const u32 size, const u32 *const cmdList)
{
	// Acknowledge last P3D IRQ and wait for the IRQ flag to clear.
	GxRegs *const gx = getGxRegs();
	gx->p3d[GPUREG_IRQ_ACK] = 0;
	while(gx->psc_irq_stat & IRQ_STAT_P3D)
		wait_cycles(0x30);

	// Start command list processing.
	gx->p3d[GPUREG_CMDBUF_SIZE0] = size>>3;
	gx->p3d[GPUREG_CMDBUF_ADDR0] = (u32)cmdList>>3;
	gx->p3d[GPUREG_CMDBUF_JUMP0] = 1;
}

void GFX_sleep(void)
{
	const GfxState *const state = &g_gfxState;
	LCD_deinit(state->mcuLcdState);

	// Wait for PSC, PPF, P3D busy.
	// TODO

	// Backup VRAM areas.
	// TODO: Is this worth implementing? Does not work in legacy modes (FCRAM inaccessible).

	// [Asynchronous] Wait for backlights off.
	// In our case this is not needed.

	// Stop display controllers.
	stopDisplayControllersSafe();

	// Wait for at least 1 horizontal line. 40 µs in this case.
	// 16713.680875044929009032708 / 413 = 40.468960956525251837852 µs.
	// TODO: 40 µs may not be enough in legacy mode?
	TIMER_sleepUs(40);

	// Flush caches to VRAM.
	flushDCacheRange((void*)VRAM_BASE, VRAM_SIZE);

	// Disable VRAM banks. This is needed for PDN sleep mode.
	GxRegs *const gx = getGxRegs();
	gx->psc_vram |= PSC_VRAM_BANK_DIS_ALL;

	// Stop clock.
	PDN_controlGpu(false, false, false);
}

void GFX_sleepAwake(void)
{
	// Resume clock and reset PSC.
	PDN_controlGpu(true, true, false);

	// Restore PSC settings.
	GxRegs *const gx = getGxRegs();
	gx->psc_vram &= ~PSC_VRAM_BANK_DIS_ALL;
	gx->gpu_clk  = 0x70100;
	gx->psc_fill0.cnt = 0;
	gx->psc_fill1.cnt = 0;
	gx->psc_dma_prio0 = PSC_DMA_PRIO0(2, 2, 2, 2, 1, 2, 0, 0);
	gx->psc_dma_prio1 = PSC_DMA_PRIO1(15, 15, 2);
	// -------------------------------------------------------

	// Not from gsp. Clear VRAM.
	GX_memoryFill((u32*)VRAM_BANK0, PSC_FILL_32_BITS, VRAM_BANK_SIZE, 0,
	              (u32*)VRAM_BANK1, PSC_FILL_32_BITS, VRAM_BANK_SIZE, 0);

	// TODO: Do we need the PPF hardware bug workaround here too?
	//       Since PPF is not being reset i don't think so?

	// Initialize display controllers.
	// gsp does completely reinitialize both display controllers here.
	// Since both PDCs are not reset in sleep mode this is not strictly necessary.
	// Warning: If we decide to change this to a full reinit restore the mode!
	const GfxState *const state = &g_gfxState;
	gx->pdc0.swap = state->swap;    // Bit 1 is not writable.
	gx->pdc1.swap = state->swap>>1;
	gx->pdc0.cnt  = PDC_CNT_OUT_EN | GFX_PDC0_IRQS | PDC_CNT_EN;
	gx->pdc1.cnt  = PDC_CNT_OUT_EN | GFX_PDC1_IRQS | PDC_CNT_EN;

	// TODO: Enable 3D LED if needed.

	// Power on LCDs and backlights.
	LCD_init(state->mcuLcdState<<1, state->lcdLum);

	// Active backlight and luminance stuff.
	// TODO

	// Not from gsp. Wait for VRAM clear finish.
	GFX_waitForPSC0();
	GFX_waitForPSC1();

	// Enable frame buffer output.
	GFX_setForceBlack(false, false);
}