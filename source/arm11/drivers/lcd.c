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

#include <math.h>
#include "types.h"
#include "arm11/drivers/lcd.h"
#include "arm11/drivers/hw_cal.h"
#include "fs.h"
#include "debug.h"
#include "drivers/gfx.h"
#include "arm11/drivers/timer.h"
#include "fb_assert.h"
#include "arm11/drivers/i2c.h"


#define LCD_BL_TIMEOUT  (10u)


// Track if the topscreen parallax barrier is on (3D mode).
static bool g_parallaxOn = false;



void LCD_init(u8 mcuLcdOnMask, const u32 lcdLum)
{
	// Disable frame buffer output to prevent glitches.
	LCD_setForceBlack(true, true);

	// Setup PWM for the parallax barrier. Disabled by default.
	LcdRegs *const lcd = getLcdRegs();
	lcd->parallax_cnt = 0;
	lcd->parallax_pwm = PARALLAX_PWM_TIMING(0xA39, 0xA39);

	// TODO: Should we force both backlight PWM off here since we only modify them as needed below?

	// Reset LCD drivers. Unknown for how long this must be low.
	// gsp seems to rely on boot11/previous FIRM having set reset to active.
	lcd->rst = LCD_RST_RST; // TODO: Not needed?
	//lcd->signal_cnt = SIGNAL_CNT_BOTH_DIS; // Stop H-/V-sync control signals.
	//TIMER_sleepMs(100);

	// TODO: Does it even make sense to check the LCD on bit?
	//       LCDs must always be initialized anyway.
	if(mcuLcdOnMask & MCU_LCD_PWR_ON)
	{
		// ---------------------------------------------------------------- //
		// Timing critical block. Must be done within 4 frames.
		// ---------------------------------------------------------------- //
		// TODO: If we start using threading raise thread priority temporarily.

		// Take LCD drivers out of reset.
		lcd->rst = LCD_RST_NORST;

		// At this point the output must be forced black or
		// the LCD drivers will not sync. Already done above.
		//LCD_setForceBlack(true, true);

		// Route H-/V-sync signals to the LCDs.
		lcd->signal_cnt = 0;

		// Wait for power supply to stabilize and LCD drivers to finish resetting.
		TIMER_sleepMs(10);

		// Initialize LCD drivers.
		LCDI2C_init();

		// Power on LCDs (MCU --> PMIC).
		MCU_setLcdPower(MCU_LCD_PWR_ON);
		// ---------------------------------------------------------------- //
		// Timing critical block end.
		// ---------------------------------------------------------------- //

		// Wait 50 µs for LCD sync. The MCU event wait will cover this.
		if(MCU_waitIrqs(MCU_LCD_IRQ_MASK) != MCU_IRQ_LCD_POWER_ON) panic();
	}

	if(mcuLcdOnMask & ~(MCU_LCD_PWR_ON))
	{
		// Wait for the backlight on flag on both LCDs.
		LCDI2C_waitBacklightsOn();

		// TODO: ABL/other enhancements.

		// Setup Backlight PWM. ABL is disabled for now.
		// Note: PWM duty is set after PWM is enabled in gsp.
		//       There should be no harm if we set duty first.
		lcd->abl0.cnt = 0; // ABL disabled for steady PWM output.
		lcd->abl1.cnt = 0; // ABL disabled for steady PWM output.
		LCD_setLuminance(lcdLum);
		mcuLcdOnMask &= ~(MCU_LCD_PWR_ON);
		LCD_setBacklightPower(mcuLcdOnMask);

		// TODO: Wait for VBlank on both LCDs to prevent flickering.

		// Turn the 3D LED off.
		// TODO: What about the parallax barrier?
		MCU_set3dLedState(0);
	}
}

