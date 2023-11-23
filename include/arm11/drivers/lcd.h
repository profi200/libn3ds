#pragma once

/*
 *   This file is part of open_agb_firm
 *   Copyright (C) 2023 derrek, profi200
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
#include "types.h"
#include "arm11/drivers/mcu.h"
#include "mem_map.h"


#define LCD_REGS_BASE     (IO_AXI_BASE + 0x2000)

#define MCU_LCD_IRQ_MASK  (MCU_IRQ_TOP_BL_ON | MCU_IRQ_TOP_BL_OFF | \
                           MCU_IRQ_BOT_BL_ON | MCU_IRQ_BOT_BL_OFF | \
                           MCU_IRQ_LCD_POWER_ON | MCU_IRQ_LCD_POWER_OFF)

// Adaptive/Active backlight registers.
typedef struct
{
	vu32 cnt;            // 0x000 Control.
	vu32 fill;           // 0x004 Fill color. Works if ABL is disabled.
	u8 _0x08[8];
	vu32 win_sx;         // 0x010 Histogram start X.
	vu32 win_ex;         // 0x014 Histogram end X (inclusive).
	vu32 win_sy;         // 0x018 Histogram start Y.
	vu32 win_ey;         // 0x01C Histogram end Y (inclusive).
	vu32 gth_ratio;      // 0x020
	vu32 gth_min;        // 0x024
	vu32 gth_max;        // 0x028
	vu32 gth_max_ex;     // 0x02C
	vu32 inertia;        // 0x030
	u8 _0x34[4];
	vu32 rs_min;         // 0x038
	vu32 rs_max;         // 0x03C
	vu32 bl_pwm_duty;    // 0x040 Backlight PWM duty (on time). Works if ABL is disabled.
	vu32 bl_pwm_cnt;     // 0x044 Backlight PWM control. Works if ABL is disabled.
	u8 _0x48[8];
	vu32 unk50;          // 0x050 Histogram RGB?
	vu32 unk54;          // 0x054 Histogram RGB too?
	u8 _0x58[8];
	vu32 dither_patt[8]; // 0x060 ? Mask 0xCCCC for each entry. Every second reg is a filler and read-only.
	vu32 rs_lut[9];      // 0x080
	u8 _0xa4[0x4c];
	const vu32 unkF0;    // 0x0F0 ?
	const vu32 unkF4;    // 0x0F4 ?
	const vu32 unkF8;    // 0x0F8 ?
	u8 _0xfc[0x204];
	vu32 unk_coef[64];   // 0x300 ? New 3DS only. RGB coefficients?
	vu32 unk_coef2[100]; // 0x400 ? Each entry has 10 writable bits. Is this supposed to be read-only?
} Abl;
static_assert(offsetof(Abl, unk_coef2[99]) == 0x58C, "Error: Member unk_coef2[99] of Abl is not at offset 0x58C!");

typedef struct
{
	vu32 parallax_cnt; // 0x000 Parallax PWM control.
	vu32 parallax_pwm; // 0x004 Parallax PWM timings.
	const vu32 status; // 0x008 ?
	vu32 signal_cnt;   // 0x00C LCD signal control.
	vu32 unk_lvds;     // 0x010 LVDS related? Bits 0-3 (LCD0?) and 8-12 (LCD1?) writable. Usually 0x900.
	vu32 rst;          // 0x014 Active low reset for external LCD controllers.
	vu32 unk18;        // 0x018 ?
	u8 _0x1c[0x1e4];
	Abl abl0;          // 0x200 LCD0 adaptive/active backlight control.
	u8 _0x790[0x270];
	Abl abl1;          // 0xA00 LCD1 adaptive/active backlight control.
} LcdRegs;
static_assert(offsetof(LcdRegs, abl1) == 0xA00, "Error: Member abl1 of LcdRegs is not at offset 0xA00!");

ALWAYS_INLINE LcdRegs* getLcdRegs(void)
{
	return (LcdRegs*)LCD_REGS_BASE;
}


// REG_LCD_PARALLAX_CNT
#define PARALLAX_CNT_PWM0_EN    (1u)     // PWM0 enable.
#define PARALLAX_CNT_PWM0_UNK   (1u<<1)  // Turns PWM0 automatically on and off?
#define PARALLAX_CNT_PWM0_INV   (1u<<2)  // Swaps PWM0 on/off timings.
#define PARALLAX_CNT_PWM1_EN    (1u<<16) // PWM1 enable. TODO: Or is this rather a selection bit (1 PWM out at a time)?
#define PARALLAX_CNT_PWM1_UNK   (1u<<17) // Turns PWM1 automatically on and off?
#define PARALLAX_CNT_PWM1_INV   (1u<<18) // Swaps PWM1 on/off timings.

// REG_LCD_PARALLAX_PWM
#define PARALLAX_PWM_TIMING(on, off)  ((on)<<16 | (off)) // PWM on/off time. For each "(N+1)*0.9µs".

// REG_LCD_STATUS

// REG_LCD_SIGNAL_CNT
#define SIGNAL_CNT_LCD0_DIS     (1u)     // Disables top LCD control signals.
#define SIGNAL_CNT_LCD1_DIS     (1u<<16) // Disables bottom LCD control signals.
#define SIGNAL_CNT_BOTH_DIS     (SIGNAL_CNT_LCD1_DIS | SIGNAL_CNT_LCD0_DIS)

// REG_LCD_RST
#define LCD_RST_RST             (0u) // LCD controllers under eset.
#define LCD_RST_NORST           (1u) // LCD controllers out of reset.

// REG_LCD_ABL_CNT
#define ABL_EN                  (1u)
#define ABL_SPATIAL_DITHER_EN   (1u<<8)
#define ABL_TEMPORAL_DITHER_EN  (1u<<9)

// REG_LCD_ABL_FILL
#define ABL_FILL_RGB(r, g, b)   ((b)<<16 | (g)<<8 | (r))
#define ABL_FILL_EN             (1u<<24)

// REG_LCD_ABL_BL_PWM_CNT
#define BL_PWM_DENOMINATOR(d)    ((d) & 0x3FFu)
#define BL_PWM_DENOMINATOR_MASK  (0x3FFu)
#define BL_PWM_PRESCALER(p)      (((p) & 15u)<<12)
#define BL_PWM_PRESCALER_MASK    (15u<<12)
#define BL_PWM_EN                (1u<<16)
// TODO: Bits 16-18 belong together. Find out their purpose. PWM counter maybe?
#define BL_PWM_UNK19             (1u<<19)            // N3DS only.
#define BL_PWM_UNK20(x)          (((x) & 15u)<<20)   // N3DS only.
#define BL_PWM_UNK24(x)          (((x) & 0x7Fu)<<24) // N3DS only.
#define BL_PWM_UNK31             (1u<<31)            // N3DS only. Doubles PWM frequency?



/**
 * @brief      Forces black output instead of the frame buffer.
 *
 * @param[in]  top   Top screen output disable. true = disabled.
 * @param[in]  bot   Bottom screen output disable. true = disabled.
 */
