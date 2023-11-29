/*
 *   This file is part of open_agb_firm
 *   Copyright (C) 2021 Sergi Granell (xerpi), Paul LaMendola (paulguy), derrek, profi200
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

// Based on code from https://github.com/xerpi/linux_3ds/blob/master/drivers/input/misc/nintendo3ds_codec_hid.c

#include "types.h"
#include "arm11/drivers/codec.h"
#include "arm11/drivers/codec_regmap.h"
#include "arm11/drivers/hw_cal.h"
#include "arm11/drivers/spi.h"
#include "arm11/drivers/pdn.h"
#include "arm11/drivers/timer.h"
#include "arm11/drivers/gpio.h"


typedef enum
{
	I2S_LINE_1 = 0u,
	I2S_LINE_2 = 1u
} I2sLine;

typedef enum
{
	I2S_FREQ_32KHZ = 0u, // 32728.498046875 Hz.
	I2S_FREQ_47KHZ = 1u  // 47605.08806818181818182 Hz.
} I2sFreq;

typedef enum
{
	MIC_FILTER_HALF  = 0u,
	MIC_FILTER_32KHZ = 1u, // I2S1?
	MIC_FILTER_47KHZ = 2u  // I2S2?
} MicFilter;


static bool g_forceOutput = false;



static void switchPage(u16 pageReg)
{
	pageReg >>= 8;

	static u8 currentPage = 0x63;
	if(currentPage != pageReg)
	{
		currentPage = pageReg;

		alignas(4) u8 inBuf[4];
		inBuf[0] = CDC_REG_PAGE_CTRL; // Bit 0: 0 = write, 1 = read.
		inBuf[1] = pageReg;
		NSPI_sendRecv(NSPI_DEV_CS_HIGH | NSPI_DEV_CTR_CODEC, inBuf, NULL, 2, 0);
	}
}

static void readRegArray(const u16 pageReg, void *buf, const u32 size)
{
	switchPage(pageReg);

	alignas(4) u8 inBuf[4];
	inBuf[0] = pageReg<<1 | 1u; // Bit 0: 0 = write, 1 = read.
	NSPI_sendRecv(NSPI_DEV_CS_HIGH | NSPI_DEV_CTR_CODEC, inBuf, buf, 1, size);
}

static u8 readReg(const u16 pageReg)
{
	alignas(4) u8 outBuf[4];
	readRegArray(pageReg, outBuf, 1);

	return outBuf[0];
}

static void writeRegArray(const u16 pageReg, const void *buf, const u32 size)
{
	switchPage(pageReg);

	alignas(4) u8 inBuf[4];
	inBuf[0] = pageReg<<1; // Bit 0: 0 = write, 1 = read.
	NSPI_sendRecv(NSPI_DEV_CTR_CODEC, inBuf, NULL, 1, 0);
	NSPI_sendRecv(NSPI_DEV_CS_HIGH | NSPI_DEV_CTR_CODEC, buf, NULL, size, 0);
}

static void writeReg(const u16 pageReg, const u8 val)
{
	switchPage(pageReg);

	alignas(4) u8 inBuf[4];
	inBuf[0] = pageReg<<1; // Bit 0: 0 = write, 1 = read.
	inBuf[1] = val;
	NSPI_sendRecv(NSPI_DEV_CS_HIGH | NSPI_DEV_CTR_CODEC, inBuf, NULL, 2, 0);
}

#ifdef LIBN3DS_LEGACY
static void writeRegPowerman(const u8 reg, const u8 val)
{
	alignas(4) u8 inBuf[4];
	inBuf[0] = reg & 0x7Fu; // Bit 7: 0 = write, 1 = read.
	inBuf[1] = val;
	NSPI_sendRecv(NSPI_DEV_CS_HIGH | NSPI_DEV_POWERMAN, inBuf, NULL, 2, 0);
}
#endif

static void maskReg(const u16 pageReg, const u8 val, const u8 mask)
{
	u8 data = readReg(pageReg);
	data = (data & ~mask) | (val & mask);
	writeReg(pageReg, data);
}

static void maskWaitReg(const u16 pageReg, const u8 val, const u8 mask)
{
	for(unsigned i = 0; i < 64; i++) // Some kind of timeout? No error checking.
	{
		maskReg(pageReg, val, mask);
		if((readReg(pageReg) & mask) == val) break;
	}
}


// Helpers.
static inline void pdnControlMclk(bool enable)
{
	// CODEC MCLK output enable(s).
	// CODEC is only connected to clock 2.
	getPdnRegs()->i2s_cnt = (enable ? PDN_I2S_CNT_I2S_CLK2_EN : 0);
}

static void softReset(void)
{
	writeReg(CDC_REG_SOFT_RST_CTR, 1);
	TIMER_sleepMs(40);
	switchPage(0); // What? Dummy switch after reset?
}

static inline void setI2sFreq(I2sLine i2sLine, I2sFreq freq)
{
	if(i2sLine == I2S_LINE_1) // I2S1
	{
		u8 val;
		if(freq == I2S_FREQ_32KHZ) val = 0x87;
		else                       val = 0x85;

		writeReg(CDC_REG_DAC_NDAC_VAL, val);
	}
	else // I2S2
	{
		u8 val;
		if(freq == I2S_FREQ_32KHZ) val = 1;
		else                       val = 0;

		maskReg(CDC_REG_100_124, val, 1);
	}
}

// TODO: Make this available as public API?
static void setIirFilterMic(MicFilter filter, const void *const coeff)
{
	u16 pageReg;
	u32 size;
	switch(filter)
	{
		case MIC_FILTER_HALF:
		{
			pageReg = 4<<8 | 8;
			size = sizeof(CdcIir);
			break;
		}
		case MIC_FILTER_32KHZ:
		{
			pageReg = 5<<8 | 8;
			size = sizeof(CdcPrbP25Filters);
			break;
		}
		case MIC_FILTER_47KHZ:
		{
			pageReg = 5<<8 | 72;
			size = sizeof(CdcPrbP25Filters);
			break;
		}
		default:
			return;
	}

	writeRegArray(pageReg, coeff, size);
}

static inline bool isDacMuted(I2sLine i2sLine)
{
	if(i2sLine == I2S_LINE_1)
		return (~readReg(CDC_REG_DAC_VOLUME_CTRL) & 0xCu) == 0; // I2S1
	else
		return (~readReg(CDC_REG_100_119) & 0xCu) == 0; // I2S2
}

static void muteUnmuteDac(I2sLine i2sLine, bool mute)
{
	if(i2sLine == I2S_LINE_1) // I2S1
	{
		maskReg(CDC_REG_DAC_DATA_PATH_SETUP, (mute ? 0u : 0xC0u), 0xC0);
		writeReg(CDC_REG_DAC_VOLUME_CTRL, (mute ? 0xCu : 0));
	}
	else // I2S2
	{
		maskReg(CDC_REG_100_119, (mute ? 0xCu : 0u), 0xC);
	}

	if(mute)
	{
		// Wait until muted.
		const u8 waitVal = (i2sLine == I2S_LINE_1 ? 0x44u : 0x88u);
		for(u32 i = 0; i < 100; i++) // Some kind of timeout? No error checking.
		{
			if(!(~readReg(CDC_REG_100_38) & waitVal)) break;
			TIMER_sleepMs(1);
		}
	}
}

/*static void setIirFilterSound(u8 type, const void *const coeff)
{
}*/

