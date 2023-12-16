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



#ifdef __cplusplus
extern "C"
{
#endif

// ARM7 vector overlay for GBA mode.
[[noreturn]] void _gba_vector_overlay(void);
extern const u32 _gba_vector_overlay_size[];

// ARM7 boot code for GBA mode.
[[noreturn]] void _gba_boot(void);
extern u8 _gba_boot_swi_a9_addr[]; // "swi 0x01" location in ARM9 address space.
extern const u32 _gba_boot_size[];

#ifdef __cplusplus
} // extern "C"
#endif