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

#include "types.h"
#include "mem_map.h"


#define CSND_REGS_BASE  (IO_COMMON_BASE + 0x3000)

typedef struct
{
	vu16 cnt;              // 0x00
	vu16 sr;               // 0x02 Sample rate.
	union
	{
		struct
		{
			vu16 vol_r;    // 0x04 Range 0-0x8000.
			vu16 vol_l;    // 0x06 Range 0-0x8000.
		};
		vu32 vol;          // 0x04 R and L combined.
	};
	union
	{
		struct
		{
			vu16 capvol_r; // 0x08 Unconfirmed. Range 0-0x8000.
			vu16 capvol_l; // 0x0A Unconfirmed. Range 0-0x8000.
		};
		vu32 capvol;       // 0x08 R and L combined.
	};
	vu32 st_addr;          // 0x0C Start address and playback position.
	vu32 size;             // 0x10 Size in bytes.
	vu32 lp_addr;          // 0x14 Loop restart address.
	vu32 st_adpcm;         // 0x18 Start IMA-ADPCM state.
	vu32 lp_adpcm;         // 0x1C Loop Restart IMA-ADPCM state.
} CsndCh;
static_assert(offsetof(CsndCh, lp_adpcm) == 0x1C, "Error: Member lp_adpcm of CsndCh is not at offset 0x1C!");

typedef struct
{
	vu16 cnt;   // 0x0
	u8 _0x2[2];
	vu16 sr;    // 0x4 Sample rate.
	u8 _0x6[2];
	vu32 size;  // 0x8 Capture length in bytes.
	vu32 addr;  // 0xC Address.
} CsndCap;
static_assert(offsetof(CsndCap, addr) == 0xC, "Error: Member addr of CsndCap is not at offset 0xC!");

typedef struct
{
	vu16 master_vol;    // 0x000 CSND master volume range 0-0x8000.
	vu16 cnt;           // 0x002 Global control.
	u8 _0x4[0xc];
	vu32 ch_fifo_stat;  // 0x010 Channel FIFO status bits? Each bit write 1 to clear.
	vu8  cap_fifo_stat; // 0x014 Capture FIFO status bits? Each bit write 1 to clear.
	u8 _0x15[0x3eb];
	CsndCh ch[32];      // 0x400 32 sound channels. PSG on channel 8-13 and noise 14-15.
	CsndCap cap[2];     // 0x800 2 capture units for right and left side.
} Csnd;
static_assert(offsetof(Csnd, cap[1].addr) == 0x81C, "Error: Member cap[1].addr of Csnd is not at offset 0x81C!");

ALWAYS_INLINE Csnd* getCsndRegs(void)
{
	return (Csnd*)CSND_REGS_BASE;
}

ALWAYS_INLINE CsndCh* getCsndChRegs(u8 ch)
{
	return &getCsndRegs()->ch[ch];
}

ALWAYS_INLINE CsndCap* getCsndCapRegs(u8 ch)
{
	return &getCsndRegs()->cap[ch];
}


// REG_CSND_CNT
// Settings for all channels (including capture).
#define CSND_CNT_MUTE          (1u)
// Bits 1-13 unused.
#define CSND_CNT_RS_FILTER_EN  (1u<<14) // Resample filter.
#define CSND_CNT_EN            (1u<<15)

// REG_CSND_CH_CNT
#define CSND_CH_DUTY(d)        (d)      // For PSG (channel 8-13) only. In 12.5% units. 0 = high/12.5%.
#define CSND_CH_LERP           (1u<<6)  // Linear interpolation.
#define CSND_CH_HOLD           (1u<<7)  // Hold last sample after one shot.
#define CSND_CH_RPT_MANUAL     (0u<<10)
#define CSND_CH_RPT_LOOP       (1u<<10)
#define CSND_CH_RPT_ONE_SHOT   (2u<<10)
#define CSND_CH_FMT_PCM8       (0u<<12) // Signed PCM8.
#define CSND_CH_FMT_PCM16      (1u<<12) // Signed PCM16 little endian.
#define CSND_CH_FMT_IMA_ADPCM  (2u<<12)
#define CSND_CH_FMT_PSG_NOISE  (3u<<12)
#define CSND_CH_PLAYING        (1u<<14)
#define CSND_CH_START          (1u<<15)