void LCD_deinit(const u8 mcuLcdOffMask)
{
	// Disable the IR LED on N3DS (PIT)?

	// Disable frame buffer output to prevent glitches.
	LCD_setForceBlack(true, true);

	// Disable parallax barrier and 3D LED.
	//g_parallaxOn = false;
	//LCD_setParallaxBarrier(false);
	MCU_set3dLedState(0); // TODO: Move to GFX driver?

	// Power down LCD backlights and disable backlight PWM.
	// 2 separate steps in gsp.
	LCD_setBacklightPower(mcuLcdOffMask & ~MCU_LCD_PWR_OFF);

	// Power down LCDs.
	LcdRegs *const lcd = getLcdRegs();
	GFX_waitForVBlank0();
	GFX_waitForVBlank0(); // Wait 2 VBlanks to make sure the LCDs are black.
	lcd->rst = LCD_RST_RST;
	lcd->signal_cnt = SIGNAL_CNT_BOTH_DIS;
	if(mcuLcdOffMask & MCU_LCD_PWR_OFF)
	{
		MCU_setLcdPower(MCU_LCD_PWR_OFF);
		if(MCU_waitIrqs(MCU_LCD_IRQ_MASK) != MCU_IRQ_LCD_POWER_OFF) panic();
	}
}

void LCD_setBacklightPower(u8 mcuBlMask)
{
	// Check if we have LCD power bits set.
	// Also check if we have power on and off bits set at the same time.
	fb_assert((mcuBlMask & (MCU_LCD_PWR_ON | MCU_LCD_PWR_OFF)) == 0);
	fb_assert((mcuBlMask & (MCU_LCD_PWR_TOP_BL_ON | MCU_LCD_PWR_TOP_BL_OFF)) < (MCU_LCD_PWR_TOP_BL_ON | MCU_LCD_PWR_TOP_BL_OFF));
	fb_assert((mcuBlMask & (MCU_LCD_PWR_BOT_BL_ON | MCU_LCD_PWR_BOT_BL_OFF)) < (MCU_LCD_PWR_BOT_BL_ON | MCU_LCD_PWR_BOT_BL_OFF));

	// Ignore top LCD backlight bits on o2DS.
	// On o2DS we have 1 big display panel and
	// bottom LCD bits control the whole panel.
	if(MCU_getSystemModel() == SYS_MODEL_2DS)
		mcuBlMask &= ~(MCU_LCD_PWR_TOP_BL_ON | MCU_LCD_PWR_TOP_BL_OFF);

	// Enable backlight PWM before we turn on backlights.
	// This is necessary to avoid the open circuit protection from PMIC.
	// TODO: With certain, very old MCU firmwares PWM prescaler is 6 and denominator 0xFF.
	LcdRegs *const lcd = getLcdRegs();
	if(mcuBlMask & MCU_LCD_PWR_TOP_BL_ON)
	{
		lcd->abl0.bl_pwm_cnt = BL_PWM_EN | BL_PWM_PRESCALER(0) | BL_PWM_DENOMINATOR(0x23E);
	}
	if(mcuBlMask & MCU_LCD_PWR_BOT_BL_ON)
	{
		lcd->abl1.bl_pwm_cnt = BL_PWM_EN | BL_PWM_PRESCALER(0) | BL_PWM_DENOMINATOR(0x23E);
	}

	MCU_setLcdPower(mcuBlMask);
	if(MCU_waitIrqs(MCU_LCD_IRQ_MASK) != MCU_LCD_PWR_2_IRQ(mcuBlMask)) panic();

	// Disable backligh PWM after turning off backlights.
	// Same reason as for power on.
	if(mcuBlMask & MCU_LCD_PWR_TOP_BL_OFF)
	{
		lcd->abl0.bl_pwm_cnt = 0;
	}
	if(mcuBlMask & MCU_LCD_PWR_BOT_BL_OFF)
	{
		lcd->abl1.bl_pwm_cnt = 0;
	}
}

static u16 getPwmDenominator(const u8 lcd)
{
	LcdRegs *const lcdRegs = getLcdRegs();
	vu32 *const ptr = (lcd == 0 ? &lcdRegs->abl0.bl_pwm_cnt : &lcdRegs->abl1.bl_pwm_cnt);

	const u32 reg = *ptr;
	return (reg & BL_PWM_EN ? reg & BL_PWM_DENOMINATOR_MASK : 511u) + 1;
}

// Converts luminance (cd/m²) to PWM duty.
static u16 lum2Brightness(const u32 lum, const float coeffs[3], const float base, const u16 min)
{
	// ax² + bx + c = y
	const float l = (float)lum;
	float y = coeffs[0] * l * l + coeffs[1] * l + coeffs[2];
	y = (y <= min ? min : y) / base;

	return (u16)(y + 0.5f);
}

