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

#ifdef NDEBUG
#define fb_assert(c) ((void)0)
#else
// #define fb_assert(c) ((c) ? ((void)0) : __fb_assert(#c ", " __FILE__, __LINE__))
#define fb_assert(c) ((c) ? ((void)0) : __fb_assert(__FILE__, __LINE__, #c))
#endif



// Moved to debug.c. TODO: Print function and condition.
[[noreturn]] void __fb_assert(const char *const file, const unsigned line, const char *const cond);

#ifdef __cplusplus
} // extern "C"
#endif