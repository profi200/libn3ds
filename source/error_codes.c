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

#include "error_codes.h"



const char* result2String(Result res)
{
	static const char *const resultStrings[] =
	{
		// Common errors.
		"OK",
		"SD card removed",
		"Disk full",
		"Invalid argument",
		"Out of memory",
		"Out of range",
		"Not found",
		"Path too long",

		// FatFs errors.
		"FatFs disk error",
		"FatFs assertion failed",
		"FatFs disk not ready",
		"FatFs file not found",
		"FatFs path not found",
		"FatFs invalid path name",
		"FatFs access denied",
		"FatFs already exists",
		"FatFs invalid file/directory object",
		"FatFs drive write protected",
		"FatFs invalid drive",
		"FatFs drive not mounted",
		"FatFs no filesystem",
		"FatFs f_mkfs() aborted",
		"FatFs thread lock timeout",
		"FatFs file locked",
		"FatFs not enough memory",
		"FatFs too many open objects",
		"FatFs invalid parameter",

		// Lgy errors.
		"Invalid GBA RTC time/date"
	};

	return (res <= MAX_LIBN3DS_RES_VALUE ? resultStrings[res] : NULL);
}
