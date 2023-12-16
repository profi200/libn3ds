#pragma once

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

#include "types.h"


#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
{
	CDC_AUDIO_OUT_AUTO      = 0u,
	CDC_AUDIO_OUT_SPEAKER   = 1u,
	CDC_AUDIO_OUT_HEADPHONE = 2u
} CdcAudioOut;

typedef struct
{
	u16 touchX[5];
	u16 touchY[5];
	u16 cpadY[8];
	u16 cpadX[8];
} CdcAdcData;



/**
 * @brief      Initialize CODEC for Circle-Pad/Touchscreen/Sound.
 */
void CODEC_init(void);

/**
 * @brief      Deinitializes the CODEC chip for sleep or poweroff.
 */
void CODEC_deinit(void);

/**
 * @brief      The opposite of CODEC_deinit(). Does a partial init.
 */
void CODEC_wakeup(void);

/**
 * @brief      Poll and debounce headphone GPIO. Run this every ~16 ms.
 */
void CODEC_runHeadphoneDetection(void);

/**
 * @brief      Sets the audio output. Default is auto.
 *
 * @param[in]  output  The audio output to use.
 */
void CODEC_setAudioOutput(const CdcAudioOut output);

/**
 * @brief      Sets volume override.
 *
 * @param[in]  vol   The volume in 0.5 dB steps. -128 is muted.
 *                   -127 is minimum & maximum is 48 (-63.5 - +24 dB).
 *                   Anything above 48 means volume control via slider.
 *                   Recommended range is -128 - -20 to match the volume slider.
 */
void CODEC_setVolumeOverride(const s8 vol);

/**
 * @brief      Get raw ADC data for Circle-Pad/Touchscreen.
 *
 * @param      data  The output data pointer. Must be 4 bytes aligned.
 *
 * @return     Returns true if data was available and false otherwise.
 */
bool CODEC_getRawAdcData(CdcAdcData *data);

#ifdef __cplusplus
} // extern "C"
#endif