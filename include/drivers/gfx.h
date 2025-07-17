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
#include "rgb_conv.h"


#ifdef __cplusplus
extern "C"
{
#endif

// Note: The LCDs are physically rotated 90° CCW.
#define LCD_WIDTH_TOP        (240u)
#define LCD_HEIGHT_TOP       (400u)
#define LCD_WIDE_HEIGHT_TOP  (800u) // In wide mode.
#define LCD_WIDTH_BOT        (240u)
#define LCD_HEIGHT_BOT       (320u)

#define BGR8_2_565(r, g, b)  ((((249u * (r) + 1024)>>11)<<11) | (((253u * (g) + 512)>>10)<<5) | ((249u * (b) + 1024)>>11))


// Pixel formats.
typedef enum
{
	GFX_ABGR8  = 0u, // {0xAA, 0xBB, 0xGG, 0xRR} in memory from lowest to highest address.
	GFX_BGR8   = 1u, // {0xBB, 0xGG, 0xRR} in memory from lowest to highest address.
	GFX_BGR565 = 2u, // {0bGGGBBBBB, 0bRRRRRGGG} in memory from lowest to highest address.
	GFX_A1BGR5 = 3u, // {0bGGBBBBBA, 0bRRRRRGGG} in memory from lowest to highest address.
	GFX_ABGR4  = 4u  // {0bBBBBAAAA, 0bRRRRGGGG} in memory from lowest to highest address.
} GfxFmt;

typedef enum
{
	GFX_LCD_TOP = 0u,
	GFX_LCD_BOT = 1u
} GfxLcd;

typedef enum
{
	GFX_SIDE_LEFT  = 0u,
	GFX_SIDE_RIGHT = 1u
} GfxSide;

static inline u8 GFX_getPixelSize(const GfxFmt fmt)
{
	if(fmt == GFX_ABGR8)
	{
		return 4;
	}
	else if(fmt == GFX_BGR8)
	{
		return 3;
	}

	return 2; // BGR565, A1BGR5, ABGR4.
}



#ifdef __ARM11__
typedef enum
{
	GFX_TOP_2D   = 0u, // 240x400.
	GFX_TOP_WIDE = 1u, // 240x800.
	GFX_TOP_3D   = 2u  // Stereo 240x400 (left + right eye).
} GfxTopMode;

typedef enum
{
	GFX_EVENT_PSC0   = 0u,
	GFX_EVENT_PSC1   = 1u,
	GFX_EVENT_PDC0   = 2u,
	GFX_EVENT_PDC1   = 3u,
	GFX_EVENT_PPF    = 4u,
	GFX_EVENT_P3D    = 5u
} GfxEvent;

// Note: Keep this synchronized with MCU backlight bits.
typedef enum
{
	GFX_BL_BOT   = BIT(2),
	GFX_BL_TOP   = BIT(4),
	GFX_BL_BOTH  = GFX_BL_TOP | GFX_BL_BOT
} GfxBl;



/**
 * @brief      Turns on the LCDs and initializes graphics.
 *
 * @param[in]  fmtTop   Top LCD pixel format.
 * @param[in]  fmtBot   Bottom LCD pixel format.
 * @param[in]  topMode  Top LCD mode.
 */
void GFX_init(const GfxFmt fmtTop, const GfxFmt fmtBot, const GfxTopMode mode);

/**
 * @brief      Equal to GFX_init(GFX_BGR8, GFX_BGR8, GFX_TOP_2D).
 */
static inline void GFX_initDefault(void)
{
	GFX_init(GFX_BGR8, GFX_BGR8, GFX_TOP_2D);
}

/**
 * @brief      Turns LCDs off and deinitializes graphics.
 */
void GFX_deinit(void);

/**
 * @brief      Sets the frame buffer format.
 *
 * @param[in]  fmtTop  The top frame buffer format.
 * @param[in]  fmtBot  The bottom frame buffer format.
 * @param[in]  mode    Top LCD mode.
 */
void GFX_setFormat(const GfxFmt fmtTop, const GfxFmt fmtBot, const GfxTopMode mode);

/**
 * @brief      Returns the frame buffer format of the given LCD.
 *
 * @param[in]  lcd   The lcd.
 *
 * @return     The frame buffer format.
 */
GfxFmt GFX_getFormat(const GfxLcd lcd);

/**
 * @brief      Returns the current display mode of the top LCD.
 *
 * @return     The top LCD display mode.
 */
GfxTopMode GFX_getTopMode(void);

/**
 * @brief      Powers on LCD backlights.
 *
 * @param[in]  mask  The backlights to power on.
 */
void GFX_powerOnBacklight(const GfxBl mask);

/**
 * @brief      Powers off LCD backlights.
 *
 * @param[in]  mask  The backlights to power off.
 */
void GFX_powerOffBacklight(const GfxBl mask);

/**
 * @brief      Sets the LCD luminance for both LCDs.
 *
 * @param[in]  lum   The luminance.
 */
void GFX_setLcdLuminance(const u32 lum);

/**
 * @brief      Forces black output to the LCDs.
 *
 * @param[in]  top   Set to true for black top LCD.
 * @param[in]  bot   Set to true for black bottom LCD.
 */
void GFX_setForceBlack(const bool top, const bool bot);

/**
 * @brief      Enables or disables double buffering for a LCD.
 *
 * @param[in]  lcd   The lcd.
 * @param[in]  dBuf  Set to true to enable double buffering.
 */
void GFX_setDoubleBuffering(const GfxLcd lcd, const bool dBuf);

/**
 * @brief      Returns the currently inactive frame buffer for drawing.
 *             Always the same buffer in single buffer mode.
 *
 * @param[in]  lcd   The lcd to return the buffer pointer for.
 * @param[in]  side  The side in 3D mode. Otherwise always use GFX_SIDE_LEFT.
 *
 * @return     Returns the frame buffer pointer.
 */
void* GFX_getBuffer(const GfxLcd lcd, const GfxSide side);

/**
 * @brief      Flushes the CPU data cache for all current frame buffers.
 */
void GFX_flushBuffers(void);

/**
 * @brief      Swaps the buffers for all LCDs with double buffering enabled.
 */
void GFX_swapBuffers(void);

/**
 * @brief      Waits for a GPU hardware event.
 *
 * @param[in]  event    The event to wait for.
 * @param[in]  discard  Set to true to clear the event before waiting.
 */
void GFX_waitForEvent(const GfxEvent event);

// Helpers
#define GFX_waitForPSC0()     GFX_waitForEvent(GFX_EVENT_PSC0)
#define GFX_waitForPSC1()     GFX_waitForEvent(GFX_EVENT_PSC1)
#define GFX_waitForVBlank0()  GFX_waitForEvent(GFX_EVENT_PDC0)
#define GFX_waitForVBlank1()  GFX_waitForEvent(GFX_EVENT_PDC1)
#define GFX_waitForPPF()      GFX_waitForEvent(GFX_EVENT_PPF)
#define GFX_waitForP3D()      GFX_waitForEvent(GFX_EVENT_P3D)

/**
 * @brief      Fill memory with a pattern via DMA. 2 fill engines.
 *
 * @param      buf0a   Buffer 0 pointer. Must be 8 bytes aligned. Set to NULL disable.
 * @param[in]  buf0v   Fill 0 pattern size flags.
 * @param[in]  buf0Sz  Fill 0 buffer size in bytes. Must be 8 bytes aligned.
 * @param[in]  val0    Fill 0 value.
 * @param      buf1a   Buffer 1 pointer. Must be 8 bytes aligned. Set to NULL disable.
 * @param[in]  buf1v   Fill 1 pattern size flags.
 * @param[in]  buf1Sz  Fill 1 buffer size in bytes. Must be 8 bytes aligned.
 * @param[in]  val1    Fill 1 value.
 */
void GX_memoryFill(u32 *buf0a, u32 buf0v, u32 buf0Sz, u32 val0, u32 *buf1a, u32 buf1v, u32 buf1Sz, u32 val1);

/**
 * @brief      Transfers a pixel buffer from A to B with optional (de)swizzling.
 *
 * @param[in]  src     Source pointer. Must be 8 bytes aligned.
 * @param[in]  inDim   Input dimensions.
 * @param      dst     Destination pointer. Must be 8 bytes aligned.
 * @param[in]  outDim  Output dimensions.
 * @param[in]  flags   Transfer control flags. Swizzling, flip, anti-aliasing ect.
 */
void GX_displayTransfer(const u32 *const src, const u32 inDim, u32 *const dst, const u32 outDim, const u32 flags);

/**
 * @brief      Raw DMA copy without conversion.
 *
 * @param[in]  src     Source pointer. Must be 8 bytes aligned.
 * @param[in]  inDim   Input dimensions.
 * @param      dst     Destination pointer. Must be 8 bytes aligned.
 * @param[in]  outDim  Output dimensions.
 * @param[in]  size    Total transfer size in bytes.
 */
void GX_textureCopy(const u32 *const src, const u32 inDim, u32 *const dst, const u32 outDim, const u32 size);

/**
 * @brief      Starts GPU command buffer execution.
 *
 * @param[in]  size     Buffer size in bytes. Must be 8 bytes aligned.
 * @param[in]  cmdList  Command list buffer pointer. Must be 8 bytes aligned.
 */
void GX_processCommandList(const u32 size, const u32 *const cmdList);

/**
 * @brief      Power down and prepare graphics hardware for sleep mode.
 */
void GFX_sleep(void);

/**
 * @brief      Wake up/initialize graphics hardware after sleep mode.
 */
void GFX_sleepAwake(void);

bool GFX_setupExceptionFrameBuffer(void);

#endif // #ifdef __ARM11__

#ifdef __cplusplus
} // extern "C"
#endif