// Converts PWM duty to luminance (cd/m²).
// Original calculation from gsp disassembly.
/*static u32 brightness2lum(const u16 bright, const float coeffs[3], const float base)
{
	const float x = (float)bright * base;
	const float bb = coeffs[1] * coeffs[1];
	const float ac4 = coeffs[0] * (x - coeffs[2]) * 4.f;
	const float l = (sqrtf(bb + ac4) - coeffs[1]) / (coeffs[0] + coeffs[0]);

	return (u32)(l + 0.5f);
}
// Adjusted variant based on code from Luma3DS.
static u32 brightness2lum(const u16 bright, const float coeffs[3], const float base)
{
	const float x = (float)bright * base;
	const float bb = coeffs[1] * coeffs[1];
	const float _4ac = 4.f * coeffs[0] * (coeffs[2] - x);
	const float l = (sqrtf(bb - _4ac) - coeffs[1]) / (coeffs[0] + coeffs[0]);

	return (u32)(l + 0.5f);
}*/

void LCD_setLuminance(u32 lum)
{
	lum = (lum > g_blPwmCal.lumLevels[6] ? g_blPwmCal.lumLevels[6] : lum);
	lum = (lum < g_blPwmCal.lumLevels[0] ? g_blPwmCal.lumLevels[0] : lum);

	// TODO: ABL RS lookup table stuff.

	// This calculation is currently a nop since we hardcoded ratio to 1.0.
	/*const float lumRatio[2] = {1.f, 1.f};
	u32 lumTop = (float)lum * lumRatio[0];
	lumTop = (lumTop < g_blPwmCal.lumLevels[0] ? g_blPwmCal.lumLevels[0] : lumTop);
	u32 lumBot = (float)lum * lumRatio[1];
	lumBot = (lumBot < g_blPwmCal.lumLevels[0] ? g_blPwmCal.lumLevels[0] : lumBot);*/
	const u32 lumTop = lum;
	const u32 lumBot = lum;

	LcdRegs *const lcd = getLcdRegs();
	const float baseTop = (float)g_blPwmCal.hwBrightnessBase / getPwmDenominator(0);
	const float baseBot = (float)g_blPwmCal.hwBrightnessBase / getPwmDenominator(1);
	const u16 minBright = g_blPwmCal.hwBrightnessMin;
	lcd->abl0.bl_pwm_duty = lum2Brightness(lumTop, g_blPwmCal.coeffs[g_parallaxOn ? 2 : 1], baseTop, minBright);
	lcd->abl1.bl_pwm_duty = lum2Brightness(lumBot, g_blPwmCal.coeffs[0], baseBot, minBright);
}

void LCD_setLuminanceLevel(u8 level)
{
	const u16 numLevels = g_blPwmCal.numLevels;
	level = (level < 1 ? 1 : (level > numLevels ? numLevels : level));

	// TODO: When the charger is plugged in gsp boosts brightness.
#if 0
	if(isChargerPluggedIn)
	{
		static const u8 boostLut[] = {0, 1, 2, 3, 5, 6};
		level = boostLut[level];
	}
#endif

	LCD_setLuminance(g_blPwmCal.lumLevels[level - 1]);
}

void LCD_setParallaxBarrier(const bool on)
{
	LcdRegs *const lcd = getLcdRegs();
	if(on)
	{
		// TODO: Parallax barrier on delay from HWCAL.
		/*if(onDelay > 0)
		{
			// Update luminance for 3D mode.
			// TODO

			TIMER_sleepMs(onDelay);
		}*/

		lcd->parallax_cnt = PARALLAX_CNT_PWM1_EN | PARALLAX_CNT_PWM0_EN;
		g_parallaxOn = true;

		// TODO: Parallax barrier on delay from HWCAL.
		// Why is there a delay before and after in gsp??
		/*if(onDelay <= 0)
		{
			if(onDelay < 0) TIMER_sleepMs(-onDelay);

			// Update luminance for 3D mode.
			// TODO
		}*/
	}
	else
	{
		// TODO: Parallax barrier off delay from HWCAL.
		/*if(offDelay > 0)
		{
			// Update luminance for 2D mode.
			// TODO

			TIMER_sleepMs(offDelay);
		}*/

		lcd->parallax_cnt = 0;
		g_parallaxOn = false;

		// TODO: Parallax barrier off delay from HWCAL.
		// Why is there a delay before and after in gsp??
		/*if(offDelay <= 0)
		{
			if(offDelay < 0) TIMER_sleepMs(-offDelay);

			// Update luminance for 2D mode.
			// TODO
		}*/
	}

	// Note: 3D LED is usually turned on/off after the barrier.
}
// ---------------------------------------------------------------- //


