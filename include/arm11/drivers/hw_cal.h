#pragma once

/*
 *   This file is part of libn3ds
 *   Copyright (C) 2023 luigoalma, profi200
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

#include <stddef.h>
#include "types.h"
#include "error_codes.h"


#define HWCAL_MAGIC                (0x4C414343u) // "CCAL"
#define CAL_CRC_OFFSET(cal_type)   offsetof(typeof(cal_type), crc16)

// Bitmasks for agingPassedMask field.
// Note: agingPassedMask is u16 so bits beyond 15 are not supported.
#define CAL_MASK_RTC_COMPENSATION  (1u<<0)
#define CAL_MASK_LCD_FLICKER       (1u<<1)
#define CAL_MASK_OUTER_CAMS1       (1u<<2)
#define CAL_MASK_TOUCH             (1u<<3)
#define CAL_MASK_CIRCLE_PAD1       (1u<<4)
#define CAL_MASK_CODEC             (1u<<5)
#define CAL_MASK_GYRO              (1u<<6)
#define CAL_MASK_RTC_CORRECTION    (1u<<7)
#define CAL_MASK_ACCELEROMETER     (1u<<8)
#define CAL_MASK_SURROUND_SOUND    (1u<<9)
#define CAL_MASK_ABL               (1u<<10) // LCD power save.
#define CAL_MASK_LCD_3D            (1u<<11) // LCD stereoscopic.
#define CAL_MASK_BACKLIGHT_PWM     (1u<<12)
#define CAL_MASK_CIRCLE_PAD2       (1u<<13)
#define CAL_MASK_OUTER_CAMS2       (1u<<14)
#define CAL_MASK_ABL_LGY           (1u<<15) // LCD power save legacy.

#define CAL_MASK_MCU_SLIDERS       (1u<<16)
#define CAL_MASK_ULCD_DELAY        (1u<<17)
#define CAL_MASK_MIC_ECHO_CANCEL   (1u<<18)
#define CAL_MASK_C_STICK           (1u<<19)
#define CAL_MASK_UNUSED            (1u<<20)
#define CAL_MASK_NEW_ABL           (1u<<21) // New LCD power save.
#define CAL_MASK_PIT               (1u<<22)
#define CAL_MASK_QTM               (1u<<23)


// Touchscreen calibration.
// ---------------------------------------------------------------- //
typedef struct
{
	u16 rawX0;
	u16 rawY0;
	u16 pointX0;
	u16 pointY0;
	u16 rawX1;
	u16 rawY1;
	u16 pointX1;
	u16 pointY1;
} TouchCalBase;
static_assert(sizeof(TouchCalBase) == 0x10);
static_assert(offsetof(TouchCalBase, pointY1) == 0xE);

typedef struct
{
	TouchCalBase base;
	u16 crc16;         // CRC16 init 0x55AA.
	u8 padding[2];
} TouchCal;
static_assert(sizeof(TouchCal) == 0x14);
static_assert(offsetof(TouchCal, padding[1]) == 0x13);

// Circle-Pad calibration 1.
// ---------------------------------------------------------------- //
typedef struct
{
	s16 centerX;
	s16 centerY;
	// "reserved" excluded to save space.
} CirclePadCal1Base;
static_assert(sizeof(CirclePadCal1Base) == 4);
static_assert(offsetof(CirclePadCal1Base, centerY) == 2);

typedef struct
{
	CirclePadCal1Base base;
	u8 reserved[4];
	u16 crc16;              // CRC16 init 0x55AA.
	u8 padding[2];
} CirclePadCal1;
static_assert(sizeof(CirclePadCal1) == 0xC);
static_assert(offsetof(CirclePadCal1, padding[1]) == 0xB);

// LCD flicker calibration.
// ---------------------------------------------------------------- //
typedef struct
{
	u8 vcomTop;
	u8 vcomBottom;
} LcdFlickerCalBase;
static_assert(sizeof(LcdFlickerCalBase) == 2);
static_assert(offsetof(LcdFlickerCalBase, vcomBottom) == 1);

typedef struct
{
	LcdFlickerCalBase base;
	u8 flippedBytes[2];
} LcdFlickerCal;
static_assert(sizeof(LcdFlickerCal) == 4);
static_assert(offsetof(LcdFlickerCal, flippedBytes[1]) == 3);

// RTC compensation calibration.
// ---------------------------------------------------------------- //
typedef struct
{
	u8 compensationValue;
} RtcCompensationCalBase;
static_assert(sizeof(RtcCompensationCalBase) == 1);

typedef struct
{
	RtcCompensationCalBase base;
	u8 flippedBytes[1];
	u8 padding[2];
} RtcCompensationCal;
static_assert(sizeof(RtcCompensationCal) == 4);
static_assert(offsetof(RtcCompensationCal, padding[1]) == 3);

// RTC correction calibration.
// ---------------------------------------------------------------- //
typedef struct
{
	u8 correctionValue;
} RtcCorrectionCalBase;
static_assert(sizeof(RtcCorrectionCalBase) == 1);

typedef struct
{
	RtcCorrectionCalBase base;
	u8 flippedBytes[1];
	u8 padding[6];
} RtcCorrectionCal;
static_assert(sizeof(RtcCorrectionCal) == 8);
static_assert(offsetof(RtcCorrectionCal, padding[5]) == 7);

// Outer camera calibration 1.
// ---------------------------------------------------------------- //
typedef struct
{
	u32 flags;
	float scale;
	float rotationZ;
	float translationX;
	float translationY;
	float rotationX;
	float rotationY;
	float viewAngleRight;
	float viewAngleLeft;
	float chartDistance;
	float cameraDistance;
	s16 imageWidth;
	s16 imageHeight;
} OuterCamStruct1;
static_assert(sizeof(OuterCamStruct1) == 0x30);
static_assert(offsetof(OuterCamStruct1, imageHeight) == 0x2E);

typedef struct
{
	s16 aeBaseTarget;
	s16 kRL;
	s16 kGL;
	s16 kBL;
	s16 ccmPosition;
} OuterCamStruct2;
static_assert(sizeof(OuterCamStruct2) == 10);
static_assert(offsetof(OuterCamStruct2, ccmPosition) == 8);

typedef struct
{
	OuterCamStruct1 base;
	u8 reserved[16];
	u8 reserved2[64];
	OuterCamStruct2 base2;
	u16 crc16;             // CRC16 init 0x55AA.
} OuterCamCal1;
static_assert(sizeof(OuterCamCal1) == 0x8C);
static_assert(offsetof(OuterCamCal1, crc16) == 0x8A);

// Gyroscope calibration.
// ---------------------------------------------------------------- //
typedef struct
{
	s16 zeroX;
	s16 plusX;
	s16 minusX;
	s16 zeroY;
	s16 plusY;
	s16 minusY;
	s16 zeroZ;
	s16 plusZ;
	s16 minusZ;
} GyroscopeCalBase;
static_assert(sizeof(GyroscopeCalBase) == 0x12);
static_assert(offsetof(GyroscopeCalBase, minusZ) == 0x10);

typedef struct
{
	GyroscopeCalBase base;
	u16 crc16;             // CRC16 init 0x55AA.
} GyroscopeCal;
static_assert(sizeof(GyroscopeCal) == 0x14);
static_assert(offsetof(GyroscopeCal, crc16) == 0x12);

// Accelerometer calibration.
// ---------------------------------------------------------------- //
typedef struct
{
	s16 offsetX;
	s16 scaleX;
	s16 offsetY;
	s16 scaleY;
	s16 offsetZ;
	s16 scaleZ;
} AccelerometerCalBase;
static_assert(sizeof(AccelerometerCalBase) == 0xC);
static_assert(offsetof(AccelerometerCalBase, scaleZ) == 0xA);

typedef struct
{
	AccelerometerCalBase base;
	u16 crc16;                 // CRC16 init 0x55AA.
	u8 padding[2];
} AccelerometerCal;
static_assert(sizeof(AccelerometerCal) == 0x10);
static_assert(offsetof(AccelerometerCal, padding[1]) == 0xF);

// CODEC calibration.
// ---------------------------------------------------------------- //
#define CDC_SWAP_IIR(b0, b1, a1)             __builtin_bswap16(b0),__builtin_bswap16(b1),__builtin_bswap16(a1)
#define CDC_SWAP_BIQUAD(b0, b1, b2, a1, a2)  __builtin_bswap16(b0),__builtin_bswap16(b1),__builtin_bswap16(b2),__builtin_bswap16(a1),__builtin_bswap16(a2)

// All coefficients are 16 bit fixed-point numbers.
// 1 sign bit (bit 15) and 15 fraction bits. Big endian.
// All filters are IIR filters.
// Note: All names based on TSC2117 datasheet/CTRAging/guesses.
//       They may not match 100%.

// First order IIR. Or half a biquad.
typedef struct
{
	s16 b0;
	s16 b1;
	// a0 always 1.0.
	s16 a1; // Inverted.
} CdcIir;
static_assert(sizeof(CdcIir) == 6);
static_assert(offsetof(CdcIir, a1) == 4);

// Full IIR biquad filter.
typedef struct
{
	s16 b0;
	s16 b1; // Halved.
	s16 b2;
	// a0 always 1.0.
	s16 a1; // Halved and inverted.
	s16 a2; // Inverted.
} CdcBiquad;
static_assert(sizeof(CdcBiquad) == 10);
static_assert(offsetof(CdcBiquad, a2) == 8);

// Equalizer filters.
typedef struct
{
	CdcBiquad a; // Not in TSC2117 datasheet. Name is a guess.
	CdcBiquad b; // Not in TSC2117 datasheet. Name is a guess.
	CdcBiquad c; // Not in TSC2117 datasheet. Name is a guess.
} CdcEqFilters;

// PRB_P25 filters.
typedef struct
{
	CdcIir iir;  // First order IIR filter.
	CdcBiquad b; // Biquad filter B.
	CdcBiquad c; // Biquad filter C.
	CdcBiquad d; // Biquad filter D.
	CdcBiquad e; // Biquad filter E.
	CdcBiquad f; // Biquad filter F.
} CdcPrbP25Filters;
static_assert(sizeof(CdcPrbP25Filters) == 0x38);
static_assert(offsetof(CdcPrbP25Filters, f.a2) == 0x36);

typedef struct
{
	u8 driverGainHp;
	u8 driverGainSp;
	u8 analogVolumeHp;
	u8 analogVolumeSp;
	s8 shutterVolumeI2s1;
	s8 shutterVolumeI2s2;
	u8 microphoneBias;
	u8 quickCharge;               // Microphone related.
	u8 pgaGain;                   // Microphone gain.
	u8 padding[3];
	CdcEqFilters filterHp32;
	CdcEqFilters filterHp47;
	CdcEqFilters filterSp32;
	CdcEqFilters filterSp47;
	CdcPrbP25Filters filterMic32; // Note: Does not perfectly match TSC2117 datasheet.
	CdcPrbP25Filters filterMic47; // Note: Does not perfectly match TSC2117 datasheet.
	CdcPrbP25Filters filterFree;

	// Analog calibration.
	u8 analogInterval;
	u8 analogStabilize;
	u8 analogPrecharge;
	u8 analogSense;
	u8 analogDebounce;
	u8 analogXpPullup;
	u8 ymDriver;
	u8 reserved;
} CodecCalBase;
static_assert(sizeof(CodecCalBase) == 0x134);
static_assert(offsetof(CodecCalBase, reserved) == 0x133);

typedef struct
{
	CodecCalBase base;
	u16 crc16;         // CRC16 init 0x55AA.
	u8 padding[2];
} CodecCal;
static_assert(sizeof(CodecCal) == 0x138);
static_assert(offsetof(CodecCal, padding[1]) == 0x137);

// PIT calibration.
// ---------------------------------------------------------------- //
typedef struct
{
	u16 visibleFactor; // float16?
	u16 irFactor;      // float16?
} PitCalBase;
static_assert(sizeof(PitCalBase) == 4);
static_assert(offsetof(PitCalBase, irFactor) == 2);

typedef struct
{
	PitCalBase base;
	u16 agingFlag;
	u16 crc16;       // CRC16 init 0x55AA.
} PitCal;
static_assert(sizeof(PitCal) == 8);
static_assert(offsetof(PitCal, crc16) == 6);

// Surround sound calibration.
// ---------------------------------------------------------------- //
typedef struct
{
	s16 specialFilter[256];
	s32 iirSurroundFilter[5];
} SurroundSoundCalBase;
static_assert(sizeof(SurroundSoundCalBase) == 0x214);
static_assert(offsetof(SurroundSoundCalBase, iirSurroundFilter[4]) == 0x210);

typedef struct
{
	SurroundSoundCalBase base;
	u16 crc16;                 // CRC16 init 0x55AA.
	u8 padding[10];
} SurroundSoundCal;
static_assert(sizeof(SurroundSoundCal) == 0x220);
static_assert(offsetof(SurroundSoundCal, padding[9]) == 0x21F);

// LCD power save (ABL) calibration 1.
// ---------------------------------------------------------------- //
typedef struct
{
	u32 ditherPattern;
	u16 startX;
	u16 startY;
	u16 sizeX;
	u16 sizeY;
	u16 gthRatio;
	u8 ditherMode;
	u8 minRs;
	u8 maxRs;
	u8 minGth;
	u8 minMax;
	u8 exMax;
	u8 inertia;
	u8 lutListRs[9];
	u8 reserved[2];
} LcdPowerSaveCalBase;
static_assert(sizeof(LcdPowerSaveCalBase) == 32);
static_assert(offsetof(LcdPowerSaveCalBase, reserved[1]) == 31);

typedef struct
{
	LcdPowerSaveCalBase base;
	u16 crc16;                // CRC16 init 0x55AA.
	u8 padding[14];
} LcdPowerSaveCal;
static_assert(sizeof(LcdPowerSaveCal) == 0x30);
static_assert(offsetof(LcdPowerSaveCal, padding) == 34);

// LCD stereoscopic (3D) calibration.
// ---------------------------------------------------------------- //
// New 3DS stable 3D calibration??
typedef struct
{
	float pupillaryDistanceInMm;
	float distanceEyesAndUpperLcdInMm;
	float lcdWidthInMm;
	float lcdHeightInMm;
	float unknown[4];
} LcdStereoscopicCalBase;
static_assert(sizeof(LcdStereoscopicCalBase) == 0x20);
static_assert(offsetof(LcdStereoscopicCalBase, unknown[3]) == 0x1C);

typedef struct
{
	LcdStereoscopicCalBase base;
	u16 crc16;                   // CRC16 init 0x55AA.
	u8 padding[14];
} LcdStereoscopicCal;
static_assert(sizeof(LcdStereoscopicCal) == 0x30);
static_assert(offsetof(LcdStereoscopicCal, padding[13]) == 0x2F);

// LCD backlight PWM calibration.
// ---------------------------------------------------------------- //
typedef struct
{
	float coeffs[3][3];   // Polynomial coefficients. [LCD/mode]{A, B, C}.
	u8 numLevels;         // Number of luminance levels excluding 0.
	u8 unknown;           // TODO: Is numLevels actually u16?
	u16 lumLevels[7];     // Luminance levels as used in HOME menu.
	u16 hwBrightnessBase; // Alternatively hwBrightnessMax. Maximum hardware brightness.
	u16 hwBrightnessMin;  // Minimum hardware brightness.
} BacklightPwmCalBase;
static_assert(sizeof(BacklightPwmCalBase) == 0x38);
static_assert(offsetof(BacklightPwmCalBase, hwBrightnessMin) == 0x36);

typedef struct
{
	BacklightPwmCalBase base;
	u16 crc16;                // CRC16 init 0x55AA.
	u8 padding[6];
} BacklightPwmCal;
static_assert(sizeof(BacklightPwmCal) == 0x40);
static_assert(offsetof(BacklightPwmCal, padding[5]) == 0x3F);

// Circle-Pad calibration 2.
// ---------------------------------------------------------------- //
typedef struct
{
	float scaleX;
	float scaleY;
	s16 maxX;
	s16 minX;
	s16 maxY;
	s16 minY;
	s16 type;
	u8 reserved[2]; // Needed for alignment anyway. Part of the reserved array in CirclePadCal2.
} CirclePadCal2Base;
static_assert(sizeof(CirclePadCal2Base) == 0x14);
static_assert(offsetof(CirclePadCal2Base, reserved[1]) == 0x13);

typedef struct
{
	CirclePadCal2Base base;
	u8 reserved[4];
	u16 crc16;              // CRC16 init 0x55AA.
	u8 padding[6];
} CirclePadCal2;
static_assert(sizeof(CirclePadCal2) == 0x20);
static_assert(offsetof(CirclePadCal2, padding[5]) == 0x1F);

// Outer camera calibration 2.
// ---------------------------------------------------------------- //
typedef struct
{
	u16 unknown[6];
} OuterCamCal2Base;
static_assert(sizeof(OuterCamCal2Base) == 0xC);
static_assert(offsetof(OuterCamCal2Base, unknown[5]) == 0xA);

typedef struct
{
	OuterCamCal2Base base;
	u16 crc16;             // CRC16 init 0x55AA.
	u8 padding[2];
} OuterCamCal2;
static_assert(sizeof(OuterCamCal2) == 0x10);
static_assert(offsetof(OuterCamCal2, padding[1]) == 0xF);

// MCU 3D and volume slider calibration.
// ---------------------------------------------------------------- //
typedef struct
{
	s16 min;
	s16 max;
} McuSliderBounds;
static_assert(sizeof(McuSliderBounds) == 4);
static_assert(offsetof(McuSliderBounds, max) == 2);

typedef struct
{
	McuSliderBounds _3d;
	McuSliderBounds vol;
	u16 agingFlag;
	u16 crc16;           // CRC16 init 0x55AA.
	u8 padding[4];
} McuSliderCal;
static_assert(sizeof(McuSliderCal) == 0x10);
static_assert(offsetof(McuSliderCal, padding[3]) == 0xF);

// ULCD (2D <--> 3D) delay calibration.
// ---------------------------------------------------------------- //
typedef struct
{
	s8 to2d;
	s8 to3d;
} ULcdDelayCalBase;
static_assert(sizeof(ULcdDelayCalBase) == 2);
static_assert(offsetof(ULcdDelayCalBase, to3d) == 1);

typedef struct
{
	ULcdDelayCalBase base;
	u16 agingFlag;
	u16 crc16;             // CRC16 init 0x55AA.
	u8 padding[10];
} ULcdDelayCal;
static_assert(sizeof(ULcdDelayCal) == 0x10);
static_assert(offsetof(ULcdDelayCal, padding[9]) == 0xF);

// Microphone echo cancellation calibration.
// ---------------------------------------------------------------- //
typedef struct
{
	u8 params[8];
} MicEchoCancelCalBase;
static_assert(sizeof(MicEchoCancelCalBase) == 8);
static_assert(offsetof(MicEchoCancelCalBase, params[7]) == 7);

typedef struct
{
	MicEchoCancelCalBase base;
	u16 agingFlag;
	u16 crc16;                 // CRC16 init 0x55AA.
	u8 padding[4];
} MicEchoCancelCal;
static_assert(sizeof(MicEchoCancelCal) == 0x10);
static_assert(offsetof(MicEchoCancelCal, padding[3]) == 0xF);

// LCD power save (ABL) calibration for New 2DS/3DS.
// ---------------------------------------------------------------- //
typedef struct
{
	u8 maxInertia;
	u8 pad;
	u16 pwmCntEx;
	u32 histogram1;
	u32 histogram2;
	u32 adjust[64];
} NewLcdPowerSaveCalBase;
static_assert(sizeof(NewLcdPowerSaveCalBase) == 0x10C);
static_assert(offsetof(NewLcdPowerSaveCalBase, adjust[63]) == 0x108);

typedef struct
{
	NewLcdPowerSaveCalBase base;
	u16 agingFlag;
	u16 crc16;                   // CRC16 init 0x55AA.
} NewLcdPowerSaveCal;
static_assert(sizeof(NewLcdPowerSaveCal) == 0x110);
static_assert(offsetof(NewLcdPowerSaveCal, crc16) == 0x10E);

// C-Stick calibration. TODO: Is this actually Circle-Pad Pro?
// ---------------------------------------------------------------- //
typedef struct
{
	s16 centerX; // TODO: Confirm this.
	s16 centerY; // TODO: Confirm this.
} CStickCalBase;
static_assert(sizeof(CStickCalBase) == 4);
static_assert(offsetof(CStickCalBase, centerY) == 2);

typedef struct
{
	CStickCalBase base;
	u8 reserved[4];
	u16 agingFlag;
	u16 crc16;          // CRC16 init 0x55AA.
	u8 padding[4];
} CStickCal;
static_assert(sizeof(CStickCal) == 0x10);
static_assert(offsetof(CStickCal, padding[3]) == 0xF);

// QTM (head tracking) calibration.
// ---------------------------------------------------------------- //
typedef struct
{
	float divisorAtZero;
	float translationX;
	float translationY;
	float rotationZ;
	float horizontalAngle;
	float optimalDistance;
} QtmCalBase;
static_assert(sizeof(QtmCalBase) == 0x18);
static_assert(offsetof(QtmCalBase, optimalDistance) == 0x14);

typedef struct
{
	QtmCalBase base;
	u16 agingFlag;
	u16 crc16;       // CRC16 init 0x55AA.
	u8 padding[4];
} QtmCal;
static_assert(sizeof(QtmCal) == 0x20);
static_assert(offsetof(QtmCal, padding[3]) == 0x1F);

// Hardware calibration header.
// ---------------------------------------------------------------- //
typedef struct
{
	u32 magic;
	u32 version;
	u32 bodySize;
	u8 modelVersion;       // ?
	u8 revision;
	u16 agingPassedMask;   // Mask of passed aging tests.
	union
	{
		u8 sha256[32];     // Dev units.
		u8 hmacSha256[32]; // Retail units.
	};
} HwcalHeader;
static_assert(sizeof(HwcalHeader) == 0x30);
static_assert(offsetof(HwcalHeader, hmacSha256[31]) == 0x2F);

// Hardware calibration body.
// ---------------------------------------------------------------- //
typedef struct
{
	TouchCal           touch;
	CirclePadCal1      circlePad1;
	LcdFlickerCal      lcdFlicker;
	RtcCompensationCal rtcCompensation;
	RtcCorrectionCal   rtcCorrection;
	OuterCamCal1       outerCam1;
	GyroscopeCal       gyro;
	AccelerometerCal   accelerometer;
	CodecCal           codec;
	PitCal             pit;
	SurroundSoundCal   surroundSound;
	LcdPowerSaveCal    lcdPowersave;
	LcdStereoscopicCal lcdStereoscopic;
	BacklightPwmCal    blPwn;
	CirclePadCal2      circlePad2;
	OuterCamCal2       outerCam2;
	LcdPowerSaveCal    lcdPowersaveLgy;
	McuSliderCal       slider;
	ULcdDelayCal       lcdModeDelay;
	MicEchoCancelCal   micEchoCancel;
	NewLcdPowerSaveCal newLcdPowersave;
	CStickCal          cStick;
	QtmCal             qtm;
} HwcalBody;
static_assert(sizeof(HwcalBody) == 0x6B0);
static_assert(offsetof(HwcalBody, qtm.padding[3]) == 0x6AF);

// Whole hardware calibration with header.
// ---------------------------------------------------------------- //
typedef struct
{
	// Header.
	u32 magic;
	u32 version;
	u32 bodySize;
	u8 modelVersion;       // ?
	u8 revision;
	u16 agingPassedMask;   // Mask of passed aging tests.
	union
	{
		u8 sha256[32];     // Dev units.
		u8 hmacSha256[32]; // Retail units.
	};
	u8 padding[0x1D0];

	// Body.
	TouchCal           touch;
	CirclePadCal1      circlePad1;
	LcdFlickerCal      lcdFlicker;
	RtcCompensationCal rtcCompensation;
	RtcCorrectionCal   rtcCorrection;
	OuterCamCal1       outerCam1;
	GyroscopeCal       gyro;
	AccelerometerCal   accelerometer;
	CodecCal           codec;
	PitCal             pit;
	SurroundSoundCal   surroundSound;
	LcdPowerSaveCal    lcdPowersave;
	LcdStereoscopicCal lcdStereoscopic;
	BacklightPwmCal    blPwn;
	CirclePadCal2      circlePad2;
	OuterCamCal2       outerCam2;
	LcdPowerSaveCal    lcdPowersaveLgy;
	McuSliderCal       slider;
	ULcdDelayCal       lcdModeDelay;
	MicEchoCancelCal   micEchoCancel;
	NewLcdPowerSaveCal newLcdPowersave;
	CStickCal          cStick;
	QtmCal             qtm;
	u8 unused[0x120];
} Hwcal;
static_assert(sizeof(Hwcal) == 0x9D0);
static_assert(offsetof(Hwcal, unused[0x11F]) == 0x9CF);


extern CodecCalBase g_cdcCal;
extern BacklightPwmCalBase g_blPwmCal;


Result HWCAL_load(void);
u32 HWCAL_getLoadedMask(void);