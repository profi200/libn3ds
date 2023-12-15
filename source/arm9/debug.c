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

#include <string.h>
#include "types.h"
#include "drivers/pxi.h"
#include "ipc_handler.h"
#include "arm9/drivers/interrupt.h"



// We are violating our IPC protocol sending data silently
// but this is ok because for exceptions we have no other choice.
static inline void sendRawPxiWord(const u32 data)
{
	Pxi *const pxi = getPxiRegs();
	while(pxi->cnt & PXI_CNT_SEND_FULL);
	pxi->send = data;
}

static inline void sendSyncRequest(void)
{
	Pxi *const pxi = getPxiRegs();
	pxi->sync_irq = PXI_SYNC_IRQ_IRQ_EN | PXI_SYNC_IRQ_IRQ;
}

static void sendFatalIpcCmd(const u32 param)
{
	sendRawPxiWord(IPC_CMD11_A9_FATAL);
	sendSyncRequest();
	sendRawPxiWord(param);
}

// Speed is not important. We need to be able to send data
// even from unaligned addresses which is why this reads in bytes.
static void sendRawPxiData(const void *data, u32 size)
{
	const u8 *ptr8 = data;

	while(size > 0)
	{
		const u32 blockSize = (size > 4 ? 4 : size);
		u32 data = 0;
		for(u32 i = 0; i < blockSize; i++)
		{
			// Construct little endian words from the data.
			data |= ((u32)*ptr8++)<<(i * 8);
		}
		sendRawPxiWord(data);

		size -= blockSize;
	}
}

static void prepareExceptionHandling(void)
{
	// Disable interrupts. Only required for assert()/panicMsg()/panic().
	__setCpsr_c(__getCpsr() | PSR_I);

	// Block any attempts to run exception handling more than once.
	// Relaxed load/store + signal fence prevents atomic library calls.
	// Good enough for the single core ARM9.
	static atomic_bool exceptionLock = false;
	if(atomic_load_explicit(&exceptionLock, memory_order_relaxed))
	{
		while(1) __wfi();
	}
	atomic_store_explicit(&exceptionLock, true, memory_order_relaxed);
	atomic_signal_fence(memory_order_acquire);
}

[[noreturn]] NOINLINE void __fb_assert(const char *const file, const unsigned line, const char *const cond)
{
	prepareExceptionHandling();

	// Transfer fatal type and file string.
	sendFatalIpcCmd(0);
	if(file != NULL)
	{
		sendRawPxiData(file, strlen(file) + 1);
	}
	else
	{
		// Nothing to send.
		sendRawPxiWord(0);
	}

	// Transfer line number and failed condition string.
	sendRawPxiWord(line);
	if(cond != NULL)
	{
		sendRawPxiData(cond, strlen(cond) + 1);
	}
	else
	{
		// Nothing to send.
		sendRawPxiWord(0);
	}

	// Wait for power off.
	while(1) __wfi();
}

[[noreturn]] NOINLINE void panicMsg(const char *const msg)
{
	prepareExceptionHandling();

	// Transfer fatal type and panic string.
	sendFatalIpcCmd(1);
	if(msg != NULL)
	{
		sendRawPxiData(msg, strlen(msg) + 1);
	}
	else
	{
		// Nothing to send.
		sendRawPxiWord(0);
	}

	// Wait for power off.
	while(1) __wfi();
}

[[noreturn]] NOINLINE void panic(void)
{
	panicMsg(NULL);
}

// Expects the registers on the exception stack to be in the following order:
// r0-r14, pc (unmodified), cpsr.
[[noreturn]] NOINLINE void guruMeditation(const u32 type, const u32 *excFrame)
{
	prepareExceptionHandling();

	// Fatal type exception. Exception type in bit 8-15.
	sendFatalIpcCmd(type<<8 | 2);

	// Transfer register dump.
	sendRawPxiData(excFrame, 4 * (16 + 1)); // r0-r15 and CPSR.

	// Transfer stack dump.
	const u32 sp = excFrame[13];
	if(sp >= DTCM_BASE && sp < DTCM_BASE + DTCM_SIZE && (sp % 4) == 0) // TODO: Allow any valid memory region.
	{
		u32 stackWords = (DTCM_BASE + DTCM_SIZE - sp) / 4;
		stackWords = (stackWords > 96 ? 96 : stackWords);
		sendRawPxiWord(stackWords);
		for(u32 i = 0; i < stackWords; i++)
		{
			sendRawPxiWord(((u32*)sp)[i]);
		}
	}
	else
	{
		// No stack data to send.
		sendRawPxiWord(0);
	}

	// Wait for power off.
	while(1) __wfi();
}
// ---------------------------------------------------------------- //

#ifndef NDEBUG
// Needs to be marked as used to work with LTO.
// The used attribute also overrides the newlib symbol.
// This is for debugging purposes only. For security this value needs to be random!
__attribute__((used)) uintptr_t __stack_chk_guard = 0x8F303A48;

// Needs to be marked as noinline and used to work with LTO.
// The used attribute also overrides the newlib symbol.
// Combine -fstack-protector-all with -fno-inline to get the most effective detection.
[[noreturn]] __attribute__((noinline, used)) void __stack_chk_fail(void)
{
	panicMsg("Stack smash!");
}


// Add "-Wl,-wrap=malloc,-wrap=calloc,-wrap=free" to LDFLAGS to enable the heap check.
static const u32 __heap_chk_guard[4] = {0x2241FCFC, 0x29EBBE29, 0x93EDFD9B, 0x141EF827};

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
	// 1024u * 1024 is roughly ok for ARM9 mem.
	if(size > (1024u * 1024) || memcmp(ptr8 + size, __heap_chk_guard, guardSize) != 0)
		panicMsg("Heap overflow!");

	__real_free(ptr8 - guardSize);
}
#endif