static inline void LCD_setForceBlack(const bool top, const bool bot)
{
	LcdRegs *const lcd = getLcdRegs();
	lcd->abl0.fill = (top ? ABL_FILL_EN | ABL_FILL_RGB(0, 0, 0) : 0);
	lcd->abl1.fill = (bot ? ABL_FILL_EN | ABL_FILL_RGB(0, 0, 0) : 0);
}

/**
 * @brief      Initializes LCDs and backlights.
 *
 * @param[in]  mcuLcdOnMask  LCD/backlight on mask. See mcu driver.
 * @param[in]  lcdLum        The initial luminance for both LCDs.
 */
void LCD_init(u8 mcuLcdOnMask, const u32 lcdLum);

/**
 * @brief      Deinitializes LCDs and backlights.
 *
 * @param[in]  mcuLcdOffMask  LCD/backlight off mask. See mcu driver.
 */
void LCD_deinit(const u8 mcuLcdOffMask);

/**
 * @brief      Powers LCD backlights on or off.
 *
 * @param[in]  mcuBlMask  Backlight on/off mask. See mcu driver.
 */
void LCD_setBacklightPower(u8 mcuBlMask);

/**
 * @brief      Sets the luminance (cd/m²) for both LCDs.
 *
 * @param[in]  lum   The luminance.
 */
void LCD_setLuminance(u32 lum);

/**
 * @brief      Sets the luminance level for both LCDs. Same as backlight levels in HOME menu.
 *
 * @param[in]  level  The luminance level. 1-5.
 */
void LCD_setLuminanceLevel(u8 level);

/**
 * @brief      Turns on or off 3D mode for the top LCD.
 *
 * @param[in]  on    Set to true for on and false for off.
 */
//void LCD_setParallaxBarrier(const bool on);
// ------------------------------------------------------------------------------------------ //


// LCD I2C regs.
typedef enum
{
	LCD_I2C_REG_POWER      = 0x01u,
	LCD_I2C_REG_UNK11      = 0x11u,
	LCD_I2C_REG_READ_ADDR  = 0x40u,
	LCD_I2C_REG_HS_SERIAL  = 0x50u, // Highspeed serial for upper LCD only.
	LCD_I2C_REG_UNK54      = 0x54u, // Checksum on/off?
	LCD_I2C_REG_UNK55      = 0x55u, // Checksum status?
	LCD_I2C_REG_STATUS     = 0x60u, // Initially 0x01.
	LCD_I2C_REG_BL_STATUS  = 0x62u, // Backlight status.
	LCD_I2C_REG_RST_STATUS = 0xFEu, // Reset status. Initially 0x00.
	LCD_I2C_REG_REVISION   = 0xFFu, // Revision/vendor infos.
} LcdI2cReg;

// LCD_I2C_REG_POWER
#define LCD_REG_POWER_BLACK      (0x11u) // Force blackscreen.
#define LCD_REG_POWER_ON         (0x10u) // Normal operation.
#define LCD_REG_POWER_OFF        (0x00u) // LCD powered off.

// LCD_I2C_REG_UNK11
#define LCD_REG_UNK11_UNK10      (0x10u) // Written on init.

// LCD_I2C_REG_HS_SERIAL
#define LCD_REG_HS_SERIAL_ON     (0x01u) // Enable highspeed serial.

// LCD_I2C_REG_UNK54

// LCD_I2C_REG_UNK55

// LCD_I2C_REG_STATUS
#define LCD_REG_STATUS_OK        (0x00u)
#define LCD_REG_STATUS_ERR       (0x01u)

// LCD_I2C_REG_BL_STATUS
#define LCD_REG_BL_STATUS_OFF    (0x00u)
#define LCD_REG_BL_STATUS_ON     (0x01u)

// LCD_I2C_REG_RST_STATUS
#define LCD_REG_RST_STATUS_NONE  (0xAAu)
#define LCD_REG_RST_STATUS_RST   (0x00u)



u8 LCDI2C_readReg(const u8 lcd, const LcdI2cReg reg);
bool LCDI2C_writeReg(const u8 lcd, const LcdI2cReg reg, const u8 data);
void LCDI2C_init(void);
void LCDI2C_waitBacklightsOn(void);
u16 LCDI2C_getRevisions(void);