static void powerOnDac(void)
{
	// Power on DAC.
	maskReg(CDC_REG_100_118, 0xC0, 0xC0);

	// 10 ms wait time for safety?
	TIMER_sleepMs(10);

	// Also check the flags for extra safety.
	for(u32 i = 0; i < 100; i++)
	{
		if(!(~readReg(CDC_REG_100_37) & 0x88u)) break;
		TIMER_sleepMs(1);
	}
}

static void enableTouchscreen(void)
{
	maskReg(CDC_REG_103_38, 0x80, 0x80);
	maskReg(CDC_REG_103_36, 0, 0x80);
	maskReg(CDC_REG_103_37, 0x10, 0x3C);
}

static void disableTouchscreen(void)
{
	maskReg(CDC_REG_103_38, 0, 0x80);
	maskReg(CDC_REG_103_36, 0x80, 0x80);
}

static void legacyTouchscreenMode(bool enabled)
{
	if(enabled)
	{
		*((vu16*)0x10141114) |= 2u;
		*((vu16*)0x10141116) |= 2u;
		maskReg(CDC_REG_103_37, 0x40, 0x40);
	}
	else
	{
		maskReg(CDC_REG_103_37, 0, 0x40);
		*((vu16*)0x10141114) &= ~2u;
	}
}