u8 LCDI2C_readReg(const u8 lcd, const LcdI2cReg reg)
{
	u8 buf[2];
	const I2cDevice dev = (lcd == 0 ? I2C_DEV_LCD0 : I2C_DEV_LCD1);

	bool res = I2C_write(dev, LCD_I2C_REG_READ_ADDR, reg);
	if(res)
	{
		res = I2C_readArray(dev, LCD_I2C_REG_READ_ADDR, buf, 2);
	}

	return (res ? buf[1] : 0xFF);
}

bool LCDI2C_writeReg(const u8 lcd, const LcdI2cReg reg, const u8 data)
{
	const I2cDevice dev = (lcd == 0 ? I2C_DEV_LCD0 : I2C_DEV_LCD1);

	return I2C_write(dev, reg, data);
}

void LCDI2C_init(void)
{
	const u16 revs = LCDI2C_getRevisions();

	// Top LCD.
	if(revs & 0xFFu)
	{
		LCDI2C_writeReg(0, LCD_I2C_REG_RST_STATUS, LCD_REG_RST_STATUS_NONE);
	}
	else
	{
		LCDI2C_writeReg(0, LCD_I2C_REG_UNK11, LCD_REG_UNK11_UNK10);
		LCDI2C_writeReg(0, LCD_I2C_REG_HS_SERIAL, LCD_REG_HS_SERIAL_ON);
	}

	// Bottom LCD.
	if(revs>>8)
	{
		LCDI2C_writeReg(1, LCD_I2C_REG_RST_STATUS, LCD_REG_RST_STATUS_NONE);
	}
	else
	{
		LCDI2C_writeReg(1, LCD_I2C_REG_UNK11, LCD_REG_UNK11_UNK10);
	}

	LCDI2C_writeReg(0, LCD_I2C_REG_STATUS, LCD_REG_STATUS_OK); // Initialize status flag.
	LCDI2C_writeReg(1, LCD_I2C_REG_STATUS, LCD_REG_STATUS_OK); // Initialize status flag.
	LCDI2C_writeReg(0, LCD_I2C_REG_POWER, LCD_REG_POWER_ON);   // Power on LCD.
	LCDI2C_writeReg(1, LCD_I2C_REG_POWER, LCD_REG_POWER_ON);   // Power on LCD.
}

void LCDI2C_waitBacklightsOn(void)
{
	const u16 revs = LCDI2C_getRevisions();

	if((revs & 0xFFu) == 0 || (revs>>8) == 0)
	{
		// Bug workaround for early LCD driver revisions?
		TIMER_sleepMs(150);
	}
	else
	{
		u32 i = 0;
		do
		{
			const u8 top = LCDI2C_readReg(0, LCD_I2C_REG_BL_STATUS);
			const u8 bot = LCDI2C_readReg(1, LCD_I2C_REG_BL_STATUS);

			if(top == LCD_REG_BL_STATUS_ON && bot == LCD_REG_BL_STATUS_ON) break;

			TIMER_sleepUs(33333);
		} while(++i < LCD_BL_TIMEOUT);
	}
}

u16 LCDI2C_getRevisions(void)
{
	static bool lcdRevsRead = false;
	static u16 lcdRevsCache = 0;

	if(!lcdRevsRead)
	{
		lcdRevsRead = true;

		lcdRevsCache = LCDI2C_readReg(0, LCD_I2C_REG_REVISION) |
		               (u16)LCDI2C_readReg(1, LCD_I2C_REG_REVISION)<<8;
	}

	return lcdRevsCache;
}