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
#include "drivers/cache.h"
#include "arm11/drivers/mcu.h"
#include "arm11/drivers/hid.h"


#define LED_RGB8(r, g, b) ((b)<<16 | (g)<<8 | (r))



// We are violating our IPC protocol sending data silently
// but this is ok because for exceptions we have no other choice.
static inline u32 recvRawPxiWord(void)
{
	Pxi *const pxi = getPxiRegs();
	while(pxi->cnt & PXI_CNT_RECV_EMPTY);
	return pxi->recv;
}

static void prepareArm9ForPowerOff(void)
{
	Pxi *const pxi = getPxiRegs();
	while(pxi->cnt & PXI_CNT_SEND_FULL);
	pxi->send = IPC_CMD9_PREPARE_POWER;
	pxi->sync_irq = PXI_SYNC_IRQ_IRQ_EN | PXI_SYNC_IRQ_IRQ;
	while(recvRawPxiWord() != (IPC_CMD_RESP_FLAG | IPC_CMD9_PREPARE_POWER));
	// We don't care about the result.
}

static void recvRawPxiData(void *data, u32 size, const bool isString)
{
	u8 *ptr8 = data;

	while(size > 0)
	{
		const u32 blockSize = (size > 4 ? 4 : size);
		const u32 tmp = recvRawPxiWord();
		for(u32 i = 0; i < blockSize; i++)
		{
			*ptr8++ = tmp>>(i * 8);
		}

		// Words are zero padded so this lazy check will work.
		if(isString && ((tmp>>24) == 0)) break;

		size -= blockSize;
	}
}