static void headsetInit(void)
{
	// Headset detection stuff.
	GPIO_config(GPIO_2_HEADPH_JACK, GPIO_IRQ_RISING | GPIO_INPUT); // Headphone jack IRQ.
	maskReg(CDC_REG_HEADSET_SEL, GPIO_read(GPIO_2_HEADPH_JACK)<<HEADSET_SEL_HP_SHIFT | HEADSET_SEL_HP_EN, 0x30); // GPIO bitmask 8.
	maskReg(CDC_REG_100_67, 0, 0x80); // TODO: Can we remove this?
	maskReg(CDC_REG_100_67, 0x80, 0x80);
}

static void microphoneInit(const CodecCalBase *const cal)
{
	// Microphone ADC output select stuff.
	maskReg(CDC_REG_100_34, 0, 4);

	// 10 Hz highpass for 32730 Hz.
	alignas(4) static const CdcIir highPass10Hz = {CDC_SWAP_IIR(32737, -32737, 32705)};
	setIirFilterMic(MIC_FILTER_HALF, &highPass10Hz);

	setIirFilterMic(MIC_FILTER_32KHZ, &cal->filterMic32);
	setIirFilterMic(MIC_FILTER_47KHZ, &cal->filterMic47);

	// Microphone impedance settings.
	maskReg(CDC_REG_ADC_IN_SEL_FOR_P_TERMINAL, 0x40, 0xC0);
	maskReg(CDC_REG_ADC_IN_SEL_FOR_M_TERMINAL, 0x40, 0xC0);

	// Microphone bias voltage.
	writeReg(CDC_REG_101_51, cal->microphoneBias);

	// Microphone gain correction.
	maskWaitReg(CDC_REG_101_65, cal->pgaGain, 0x3F);

	// Quick charge? What does that even mean here?
	maskWaitReg(CDC_REG_101_66, cal->quickCharge, 3);

	// Microphone PGA.
	writeReg(CDC_REG_MIC_PGA, 43u & 0x7Fu); // TODO: Should be a global function?
}

static void shutterSoundInit(const CodecCalBase *const cal)
{
	// I2S mute and volume settings on shutter sound playback.
	maskReg(CDC_REG_100_49, 0x44, 0x44); // TODO: TwlBg/AgbBg uses val = 0 here.

	// I2S1 volumes.
	writeReg(CDC_REG_DAC_L_VOLUME_CTRL, cal->shutterVolumeI2s1);
	writeReg(CDC_REG_DAC_R_VOLUME_CTRL, cal->shutterVolumeI2s1);

	// I2S2 volume.
	writeReg(CDC_REG_100_123, cal->shutterVolumeI2s2);
}

