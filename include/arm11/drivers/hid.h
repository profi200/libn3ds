#pragma once

/*
 *   This file is part of libn3ds
 *   Copyright (C) 2024 derrek, profi200
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


#ifdef __cplusplus
extern "C"
{
#endif

// Based on code from https://github.com/smealum/ctrulib

#define HID_REGS_BASE   (IO_COMMON_BASE + 0x46000)
#define REG_HID_PAD     (*((vu16*)(HID_REGS_BASE + 0x0)) ^ 0xFFFu)
#define REG_HID_PADCNT  *((vu16*)(HID_REGS_BASE + 0x2))


enum
{
	KEY_A            = BIT(0),  // A
	KEY_B            = BIT(1),  // B
	KEY_SELECT       = BIT(2),  // Select
	KEY_START        = BIT(3),  // Start
	KEY_DRIGHT       = BIT(4),  // D-Pad Right
	KEY_DLEFT        = BIT(5),  // D-Pad Left
	KEY_DUP          = BIT(6),  // D-Pad Up
	KEY_DDOWN        = BIT(7),  // D-Pad Down
	KEY_R            = BIT(8),  // R
	KEY_L            = BIT(9),  // L
	KEY_X            = BIT(10), // X
	KEY_Y            = BIT(11), // Y
	KEY_ZL           = BIT(14), // ZL (New 3DS only)
	KEY_ZR           = BIT(15), // ZR (New 3DS only)
	KEY_TOUCH        = BIT(20), // Touch (Not actually provided by HID)
	KEY_CSTICK_RIGHT = BIT(24), // C-Stick Right (New 3DS only)
	KEY_CSTICK_LEFT  = BIT(25), // C-Stick Left (New 3DS only)
	KEY_CSTICK_UP    = BIT(26), // C-Stick Up (New 3DS only)
	KEY_CSTICK_DOWN  = BIT(27), // C-Stick Down (New 3DS only)
	KEY_CPAD_RIGHT   = BIT(28), // Circle Pad Right
	KEY_CPAD_LEFT    = BIT(29), // Circle Pad Left
	KEY_CPAD_UP      = BIT(30), // Circle Pad Up
	KEY_CPAD_DOWN    = BIT(31), // Circle Pad Down

	// Generic catch-all directions
	KEY_UP    = KEY_DUP    | KEY_CPAD_UP,    // D-Pad Up or Circle Pad Up
	KEY_DOWN  = KEY_DDOWN  | KEY_CPAD_DOWN,  // D-Pad Down or Circle Pad Down
	KEY_LEFT  = KEY_DLEFT  | KEY_CPAD_LEFT,  // D-Pad Left or Circle Pad Left
	KEY_RIGHT = KEY_DRIGHT | KEY_CPAD_RIGHT, // D-Pad Right or Circle Pad Right

	// Masks
	KEY_DPAD_MASK   = KEY_DDOWN       | KEY_DUP       | KEY_DLEFT       | KEY_DRIGHT,
	KEY_CSTICK_MASK = KEY_CSTICK_DOWN | KEY_CSTICK_UP | KEY_CSTICK_LEFT | KEY_CSTICK_RIGHT,
	KEY_CPAD_MASK   = KEY_CPAD_DOWN   | KEY_CPAD_UP   | KEY_CPAD_LEFT   | KEY_CPAD_RIGHT
};

// Extra keys use with hidGetExtraKeys()
enum
{
	KEY_POWER        = BIT(0),
	KEY_POWER_HELD   = BIT(1),
	KEY_HOME         = BIT(2), // Auto clears on release
	KEY_WIFI         = BIT(3),
	KEY_SHELL        = BIT(4), // Auto clears on open
	KEY_BAT_CHARGING = BIT(5), // Auto clears when charging stops
	KEY_VOL_SLIDER   = BIT(6)
};

typedef struct
{
	u16 x;
	u16 y;
} TouchPos;

typedef struct
{
	s16 x;
	s16 y;
} CpadPos;



void hidInit(void); // For libn3ds internal usage only.
void hidScanInput(void);
u32 hidKeysHeld(void);
u32 hidKeysDown(void);
u32 hidKeysUp(void);
const TouchPos* hidGetTouchPosPtr(void);
const CpadPos* hidGetCpadPosPtr(void);
u32 hidGetExtraKeys(u32 clearMask);

#ifdef __cplusplus
} // extern "C"
#endif