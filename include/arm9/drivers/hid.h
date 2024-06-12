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


#ifdef __cplusplus
extern "C"
{
#endif

// This code is part of ctrulib (https://github.com/devkitPro/libctru).

#define HID_KEY_MASK_ALL          ((KEY_Y << 1) - 1)
#define HID_VERBOSE_MODE_BUTTONS  (KEY_SELECT | KEY_START)

enum
{
	KEY_A       = BIT(0),
	KEY_B       = BIT(1),
	KEY_SELECT  = BIT(2),
	KEY_START   = BIT(3),
	KEY_DRIGHT  = BIT(4),
	KEY_DLEFT   = BIT(5),
	KEY_DUP     = BIT(6),
	KEY_DDOWN   = BIT(7),
	KEY_R       = BIT(8),
	KEY_L       = BIT(9),
	KEY_X       = BIT(10),
	KEY_Y       = BIT(11)
};



void hidScanInput(void);
u32 hidKeysHeld(void);
u32 hidKeysDown(void);
u32 hidKeysUp(void);

#ifdef __cplusplus
} // extern "C"
#endif