static void soundInit(const CodecCalBase *const cal)
{
	// TODO: Depop circuit stuff is CTR only.
	// Speaker depop. Probably to suppress the noise when the driver turns on.
	// But this doesn't stop the pop noise on o3DS (CTR) at all?
	GPIO_config(GPIO_3_0, GPIO_OUTPUT);
	GPIO_write(GPIO_3_0, 1); // GPIO bitmask 0x40
	TIMER_sleepMs(10); // Fixed 10 ms delay when setting this GPIO.

	// TODO: Clean this up.
	// Before enabling the I2S interfaces make sure they are fully
	// off so we can write to all bits in these registers.
	*((vu16*)0x10145000) = 0;
	*((vu16*)0x10145002) = 0;
	// Bit 14 is MCLK1 multiplier (8 and 16 MHz)? I2S1: DSP and GBA 32728.498046875 Hz.
	*((vu16*)0x10145000) = 1u<<15 | 2u<<13 | 32u<<6;
	// Bit 14 is MCLK2 multiplier (8 and 16 MHz)? I2S2: CSND 47605.08806818181818182 Hz.
	*((vu16*)0x10145002) = 1u<<15 | 3u<<13;

	// Speaker driver powerup time?
	maskReg(CDC_REG_101_17, 0x10, 0x1C);

	// I2S1 volume?
	writeReg(CDC_REG_100_122, 0);
	// I2S2 volume?
	writeReg(CDC_REG_100_120, 0);

	// TODO: Function for setting sound filters.

#ifndef LIBN3DS_LEGACY
	{
		// Missing in Twl-/AgbBg but present in codec module.
		// These phase altering filters will distort GBA sound.
		// Square waves look more like triangle waves and the sound is "softer".
		const bool dacMuted = isDacMuted(I2S_LINE_1);
		muteUnmuteDac(I2S_LINE_1, true); // Mute.
		writeRegArray(9<<8 | 2, &cal->filterFree.iir, 6);
		writeRegArray(8<<8 | 12, &cal->filterFree.b, 50);
		writeRegArray(9<<8 | 8, &cal->filterFree.iir, 6);
		writeRegArray(8<<8 | 76, &cal->filterFree.b, 50);
		if(!dacMuted) muteUnmuteDac(I2S_LINE_1, false); // Unmute.
	}
#endif
	{
		const bool dacMuted = isDacMuted(I2S_LINE_2);
		muteUnmuteDac(I2S_LINE_2, true); // Mute.
		writeRegArray(10<<8 | 2, &cal->filterFree.iir, 6);
		writeRegArray(10<<8 | 12, &cal->filterFree.b, 50);
		if(!dacMuted) muteUnmuteDac(I2S_LINE_2, true); // Unmute.
	}

	writeRegArray(12<<8 | 2, &cal->filterSp32, 30);
	writeRegArray(12<<8 | 66, &cal->filterSp32, 30);
	writeRegArray(12<<8 | 32, &cal->filterSp47, 30);
	writeRegArray(12<<8 | 96, &cal->filterSp47, 30);
	writeRegArray(11<<8 | 2, &cal->filterHp32, 30);
	writeRegArray(11<<8 | 66, &cal->filterHp32, 30);
	writeRegArray(11<<8 | 32, &cal->filterHp47, 30);
	writeRegArray(11<<8 | 96, &cal->filterHp47, 30);

	// Power on DAC?
	powerOnDac();

	// Route DAC?
	writeReg(CDC_REG_101_10, 0xA);

	// TODO: Can we omit this since the DACs should already be unmuted?
	muteUnmuteDac(I2S_LINE_1, false); // Unmute I2S1.
	muteUnmuteDac(I2S_LINE_2, false); // Unmute I2S2.

	{ // Headphone.
		// Power on headphone driver?
		// Different settings depending on vendor and revision?
		// It seems we have 2 vendors and 3 revisions. Revsions: 1=B?, 2=C?, 3=D?
		u8 val;
		if((readReg(CDC_REG_0_2) & 0xFu) <= 1u && ((readReg(CDC_REG_0_3) & 0x70u)>>4 <= 2u))
		{
			val = 0x3C;
		}
		else val = 0x1C;
		writeReg(CDC_REG_101_11, val);

		// Unmute headphone driver?
		writeReg(CDC_REG_101_12, (cal->driverGainHp<<3) | 4);

		// Set/unmute analog volume?
		// TODO: We can combine these reg writes.
		writeReg(CDC_REG_101_22, cal->analogVolumeHp); // Left?
		writeReg(CDC_REG_101_23, cal->analogVolumeHp); // Right?
	}
	{ // Speaker.
		// Power on speaker driver?
		maskReg(CDC_REG_101_17, 0xC0, 0xC0);

		// Unmute speaker driver?
		// TODO: We can combine these reg writes.
		writeReg(CDC_REG_101_18, (cal->driverGainSp<<2) | 2); // Left?
		writeReg(CDC_REG_101_19, (cal->driverGainSp<<2) | 2); // Right?

		// // Set/unmute analog volume?
		writeReg(CDC_REG_101_27, cal->analogVolumeSp);
		writeReg(CDC_REG_101_28, cal->analogVolumeSp);
	}

	// Some delay waiting for headphone and speaker
	// outputs to be fully up and running?
	TIMER_sleepMs(38);

	// TODO: Depop circuit stuff is CTR only.
	// Speaker depop. Probably to suppress the noise when the driver turns on.
	// But this doesn't stop the pop noise on o3DS (CTR) at all?
	GPIO_write(GPIO_3_0, 0); // GPIO bitmask 0x40
	TIMER_sleepMs(18); // Fixed 18 ms delay when unsetting this GPIO.
}