// Maximum value for frameMs is 500.
// Make sure the last frame is 0%/off otherwise the LED stays on.
// Note: First pattern is skipped by the MCU due to a firmware bug.
static void exceptionInfoLedBlink(const u16 frameMs, const bool smooth, const u32 color, u32 patt)
{
	// Fixed point calculation is tested and works perfectly from 0-500 ms.
	const u32 frameTime = (4294968u * frameMs)>>23; // 512u * frameMs / 1000.

	// frameTime 0 is equal to 256 (wraps around).
	// lastRepeat is set to 255 to repeat the last frame forever (plays whole pattern once).
	InfoLedPattern p;
	p.frameTime  = frameTime & 0xFFu;
	p.rampTime   = (smooth ? (frameTime > 255 ? 255 : frameTime) : 0);
	p.lastRepeat = 255;
	p.unused     = 0;

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

static bool prepareExceptionHandling(const u32 fatalType, const u32 ledPattern)
{
	// Disable interrupts. Only required for assert()/panicMsg()/panic().
	__cpsid(i);

	// Block any attempts to run exception handling more than once.
	// This must be a strong atomic to account for multiple cores.
	static au32 exceptionLock = 0;
	if(atomic_exchange_explicit(&exceptionLock, 1, memory_order_acquire) != 0)
	{
		while(1) __wfi();
	}

	// Make the info LED blink for ~2 seconds in case this is a silent exception.
	static const u32 fatalColors[3] = {LED_RGB8(0, 128, 0), LED_RGB8(128, 128, 0), LED_RGB8(128, 0, 0)};
	exceptionInfoLedBlink(65, false, fatalColors[fatalType], ledPattern);

	// Setup frame buffer console for exceptions.
	const bool gfxReady = GFX_setupExceptionFrameBuffer();
	if(gfxReady) consoleInit(GFX_LCD_BOT, NULL);

	return gfxReady;
}

static void printException(const u32 excType, const u32 *const regs, const bool isArm9)
{
	// Check CPU mode. Thumb instructions are usually 2 bytes big.
	const u32 cpsr = regs[16];
	const u32 instSize = ((cpsr & PSR_T) != 0 ? 2 : 4);

	// Adjust pc to point at the actual fault instruction.
	// On data abort (exception type 2) pc is 2 instructions ahead.
	// On other exceptions pc is 1 instruction ahead.
	const u32 pc = regs[15];
	const u32 realPc = (excType == 2 ? pc - instSize * 2 : pc - instSize);

	// Print register dump. Fits exactly on bottom LCD width.
	ee_printf("r0: %08" PRIX32 " r4: %08" PRIX32 " r8:  %08" PRIX32 " r12: %08" PRIX32
	          "r1: %08" PRIX32 " r5: %08" PRIX32 " r9:  %08" PRIX32 " sp:  %08" PRIX32
	          "r2: %08" PRIX32 " r6: %08" PRIX32 " r10: %08" PRIX32 " lr:  %08" PRIX32
	          "r3: %08" PRIX32 " r7: %08" PRIX32 " r11: %08" PRIX32 " pc:  %08" PRIX32,
	          regs[0], regs[4], regs[8],  regs[12],
	          regs[1], regs[5], regs[9],  regs[13],
	          regs[2], regs[6], regs[10], regs[14],
	          regs[3], regs[7], regs[11], realPc);

	if(!isArm9)
	{
		// Print remaining regs.
		ee_printf("CPSR: %08" PRIX32 " DFSR: %08" PRIX32 " IFSR: %08" PRIX32 "\n"
		          "FAR: %08" PRIX32 " WFAR: %08" PRIX32 "\n\n",
		          cpsr, regs[17], regs[18],
		          regs[19], regs[20]);

		// Print stack dump.
		const u32 sp = regs[13];
		if(sp >= AXI_RAM_BASE && sp < AXI_RAM_BASE + AXI_RAM_SIZE && (sp % 4) == 0) // TODO: Allow any valid memory region.
		{
			u32 stackWords = (AXI_RAM_BASE + AXI_RAM_SIZE - sp) / 4;
			stackWords = (stackWords > 90 ? 90 : stackWords);
			for(u32 i = 0; i < stackWords; i++)
			{
				ee_printf("%08" PRIX32, ((u32*)sp)[i]);
				if((i % 6) < 5) ee_printf(" ");
			}
		}
	}
	else
	{
		// Print CPSR.
		ee_printf("CPSR: %08" PRIX32 "\n\n", cpsr);

		// Print stack dump.
		u32 stackWords = recvRawPxiWord();
		stackWords = (stackWords > 96 ? 96 : stackWords);
		for(u32 i = 0; i < stackWords; i++)
		{
			ee_printf("%08" PRIX32, recvRawPxiWord());
			if((i % 6) < 5) ee_printf(" ");
		}
	}
}

[[noreturn]] static void exceptionHandlerEnd(void)
{
	// Flush D-Cache just in case and also because the frame buffer may be cached.
	flushDCache();

	// Wait 2 seconds.
	// If we are on New 2/3DS (LGR2) the clock may be at 804 MHz.
	// If it's not at 804 MHz this will delay for 6 seconds.
	const u32 multiplier = (getCfg11Regs()->socinfo & SOCINFO_LGR2 ? 3 : 1);
	wait_cycles((268111856u * multiplier) * 2);

	// Wait for A/B/X/Y.
	while(!(REG_HID_PAD & (KEY_A | KEY_B | KEY_X | KEY_Y)));

	// Trigger power off via MCU.
	MCU_sysPowerOff();

	// Wait for power off.
	while(1) __wfi();
}

[[noreturn]] NOINLINE void __fb_assert(const char *const file, const unsigned line, const char *const cond)
{
	if(prepareExceptionHandling(0, 0x7FFFFFFF))
	{
		ee_printf("ARM11(%" PRIu32 ") assert() called\n\n", __getCpuId());
		ee_printf("%s:%u: Assertion '%s' failed.", file, line, cond);
	}

	prepareArm9ForPowerOff();
	exceptionHandlerEnd();
}

[[noreturn]] NOINLINE void panicMsg(const char *const msg)
{
	if(prepareExceptionHandling(1, 0x7FFFFFFF))
	{
		ee_printf("ARM11(%" PRIu32 ") panic() called\n\n", __getCpuId());
		if(msg != NULL) ee_puts(msg);
	}

	prepareArm9ForPowerOff();
	exceptionHandlerEnd();
}

[[noreturn]] NOINLINE void panic(void)
{
	panicMsg(NULL);
}

// Expects the registers on the exception stack to be in the following order:
// r0-r14, pc (unmodified), CPSR, DFSR, IFSR, FAR, WFAR.
[[noreturn]] NOINLINE void guruMeditation(const u32 type, const u32 *const excFrame)
{
	if(prepareExceptionHandling(2, 0x7FFFFFFF))
	{
		static const char *const excStrs[3] = {"undefined instruction", "prefetch abort", "data abort"};
		ee_printf("ARM11(%" PRIu32 ") exception %s\n\n", __getCpuId(), excStrs[type]);

		printException(type, excFrame, false);
	}

	prepareArm9ForPowerOff();
	exceptionHandlerEnd();
}

[[noreturn]] NOINLINE void arm9FatalError(const u32 type)
{
	const u32 fatalType = type & 3u;
	if(prepareExceptionHandling(fatalType, 0x7FE007FE))
	{
		const u32 excType = type>>8;
		static const char *const errors[3] = {"assert() called", "panic() called", "exception"};
		static const char *const excStrs[3] = {"undefined instruction", "prefetch abort", "data abort"};
		ee_printf("ARM9 %s %s\n\n", errors[fatalType], (fatalType == 2 ? excStrs[excType] : ""));

		if(fatalType == 0) // assert().
		{
			char assertStr[256];
			recvRawPxiData(assertStr, 255, true);
			assertStr[255] = '\0';
			ee_printf("%s:%" PRIu32 ": ", assertStr, recvRawPxiWord());

			recvRawPxiData(assertStr, 255, true);
			ee_printf("Assertion '%s' failed.", assertStr);
		}
		else if(fatalType == 1) // pnaic().
		{
			char panicStr[256];
			recvRawPxiData(panicStr, 255, true);
			panicStr[255] = '\0';

			if(*panicStr != '\0')
				ee_puts(panicStr);
		}
		else // fatalType >= 2. Exception.
		{
			u32 excFrame[17];
			recvRawPxiData(excFrame, sizeof(excFrame), false);

			// GCC for some reason assumes parts of the array can be uninitialized here when they are in fact not.
			// Beware if excFrame is pre-initialized with {0} it will replace the CPSR print with 0.
//#pragma GCC diagnostic push
//#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
			printException(excType, excFrame, true);
//#pragma GCC diagnostic pop
		}
	}

	exceptionHandlerEnd();
}
// ---------------------------------------------------------------- //

#ifndef NDEBUG
// Needs to be marked as used to work with LTO.
// The used attribute also overrides the newlib symbol.
// This is for debugging purposes only. For security this value needs to be random!
__attribute__((used)) uintptr_t __stack_chk_guard = 0x3393E532;

// Needs to be marked as noinline and used to work with LTO.
// The used attribute also overrides the newlib symbol.
// Combine -fstack-protector-all with -fno-inline to get the most effective detection.
[[noreturn]] __attribute__((noinline, used)) void __stack_chk_fail(void)
{
	panicMsg("Stack smash!");
}


// Add "-Wl,-wrap=malloc,-wrap=calloc,-wrap=free" to LDFLAGS to enable the heap check.
static const u32 __heap_chk_guard[4] = {0x8B98ADAA, 0xE98855D2, 0x3C482F85, 0x0F872B4C};

void* __real_malloc(size_t size);
void __real_free(void *ptr);

void* __wrap_malloc(size_t size)
{
	// Check if size + guard data overflows.
	size_t guardedSize;
	const size_t guardSize = sizeof(__heap_chk_guard);
	if(__builtin_add_overflow(size, guardSize * 2, &guardedSize)) return NULL;

	// Allocate memory large enough to hold size + guard data.
	u8 *const buf = __real_malloc(guardedSize);
	if(buf == NULL) return NULL;

	// Copy guard data at start and end of buffer.
	memcpy(buf, &size, sizeof(size_t));
	memcpy(buf + sizeof(size_t), (u8*)__heap_chk_guard + sizeof(size_t), guardSize - sizeof(size_t));
	memcpy(buf + guardSize + size, __heap_chk_guard, guardSize);

	return buf + guardSize;
}

void* __wrap_calloc(size_t num, size_t size)
{
	// Check if num * size overflows.
	size_t allocSize;
	if(__builtin_mul_overflow(num, size, &allocSize)) return NULL;

	// Allocate buffer.
	void *const buf = __wrap_malloc(allocSize);
	if(buf == NULL) return NULL;

	// Clear buffer.
	memset(buf, 0, allocSize);

	return buf;
}

void __wrap_free(void *ptr)
{
	// Check for NULL.
	u8 *const ptr8 = ptr;
	if(ptr8 == NULL) return;

	// Check if guard data at buffer start got overwritten.
	const size_t guardSize = sizeof(__heap_chk_guard);
	if(memcmp(ptr8 - (guardSize - sizeof(size_t)), (u8*)__heap_chk_guard + sizeof(size_t), guardSize - sizeof(size_t)) != 0)
		panicMsg("Heap underflow!");

	// Copy the stored real buffer size.
	size_t size;
	memcpy(&size, ptr8 - guardSize, sizeof(size_t));

	// Check if the buffer size makes sense.
	// Important! Adjust the size check if needed.
	// 1024u * 512 is roughly ok for AXIWRAM.
	if(size > (1024u * 512) || memcmp(ptr8 + size, __heap_chk_guard, guardSize) != 0)
		panicMsg("Heap overflow!");

	__real_free(ptr8 - guardSize);
}
#endif