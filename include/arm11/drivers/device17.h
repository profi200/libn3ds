#pragma once

/*
 *   This file is part of open_agb_firm
 *   Copyright (C) 2024 EvansJahja
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
#ifdef __cplusplus
extern "C"
{
#endif

#include "types.h"

enum Device17_Button {
    ZR = (1<<1),
    ZL = (1<<2),
};

typedef struct
{
    u8 _status;
    enum Device17_Button button;
    s8 cstick_y_coarse;
    s8 cstick_x_coarse;
    u8 _ignored;
    s8 cstick_y_fine;
    s8 cstick_x_fine;

} Device17;

void Device17_Poll(void);

Device17* Device17_GetDevice(void);

#ifdef __cplusplus
} // extern "C"
#endif