// REG_CSND_CAP_CNT
#define CSND_CAP_RPT_LOOP      (0u)
#define CSND_CAP_RPT_ONE_SHOT  (1u)
#define CSND_CAP_FMT_PCM16     (0u)     // Signed PCM16 little endian.
#define CSND_CAP_FMT_PCM8      (1u<<1)  // Signed PCM8.
#define CSND_CAP_UNK2          (1u<<2)
#define CSND_CAP_START         (1u<<15)


// Sample rate/frequency helpers.
#define CSND_SAMPLE_RATE(s)  (0x10000 - (67027964u / (s)))
#define CSND_PSG_FREQ(f)     (CSND_SAMPLE_RATE(32u * (f)))



/**
 * @brief      Initializes the CSND hardware.
 */
void CSND_init(void);

/**
 * @brief      Calculates the left and right volumes.
 *
 * @param[in]  vol  The volume. 0.0 - 1.0.
 * @param[in]  pan  Left/right channel pan. -1.0 - 1.0. 0.0 is center.
 *
 * @return     The volume pair needed for CSND_setupCh().
 */
static inline u32 CSND_calcVol(const float vol, const float pan)
{
	const u32 lvol = 32768 * vol * (1.f - pan > 1.f ? 1.f : 1.f - pan);
	const u32 rvol = 32768 * vol * (1.f + pan > 1.f ? 1.f : 1.f + pan);
	return lvol<<16 | rvol;
}

/**
 * @brief      Sets up a channel for sound playback (in paused state).
 *
 * @param[in]  ch      The sound channel. 0-31.
 * @param[in]  srFreq  The sample rate/frequency.
 * @param[in]  vol     The L/R volume pair.
 * @param[in]  data    The start address.
 * @param[in]  data2   The loop restart address.
 * @param[in]  size    The size.
 * @param[in]  flags   The flags.
 */
void CSND_setupCh(const u8 ch, const u16 srFreq, const u32 vol, const u32 *const data,
                  const u32 *const data2, const u32 size, const u16 flags);

/**
 * @brief      Sets the sample rate/frequency of a channel.
 *
 * @param[in]  ch      The sound channel. 0-31.
 * @param[in]  srFreq  The sample rate/frequency.
 */
static inline void CSND_setSrFreq(const u8 ch, const u16 srFreq)
{
	getCsndChRegs(ch)->sr = srFreq;
}

/**
 * @brief      Pauses or unpauses a sound channel.
 *
 * @param[in]  ch       The sound channel. 0-31.
 * @param[in]  playing  The play state.
 */
static inline void CSND_setChState(const u8 ch, const bool playing)
{
	CsndCh *const csndCh = getCsndChRegs(ch);
	csndCh->cnt = (csndCh->cnt & ~CSND_CH_PLAYING) | ((u16)playing<<14);
}

/**
 * @brief      Returns the current audio buffer position (address).
 *
 * @param[in]  ch    The sound channel. 0-31.
 *
 * @return     The playback position (address).
 */
static inline u32 CSND_getChPos(const u8 ch)
{
	return getCsndChRegs(ch)->st_addr;
}

/**
 * @brief      Stops a sound channel.
 *
 * @param[in]  ch    The sound channel. 0-31.
 */
static inline void CSND_stopCh(const u8 ch)
{
	getCsndChRegs(ch)->cnt = 0; // Stop.
}


/**
 * @brief      Captures the output of all left/right sound channels combined.
 *
 * @param[in]  ch     The capture side. 0 = right, 1 = left.
 * @param[in]  sr     The sample rate.
 * @param      data   The output address.
 * @param[in]  size   The size.
 * @param[in]  flags  The flags.
 */
void CSND_startCap(const u8 ch, const u16 sr, u32 *const data, const u32 size, const u16 flags);

/**
 * @brief      Returns the current capture buffer position (address).
 *
 * @param[in]  ch    The capture side. 0 = right, 1 = left.
 *
 * @return     The capture position (address).
 */
static inline u32 CSND_getCapPos(const u8 ch)
{
	return getCsndCapRegs(ch)->addr;
}

/**
 * @brief      Stops a capture channel.
 *
 * @param[in]  ch    The capture side. 0 = right, 1 = left.
 */
static inline void CSND_stopCap(const u8 ch)
{
	getCsndCapRegs(ch)->cnt = 0;
}