static void touchAndCirclePadInit(const CodecCalBase *const cal)
{
	// Stop conversion/sampling?
	writeReg(CDC_REG_103_36, 0x98);

	writeReg(CDC_REG_103_38, 0x00);
	writeReg(CDC_REG_103_37, 0x43);
	writeReg(CDC_REG_103_36, 0x18);
	writeReg(CDC_REG_103_23, cal->analogPrecharge<<4 | cal->analogSense);
	writeReg(CDC_REG_103_25, cal->analogXpPullup<<4 | cal->analogStabilize);
	writeReg(CDC_REG_103_27, cal->ymDriver<<7 | cal->analogDebounce);
	writeReg(CDC_REG_103_39, 0x10u | cal->analogInterval);
	writeReg(CDC_REG_103_38, 0xEC);
	writeReg(CDC_REG_103_36, 0x18);
	writeReg(CDC_REG_103_37, 0x53);

	// Not needed?
	// Console dependent.
	//I2C_writeReg(I2C_DEV_CTR_MCU, 0x26, I2C_readReg(I2C_DEV_CTR_MCU, 0x26) | 0x10);

	// TODO: This should be called externally.
	enableTouchscreen();
}

#ifdef LIBN3DS_LEGACY
static void legacyWorkaround(void)
{
	// CODEC emulates the old DS power management chip.
	// Looks like there is a bug where it doesn't generate
	// IRQs if these regs are not properly initialized.
	writeRegPowerman(0u, 0u);
	writeRegPowerman(4u, 0u);
	writeRegPowerman(16u, 0u);
	writeRegPowerman(0u, 0x0Cu); // Triggers IRQ?
}
#endif

void CODEC_init(void)
{
	static bool inited = false;
	if(inited) return;
	inited = true;


	NSPI_init();

	const CodecCalBase *const cal = &g_cdcCal;

	// Turn on CODEC MCLK and reset it.
	pdnControlMclk(true); // Enable MCLK.
	softReset();

	// Headset detection timing stuff.
	writeReg(CDC_REG_100_67, 0x11);
	maskReg(CDC_REG_101_119, 1, 1);

	// Don't force speaker output.
	maskReg(CDC_REG_GPI1_GPI2_PIN_CTRL, 0x66, 0x66);

	// VREF stuff.
	writeReg(CDC_REG_101_122, 1);

	// PLL stuff.
	maskReg(CDC_REG_100_34, 0x18, 0x18);

	headsetInit();

	// Set CODEC side I2S frequencies (dividers?).
	setI2sFreq(I2S_LINE_1, I2S_FREQ_32KHZ);
	setI2sFreq(I2S_LINE_2, I2S_FREQ_47KHZ);

	microphoneInit(cal);

	shutterSoundInit(cal);

	soundInit(cal);

	touchAndCirclePadInit(cal);

	// TODO: For TWL legacy mode we need to set some uninitialized regs here (bug workaround?).
	//       We also need to watch for a certain GPIO to detect the TWL side
	//       changing I2S frequency.
#ifdef LIBN3DS_LEGACY
	legacyWorkaround();
#endif
}

bool g_touchscreenState = false;
bool g_legacySwitchState = false;

void CODEC_deinit(void)
{
	GPIO_write(GPIO_3_0, 1); // GPIO bitmask 0x40
	TIMER_sleepMs(10); // Fixed 10 ms delay when setting this GPIO.
	g_legacySwitchState = (readReg(CDC_REG_103_37) & 0x40u) != 0;
	if(!g_legacySwitchState) legacyTouchscreenMode(true);
	maskReg(CDC_REG_103_37, 0, 3);
	g_touchscreenState = (readReg(CDC_REG_103_36)>>7) == 0;
	disableTouchscreen();
	maskReg(CDC_REG_100_118, 0, 0xC0);
	TIMER_sleepMs(30);
	for(u32 i = 0; i < 100; i++)
	{
		if(!(readReg(CDC_REG_100_37) & 0x88u)) break;
		TIMER_sleepMs(1);
	}
	maskReg(CDC_REG_100_34, 2, 2);
	TIMER_sleepMs(30);
	for(u32 i = 0; i < 64; i++)
	{
		if(readReg(CDC_REG_100_34) & 1u) break;
		TIMER_sleepMs(1);
	}
	*((vu16*)0x10145000) &= ~0x8000u;
	*((vu16*)0x10145002) &= ~0x8000u;
	getPdnRegs()->i2s_cnt = 0;
	GPIO_write(GPIO_3_0, 0); // GPIO bitmask 0x40
	TIMER_sleepMs(18); // Fixed 18 ms delay when unsetting this GPIO.
}

