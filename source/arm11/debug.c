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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "mem_map.h"
#include "arm11/drivers/i2c.h"
#include "arm11/drivers/cfg11.h"
#include "arm11/console.h"
#include "arm11/fmt.h"
#include "drivers/pxi.h"
#include "ipc_handler.h"
#include "arm11/drivers/interrupt.h"
#include "arm.h"
#include "util.h"
#include "arm11/drivers/mcu.h"
#include "arm11/drivers/hid.h"


#define LED_RGB8(r, g, b) ((b)<<16 | (g)<<8 | (r))



// Maximum value for frameMs is 500.
// Make sure the last frame is 0%/off otherwise the LED stays on.
static void exceptionInfoLedBlink(const u16 frameMs, const bool smooth, const u32 color, u32 patt)
{
	// This fixed point calculation is tested and works perfectly from 0-500 ms.
	const u32 frameTime = (4294968u * frameMs)>>23; // 512u * frameMs / 1000.

	// frameTime 0 is equal to 256 (wraps around).
	// lastRepeat is set to 255 to repeat the last frame forever (plays whole pattern once).
	InfoLedPattern p;
	p.frameTime  = frameTime & 0xFFu;
	p.rampTime   = (smooth ? (frameTime > 255 ? 255 : frameTime) : 0);
	p.lastRepeat = 255;
	p.unused     = 0;

	// Note: First pattern is skipped by the MCU due to a firmware bug.
	for(unsigned i = 0; i < 32; i++)
	{
		const u32 c = ((patt & 1u) == 1 ? color : 0u);
		p.r[i] = c;
		p.g[i] = c>>8;
		p.b[i] = c>>16;

		patt >>= 1;
	}

	// Since we don't know which state the CPU is in
	// we have to use the interrupt safe I2C function.
	I2C_writeRegArrayIntSafe(I2C_DEV_CTR_MCU, MCU_REG_INFO_LED, &p, sizeof(p));
}

static void delayFor2seconds(void)
{
	// If we are on New 2/3DS (LGR2) the clock may be at 804 MHz.
	// If it's not at 804 MHz this will delay for 6 seconds.
	const u32 multiplier = (getCfg11Regs()->socinfo & SOCINFO_LGR2 ? 3 : 1);
	wait_cycles((268111856u * multiplier) * 2);
}

[[noreturn]] NOINLINE void panic(void)
{
	// Disable interrupts.
	enterCriticalSection();

	// Make the info LED blink for ~2 seconds in case this is a silent panic.
	exceptionInfoLedBlink(65, false, LED_RGB8(128, 128, 0), 0x7FFFFFFF);

	consoleInit(GFX_LCD_BOT, NULL);
	ee_printf("\x1b[41m\x1b[0J\x1b[15C****PANIC!!!****\n");

	PXI_sendPanicCmd(IPC_CMD9_PREPARE_POWER);

	// Wait 2 seconds and then wait for A/B/X/Y.
	delayFor2seconds();
	while(!(REG_HID_PAD & (KEY_A | KEY_B | KEY_X | KEY_Y)));

	MCU_sysPowerOff();
	while(1) __wfi();
}

[[noreturn]] NOINLINE void panicMsg(const char *msg)
{
	// Disable interrupts.
	enterCriticalSection();

	// Make the info LED blink for ~2 seconds in case this is a silent panic.
	exceptionInfoLedBlink(65, false, LED_RGB8(128, 128, 0), 0x7FFFFFFF);

	consoleInit(GFX_LCD_BOT, NULL);
	ee_printf("\x1b[41m\x1b[0J\x1b[15C****PANIC!!!****\n\n");
	ee_printf("\nERROR MESSAGE:\n%s\n", msg);

	PXI_sendPanicCmd(IPC_CMD9_PREPARE_POWER);

	// Wait 2 seconds and then wait for A/B/X/Y.
	delayFor2seconds();
	while(!(REG_HID_PAD & (KEY_A | KEY_B | KEY_X | KEY_Y)));

	MCU_sysPowerOff();
	while(1) __wfi();
}

