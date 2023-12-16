#pragma once

/*
 *   This file is part of open_agb_firm
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

#include "types.h"
#include "arm11/drivers/gx.h"


#ifdef __cplusplus
extern "C"
{
#endif

#define NUM_PDC_PRESETS     (4u)
#define PDC_PRESET_IDX_BOT  (3u)


// Note: Offsets according to display controller registers, not struct offsets.
typedef struct
{
	u32 h_total;       // 0x00
	u32 h_start;       // 0x04
	u32 h_border;      // 0x08
	u32 h_blank;       // 0x0C
	u32 h_sync;        // 0x10
	u32 h_back_porch;  // 0x14
	u32 h_left_border; // 0x18
	u32 h_irq_range;   // 0x1C
	u32 h_dma_pos;     // 0x20

	u32 v_total;       // 0x24
	u32 v_start;       // 0x28
	u32 v_border;      // 0x2C
	u32 v_blank;       // 0x30
	u32 v_sync;        // 0x34
	u32 v_back_porch;  // 0x38
	u32 v_top_border;  // 0x3C
	u32 v_irq_range;   // 0x40
	u32 v_incr_h_pos;  // 0x44

	u32 signal_pol;    // 0x48
	u32 border_color;  // 0x4C
	u32 pic_dim;       // 0x5C
	u32 pic_border_h;  // 0x60
	u32 pic_border_v;  // 0x64
	//u32 fb_stride;     // 0x90
	u32 latch_pos;     // 0x9C
} PdcPreset;
static_assert(offsetof(PdcPreset, latch_pos) == 0x5C, "Error: Member latch_pos of PdcPreset is not at offset 0x5C!");

static const PdcPreset g_pdcPresets[NUM_PDC_PRESETS] =
{
	{ // Top screen in 2D mode 240x400 (pixel doubling).
		.h_total       = 450,
		.h_start       = 209,
		.h_border      = 449,
		.h_blank       = 449,
		.h_sync        = 0,
		.h_back_porch  = 207,
		.h_left_border = 209,
		.h_irq_range   = PDC_RANGE(449, 453),
		.h_dma_pos     = PDC_RANGE(0, 1),

		.v_total       = 413,
		.v_start       = 2,
		.v_border      = 402,
		.v_blank       = 402,
		.v_sync        = 402,
		.v_back_porch  = 1,
		.v_top_border  = 2,
		.v_irq_range   = PDC_RANGE(402, 406),
		.v_incr_h_pos  = 0,

		.signal_pol    = PDC_SIGNAL_POL_V_ACT_LO | PDC_SIGNAL_POL_H_ACT_LO,
		.border_color  = PDC_COLOR_RGB(0, 255, 0),
		.pic_dim       = PDC_RANGE(240, 400), // Not actually a range.
		.pic_border_h  = PDC_RANGE(209, 449),
		.pic_border_v  = PDC_RANGE(2, 402),
		//.fb_stride     = 960, // 240 * 4 (RGBA8).
		.latch_pos     = PDC_RANGE(0, 0)
	},
	{ // Top screen in 2D mode 240x800.
		.h_total       = 450,
		.h_start       = 209,
		.h_border      = 449,
		.h_blank       = 449,
		.h_sync        = 0,
		.h_back_porch  = 207,
		.h_left_border = 209,
		.h_irq_range   = PDC_RANGE(449, 453),
		.h_dma_pos     = PDC_RANGE(0, 1),

		.v_total       = 827,
		.v_start       = 2,
		.v_border      = 802,
		.v_blank       = 802,
		.v_sync        = 802,
		.v_back_porch  = 1,
		.v_top_border  = 2,
		.v_irq_range   = PDC_RANGE(802, 806),
		.v_incr_h_pos  = 0,

		.signal_pol    = PDC_SIGNAL_POL_V_ACT_LO | PDC_SIGNAL_POL_H_ACT_LO,
		.border_color  = PDC_COLOR_RGB(255, 0, 0),
		.pic_dim       = PDC_RANGE(240, 800), // Not actually a range.
		.pic_border_h  = PDC_RANGE(209, 449),
		.pic_border_v  = PDC_RANGE(2, 802),
		//.fb_stride     = 960, // 240 * 4 (RGBA8).
		.latch_pos     = PDC_RANGE(0, 0)
	},
	{ // Top screen in 3D mode (stereo 240x400).
		.h_total       = 450,
		.h_start       = 209,
		.h_border      = 449,
		.h_blank       = 449,
		.h_sync        = 0,
		.h_back_porch  = 207,
		.h_left_border = 209,
		.h_irq_range   = PDC_RANGE(449, 453),
		.h_dma_pos     = PDC_RANGE(0, 1),

		.v_total       = 827,
		.v_start       = 2,
		.v_border      = 802,
		.v_blank       = 802,
		.v_sync        = 802,
		.v_back_porch  = 1,
		.v_top_border  = 2,
		.v_irq_range   = PDC_RANGE(802, 806),
		.v_incr_h_pos  = 0,

		.signal_pol    = PDC_SIGNAL_POL_V_ACT_LO | PDC_SIGNAL_POL_H_ACT_LO,
		.border_color  = 0xff000000, // Invalid border color but that's what gsp uses...
		.pic_dim       = PDC_RANGE(240, 400), // Not actually a range.
		.pic_border_h  = PDC_RANGE(209, 449),
		.pic_border_v  = PDC_RANGE(2, 802),
		//.fb_stride     = 960, // 240 * 4 (RGBA8).
		.latch_pos     = PDC_RANGE(0, 0)
	},
	{ // Bottom screen 240x320.
		.h_total       = 450,
		.h_start       = 209,
		.h_border      = 449,
		.h_blank       = 449,
		.h_sync        = 205,
		.h_back_porch  = 207,
		.h_left_border = 209,
		.h_irq_range   = PDC_RANGE(449, 453),
		.h_dma_pos     = PDC_RANGE(0, 1),

		.v_total       = 413,
		.v_start       = 82,
		.v_border      = 402,
		.v_blank       = 402,
		.v_sync        = 79,
		.v_back_porch  = 80,
		.v_top_border  = 82,
		.v_irq_range   = PDC_RANGE(404, 408), // HOS apps: 404/408. gsp: 403/407.
		.v_incr_h_pos  = 0,

		.signal_pol    = PDC_SIGNAL_POL_V_ACT_HI | PDC_SIGNAL_POL_H_ACT_HI,
		.border_color  = PDC_COLOR_RGB(255, 0, 0),
		.pic_dim       = PDC_RANGE(240, 320), // Not actually a range.
		.pic_border_h  = PDC_RANGE(209, 449),
		.pic_border_v  = PDC_RANGE(82, 402),
		//.fb_stride     = 960, // 240 * 4 (RGBA8).
		.latch_pos     = PDC_RANGE(0, 0)
	}
};

#ifdef __cplusplus
} // extern "C"
#endif