void CODEC_wakeup(void)
{
	GPIO_write(GPIO_3_0, 1); // GPIO bitmask 0x40
	TIMER_sleepMs(10); // Fixed 10 ms delay when setting this GPIO.
	getPdnRegs()->i2s_cnt = PDN_I2S_CNT_I2S_CLK2_EN;
	*((vu16*)0x10145000) |= 0x8000u;
	*((vu16*)0x10145002) |= 0x8000u;
	//maskReg(0x64, 0x45, 0, 0x30); // Output select automatic
	maskReg(CDC_REG_100_67, 0, 0x80);
	maskReg(CDC_REG_100_67, 0x80, 0x80);
	maskReg(CDC_REG_100_34, 0, 2);
	TIMER_sleepMs(40);
	for(u32 i = 0; i < 40; i++)
	{
		if(!(readReg(CDC_REG_100_34) & 1u)) break;
		TIMER_sleepMs(1);
	}
	maskReg(CDC_REG_100_118, 0xC0, 0xC0);
	TIMER_sleepMs(10);
	for(u32 i = 0; i < 100; i++)
	{
		if(!(~readReg(CDC_REG_100_37) & 0x88u)) break;
		TIMER_sleepMs(1);
	}
	maskReg(CDC_REG_103_37, 3, 3);
	legacyTouchscreenMode(g_legacySwitchState);
	if(g_touchscreenState) enableTouchscreen();
	GPIO_write(GPIO_3_0, 0); // GPIO bitmask 0x40
	TIMER_sleepMs(18); // Fixed 18 ms delay when unsetting this GPIO.
}

void CODEC_runHeadphoneDetection(void)
{
	static u8 prevState = 0;
	const u8 currState = GPIO_read(GPIO_2_HEADPH_JACK);
	static u8 debounceCounter = 0;
	if(prevState != currState)
	{
		if(++debounceCounter > 4)
		{
			prevState = currState;
			debounceCounter = 0;

			if(!g_forceOutput)
				maskReg(CDC_REG_HEADSET_SEL, prevState<<HEADSET_SEL_HP_SHIFT | HEADSET_SEL_HP_EN, 0x30);
		}
	}
	else debounceCounter = 0;
}

void CODEC_setAudioOutput(const CdcAudioOut output)
{
	if(output == CDC_AUDIO_OUT_AUTO)
	{
		g_forceOutput = false;
		return;
	}

	g_forceOutput = true;
	u8 selection;
	if(output == CDC_AUDIO_OUT_SPEAKER) selection = HEADSET_SEL_SP;
	else                                selection = HEADSET_SEL_HP;

	maskReg(CDC_REG_HEADSET_SEL, selection | HEADSET_SEL_HP_EN, 0x30);
}

void CODEC_setVolumeOverride(const s8 vol)
{
	if(vol > 48)
	{
		// Volume control via slider.
		maskReg(CDC_REG_VOL_MICDET_PIN_SAR_ADC, 0x80, 0x80);
	}
	else
	{
		// Volume control via register.
		// Overflows into CDC_REG_DAC_R_VOLUME_CTRL.
		alignas(4) s8 volumes[2] = {vol, vol};
		writeRegArray(CDC_REG_DAC_L_VOLUME_CTRL, volumes, 2);

		maskReg(CDC_REG_VOL_MICDET_PIN_SAR_ADC, 0, 0x80);
	}
}

bool CODEC_getRawAdcData(CdcAdcData *data)
{
	if((readReg(CDC_REG_103_38) & 2u) == 0)
	{
		readRegArray(251<<8 | 1, data, sizeof(CdcAdcData));

		return true;
	}

	// Codec module does this when data is unavailable. Why?
	//switchPage(0);

	return false;
}