// Expects the registers in the exception stack to be in the following order:
// r0-r14, pc (unmodified), CPSR, DFSR, IFSR, FAR, WFAR.
[[noreturn]] NOINLINE void guruMeditation(u8 type, const u32 *excStack)
{
	// Make the info LED blink for ~2 seconds in case this is a silent exception.
	exceptionInfoLedBlink(65, false, LED_RGB8(128, 0, 0), 0x7FFFFFFF);

	const char *const typeStr[3] = {"Undefined instruction", "Prefetch abort", "Data abort"};
	u32 realPc, instSize = 4;

	consoleInit(GFX_LCD_BOT, NULL);

	if(excStack[16] & 0x20) instSize = 2;                 // Processor was in Thumb mode?
	if(type == 2) realPc = excStack[15] - (instSize * 2); // Data abort
	else realPc = excStack[15] - instSize;                // Other

	ee_printf("\x1b[41m\x1b[0J\x1b[15CGuru Meditation Error!\n\n%s:\n", typeStr[type]);
	ee_printf("CPSR: 0x%" PRIX32 " DFSR: 0x%" PRIX32 " IFSR: 0x%" PRIX32 "\n"
	       "r0 = 0x%08" PRIX32 " r6  = 0x%08" PRIX32 " r12 = 0x%08" PRIX32 "\n"
	       "r1 = 0x%08" PRIX32 " r7  = 0x%08" PRIX32 " sp  = 0x%08" PRIX32 "\n"
	       "r2 = 0x%08" PRIX32 " r8  = 0x%08" PRIX32 " lr  = 0x%08" PRIX32 "\n"
	       "r3 = 0x%08" PRIX32 " r9  = 0x%08" PRIX32 " pc  = 0x%08" PRIX32 "\n"
	       "r4 = 0x%08" PRIX32 " r10 = 0x%08" PRIX32 "\n"
	       "r5 = 0x%08" PRIX32 " r11 = 0x%08" PRIX32 "\n\n",
	       excStack[16], excStack[17], excStack[18],
	       excStack[0], excStack[6],  excStack[12],
	       excStack[1], excStack[7],  excStack[13],
	       excStack[2], excStack[8],  excStack[14],
	       excStack[3], excStack[9],  realPc,
	       excStack[4], excStack[10],
	       excStack[5], excStack[11]);

	ee_puts("Stack dump:");
	u32 sp = excStack[13];
	if(sp >= AXI_RAM_BASE && sp < AXI_RAM_BASE + AXI_RAM_SIZE && !(sp & 3u))
	{
		u32 stackWords = ((AXI_RAM_BASE + AXI_RAM_SIZE - sp) / 4 > 48 ? 48 : (AXI_RAM_BASE + AXI_RAM_SIZE - sp) / 4);

		u32 newlineCounter = 0;
		for(u32 i = 0; i < stackWords; i++)
		{
			if(newlineCounter == 4) {ee_printf("\n"); newlineCounter = 0;}
			ee_printf("0x%08" PRIX32 " ", ((u32*)sp)[i]);
			newlineCounter++;
		}
	}

	PXI_sendPanicCmd(IPC_CMD9_PREPARE_POWER);

	// Wait 2 seconds and then wait for A/B/X/Y.
	delayFor2seconds();
	while(!(REG_HID_PAD & (KEY_A | KEY_B | KEY_X | KEY_Y)));

	MCU_sysPowerOff();
	while(1) __wfi();
}

#ifndef NDEBUG
// Needs to be marked as used to work with LTO.
// The used attribute also overrides the newlib symbol.
// This is for debugging purposes only. For security this value needs to be random!
__attribute__((used)) uintptr_t __stack_chk_guard = 0xC724B66D;

// Needs to be marked as noinline and used to work with LTO.
// The used attribute also overrides the newlib symbol.
// Combine -fstack-protector-all with -fno-inline to get the most effective detection.
[[noreturn]] __attribute__((noinline, used)) void __stack_chk_fail(void)
{
	panicMsg("Stack smash!");
}


// Add "-Wl,-wrap=malloc,-wrap=calloc,-wrap=free" to LDFLAGS to enable the heap check.
static const u32 __heap_chk_guard[4] = {0x9240A724, 0x6A6594A0, 0x976F0392, 0xB3A669AB};

void* __real_malloc(size_t size);
void __real_free(void *ptr);

void* __wrap_malloc(size_t size)
 {
	void *const buf = __real_malloc(size + 32);
	if(buf == NULL) return NULL;

	memcpy(buf, &size, sizeof(size_t));
	memcpy(buf + sizeof(size_t), (u8*)__heap_chk_guard + sizeof(size_t), 16 - sizeof(size_t));
	memcpy(buf + 16 + size, __heap_chk_guard, 16);

	return buf + 16;
}

void* __wrap_calloc(size_t num, size_t size)
{
	void *const buf = __wrap_malloc(num * size);
	if(buf == NULL) return NULL;

	memset(buf, 0, num * size);

	return buf;
}

void __wrap_free(void *ptr)
{
	if(ptr == NULL) return;

	if(memcmp(ptr - (16 - sizeof(size_t)), (u8*)__heap_chk_guard + sizeof(size_t), 16 - sizeof(size_t)) != 0)
		panicMsg("Heap underflow!");
	size_t size;
	memcpy(&size, ptr - 16, sizeof(size_t));

	// Important! Adjust the size check if needed.
	// 1024u * 512 is roughly ok for AXIWRAM.
	if(size > (1024u * 512) || memcmp(ptr + size, __heap_chk_guard, 16) != 0)
		panicMsg("Heap overflow!");

	__real_free(ptr - 16);
}
#endif
