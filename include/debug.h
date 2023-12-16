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



#ifdef __cplusplus
extern "C"
{
#endif

[[noreturn]] void panicMsg(const char *const msg);
[[noreturn]] void panic(void);

#ifdef __ARM11__
[[noreturn]] void arm9FatalError(const u32 type);
#endif // #ifdef __ARM11__

// Exception tests.
/*static inline void __regTest(void)
{
	__asm__ volatile("mov r0, #1\n\t"
	                 "mov r1, #2\n\t"
	                 "mov r2, #3\n\t"
	                 "mov r3, #4\n\t"
	                 "mov r4, #5\n\t"
	                 "mov r5, #6\n\t"
	                 "mov r6, #7\n\t"
	                 "mov r7, #8\n\t"
	                 "mov r8, #9\n\t"
	                 "mov r9, #10\n\t"
	                 "mov r10, #11\n\t"
	                 "mov r11, #12\n\t"
	                 "mov r12, #13\n\t"
	                 "mov r13, #14\n\t"
	                 "mov r14, #15\n\t"
	                 "mov r15, #16" : : : "memory");
}

static inline void __breakpointTest(void)
{
	__asm__ volatile("bkpt #0xCAFE" : : : "memory");
}

static inline void __dataAbortTest(void)
{
	__asm__ volatile("mov r0, #4\n\t"
	                 "mov r1, #0xEF\n\t"
	                 "str r1, [r0]" : : : "memory");
}

static inline void __undefInstrTest(void)
{
	__asm__ volatile("udf #0xDEAD" : : : "memory");
}

static inline void __stackTest(void)
{
	__asm__ volatile("mov r0, #0\n\t"
	                 "sub sp, #384\n\t"
	                 "mov r1, sp\n\t"
	                 "1: str r0, [r1], #4\n\t"
	                 "cmp r0, #384 - 4\n\t"
	                 "add r0, #4\n\t"
	                 "bne 1b\n\t"
	                 "str r0, [r0]" : : : "memory"); // Trigger data abort.
}*/

#ifdef __cplusplus
} // extern "C"
#endif