/*
 *   This file is part of libn3ds
 *   Copyright (C) 2023 profi200
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

#include <stdlib.h>
#include <string.h>
#include "arm11/drivers/hw_cal.h"
#include "fs.h"


alignas(4) CodecCalBase g_cdcCal =
{
	.driverGainHp      =   0, // 0 dB.
	.driverGainSp      =   1, // 12 dB.
	.analogVolumeHp    =   0, // 0 dB.
	.analogVolumeSp    =   7, // -3.5 dB.
	.shutterVolumeI2s1 =  -3,
	.shutterVolumeI2s2 = -20,
	.microphoneBias    =   3,
	.quickCharge       =   2,
	.pgaGain           =   0,
	.padding           = {0, 0, 0},
	// Note:
	// The sample rate used for all filters
	// is the word clock of the I2S line.
	.filterHp32 = // I2S1.
	{
		{CDC_SWAP_BIQUAD( 32767,      0,      0,      0,      0)},
		{CDC_SWAP_BIQUAD( 32767,      0,      0,      0,      0)},
		{CDC_SWAP_BIQUAD( 32736, -16368,      0,  16352,      0)}  // First order 10 Hz high-pass at 32730 Hz.
	},
	.filterHp47 = // I2S2.
	{
		{CDC_SWAP_BIQUAD( 32767,      0,      0,      0,      0)},
		{CDC_SWAP_BIQUAD( 32767,      0,      0,      0,      0)},
		{CDC_SWAP_BIQUAD( 32745, -16372,      0,  16361,      0)}  // First order 10 Hz high-pass at 47610 Hz.
	},
	.filterSp32 = // I2S1.
	{
		{CDC_SWAP_BIQUAD( 32767, -27535,  22413,  30870, -29096)}, // Customized peak filter with negative offset?
		{CDC_SWAP_BIQUAD(-14000,  30000, -14000,      0,      0)}, // Customized high shelf filter?
		{CDC_SWAP_BIQUAD( 32736, -16368,      0,  16352,      0)}  // First order 10 Hz high-pass at 32730 Hz.
	},
	.filterSp47 = // I2S2.
	{
		{CDC_SWAP_BIQUAD( 32767, -28995,  25277,  31456, -30200)}, // Customized peak filter with negative offset?
		{CDC_SWAP_BIQUAD(-14402,  30000, -14402,      0,      0)}, // Customized high shelf filter?
		{CDC_SWAP_BIQUAD( 32745, -16372,      0,  16361,      0)}  // First order 10 Hz high-pass at 47610 Hz.
	},
	.filterMic32 = // I2S1?
	{
		{CDC_SWAP_IIR( 32767,      0,      0)},
		{CDC_SWAP_BIQUAD( 32767,      0,      0,      0,      0)},
		{CDC_SWAP_BIQUAD( 32767,      0,      0,      0,      0)},
		{CDC_SWAP_BIQUAD( 32767,      0,      0,      0,      0)},
		{CDC_SWAP_BIQUAD( 32767,      0,      0,      0,      0)},
		{CDC_SWAP_BIQUAD( 32767,      0,      0,      0,      0)}
	},
	.filterMic47 = // I2S2?
	{
		{CDC_SWAP_IIR( 32767,      0,      0)},
		{CDC_SWAP_BIQUAD( 32767,       0,      0,      0,      0)},
		{CDC_SWAP_BIQUAD( 32767,       0,      0,      0,      0)},
		{CDC_SWAP_BIQUAD( 32767,       0,      0,      0,      0)},
		{CDC_SWAP_BIQUAD( 32767,       0,      0,      0,      0)},
		{CDC_SWAP_BIQUAD( 32767,       0,      0,      0,      0)}
	},
	.filterFree = // All I2S lines.
	{
		{CDC_SWAP_IIR( 32767,      0,      0)},
		{CDC_SWAP_BIQUAD(-12959,  -8785,  32767,   8785,  12959)}, // all-pass: Fc 2500 Hz, Q 0.1 at 32730(?) Hz.
		{CDC_SWAP_BIQUAD(-12959,  -8785,  32767,   8785,  12959)}, // all-pass: Fc 2500 Hz, Q 0.1 at 32730(?) Hz.
		{CDC_SWAP_BIQUAD(-12959,  -8785,  32767,   8785,  12959)}, // all-pass: Fc 2500 Hz, Q 0.1 at 32730(?) Hz.
		{CDC_SWAP_BIQUAD( 32767,      0,      0,      0,      0)},
		{CDC_SWAP_BIQUAD( 32767,      0,      0,      0,      0)}
	},
	.analogInterval  = 1,
	.analogStabilize = 9,
	.analogPrecharge = 4,
	.analogSense     = 3,
	.analogDebounce  = 0,
	.analogXpPullup  = 6,
	.ymDriver        = 1,
	.reserved        = 0
};

// These defaults are identical to launch day o3DS calibration.
BacklightPwmCalBase g_blPwmCal =
{
	.coeffs =
	{
		{0.00111639f, 1.41412f, 0.07178809f},  // Bottom LCD.
		{0.000418169f, 0.66567f, 0.06098654f}, // Top LCD in 2D mode.
		{0.00208543f, 1.55639f, 0.0385939f}    // Top LCD in 3D mode.
	},
	.numLevels        = 5,
	.unknown          = 0,
	.lumLevels        = {20, 43, 73, 95, 117, 172, 172},
	.hwBrightnessBase = 512,
	.hwBrightnessMin  = 13
};

static u32 g_calLoadedMask = 0;



// Based on code from cfg module plus an optimization idea eleminating 2 table lookups from here:
// https://github.com/LacobusVentura/MODBUS-CRC16/blob/master/MODBUS_CRC16.c#L131
static u16 reverseCrc16Modbus(const u16 init, const void *src, u32 size)
{
	u32 crc = init;
	const u8 *ptr8 = (const u8*)src;

	static const u16 crc16Table[16] =
	{
		0x0000, 0xCC01, 0xD801, 0x1400,
		0xF001, 0x3C00, 0x2800, 0xE401,
		0xA001, 0x6C00, 0x7800, 0xB401,
		0x5000, 0x9C01, 0x8801, 0x4400
	};

	do
	{
		const u32 data = *ptr8++;
		crc = (crc>>4) ^ crc16Table[(data ^ crc) & 15u];
		crc = (crc>>4) ^ crc16Table[((data>>4) ^ crc) & 15u];
	} while(--size > 0);

	return crc;
}

static void endianSwapCodecFilter(u16 *filt, u32 entries)
{
	do
	{
		const u16 tmp = __builtin_bswap16(*filt);
		*filt++ = tmp;
	} while(--entries > 0);
}

static void updateCals(const Hwcal *const hwcal)
{
	// Check and update CODEC calibration.
	const u16 agingPassedMask = hwcal->agingPassedMask;
	u32 calLoadedMask = 0;
	if(agingPassedMask & CAL_MASK_CODEC)
	{
		const u16 crc16 = reverseCrc16Modbus(0x55AA, &hwcal->codec, CAL_CRC_OFFSET(hwcal->codec));
		if(hwcal->codec.crc16 == crc16)
		{
			// First copy data raw and then endian swap filters.
			// Stored in little endian in HWCAL but CODEC wants them in big endian.
			memcpy(&g_cdcCal, &hwcal->codec, sizeof(g_cdcCal));
			endianSwapCodecFilter((u16*)&g_cdcCal.filterHp32, sizeof(g_cdcCal.filterHp32) / 2);
			endianSwapCodecFilter((u16*)&g_cdcCal.filterHp47, sizeof(g_cdcCal.filterHp47) / 2);
			endianSwapCodecFilter((u16*)&g_cdcCal.filterSp32, sizeof(g_cdcCal.filterSp32) / 2);
			endianSwapCodecFilter((u16*)&g_cdcCal.filterSp47, sizeof(g_cdcCal.filterSp47) / 2);
			endianSwapCodecFilter((u16*)&g_cdcCal.filterMic32, sizeof(g_cdcCal.filterMic32) / 2);
			endianSwapCodecFilter((u16*)&g_cdcCal.filterMic47, sizeof(g_cdcCal.filterMic47) / 2);
			endianSwapCodecFilter((u16*)&g_cdcCal.filterFree, sizeof(g_cdcCal.filterFree) / 2);

			calLoadedMask = CAL_MASK_CODEC;
		}
	}

	// Check and update backlight PWM calibration.
	if(agingPassedMask & CAL_MASK_BACKLIGHT_PWM)
	{
		const u16 crc16 = reverseCrc16Modbus(0x55AA, &hwcal->blPwn, CAL_CRC_OFFSET(hwcal->blPwn));
		if(hwcal->blPwn.crc16 == crc16)
		{
			memcpy(&g_blPwmCal, &hwcal->blPwn, sizeof(g_blPwmCal));
			calLoadedMask |= CAL_MASK_BACKLIGHT_PWM;
		}
	}

	// Keep track of successfully updated calibrations.
	g_calLoadedMask = calLoadedMask;
}

// TODO: If we dump HWCAL to SD give the file an unique name in case the SD is shared between consoles.
Result HWCAL_load(void)
{
	// Try to open the HWCAL file.
	FHandle f;
	Result res = fOpen(&f, "sdmc:/3ds/HWCAL.dat", FA_OPEN_EXISTING | FA_READ);
	if(res != RES_OK) return res;

	// Allocate memory for the file.
	Hwcal *const hwcal = malloc(sizeof(Hwcal));
	if(hwcal == NULL)
	{
		fClose(f);
		return RES_OUT_OF_MEM;
	}

	// Load the whole file.
	u32 bytesRead;
	res = fRead(f, hwcal, sizeof(Hwcal), &bytesRead);
	if(res != RES_OK)
	{
		fClose(f);
		free(hwcal);
		return res;
	}
	if(bytesRead != sizeof(Hwcal))
	{
		fClose(f);
		free(hwcal);
		return RES_OUT_OF_RANGE;
	}

	fClose(f);

	// Check HWCAL magic.
	if(hwcal->magic != HWCAL_MAGIC)
	{
		free(hwcal);
		return RES_INVALID_ARG;
	}

	// Check and load all calibration data.
	updateCals(hwcal);

	free(hwcal);

	return res;
}

u32 HWCAL_getLoadedMask(void)
{
	return g_calLoadedMask;
}