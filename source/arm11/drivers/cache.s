@ This file is part of libn3ds
@ Copyright (C) 2024 derrek, profi200
@
@ This program is free software: you can redistribute it and/or modify
@ it under the terms of the GNU General Public License as published by
@ the Free Software Foundation, either version 3 of the License, or
@ (at your option) any later version.
@
@ This program is distributed in the hope that it will be useful,
@ but WITHOUT ANY WARRANTY; without even the implied warranty of
@ MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
@ GNU General Public License for more details.
@
@ You should have received a copy of the GNU General Public License
@ along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "asm_macros.h"

.syntax unified
.cpu mpcore
.fpu vfpv2


.equ I_CACHE_SIZE,       0x4000
.equ D_CACHE_SIZE,       0x4000
.equ L2_CACHE_SIZE,      0x200000
.equ CACHE_LINE_SIZE,    32

@ If a cache range operation is >= threshold do it on the whole cache.
@ Whole cache invalidate is almost always faster (single instruction)
@ but it's very hard to predict how badly this affects system performance
@ so these are rough guesses at which point whole cache is overall cheaper.
@ Flush may need adjustment too.
.equ I_INVAL_THRESHOLD,  0x6800
.equ D_INVAL_THRESHOLD,  I_INVAL_THRESHOLD
.equ CLEAN_THRESHOLD,    D_CACHE_SIZE
.equ FLUSH_THRESHOLD,    D_CACHE_SIZE
.equ L2_INVAL_THRESHOLD, 0x700000
.equ L2_CLEAN_THRESHOLD, 0x700000
.equ L2_FLUSH_THRESHOLD, 0x700000



BEGIN_ASM_FUNC invalidateICache
	mov r0, #0
	mcr p15, 0, r0, c7, c5, 0  @ Invalidate Entire Instruction Cache, also flushes the branch target cache.
	@mcr p15, 0, r0, c7, c5, 6  @ Flush Entire Branch Target Cache.
	mcr p15, 0, r0, c7, c10, 4 @ Data Synchronization Barrier.
	mcr p15, 0, r0, c7, c5, 4  @ Flush Prefetch Buffer.
	bx  lr
END_ASM_FUNC

BEGIN_ASM_FUNC invalidateICacheRange
	cmp r1, #I_INVAL_THRESHOLD
	bhs invalidateICache

	add r1, r1, r0
	bic r0, r0, #CACHE_LINE_SIZE - 1
	mov r2, #0
	invalidateICacheRange_lp:
		mcr p15, 0, r0, c7, c5, 1    @ Invalidate Instruction Cache Line (using MVA).
		add r0, r0, #CACHE_LINE_SIZE
		cmp r0, r1
		blo invalidateICacheRange_lp

	mcr p15, 0, r2, c7, c5, 6        @ Flush Entire Branch Target Cache.
	mcr p15, 0, r2, c7, c10, 4       @ Data Synchronization Barrier.
	mcr p15, 0, r2, c7, c5, 4        @ Flush Prefetch Buffer.
	bx  lr
END_ASM_FUNC

BEGIN_ASM_FUNC invalidateDCache
	mov r0, #0
	mcr p15, 0, r0, c7, c6, 0  @ Invalidate Entire Data Cache.
	mcr p15, 0, r0, c7, c10, 4 @ Data Synchronization Barrier.
	bx  lr
END_ASM_FUNC

BEGIN_ASM_FUNC invalidateDCacheRange
	cmp   r1, #D_INVAL_THRESHOLD
	bhs   flushDCache

	tst   r0, #CACHE_LINE_SIZE - 1
	add   r1, r1, r0
	mcrne p15, 0, r0, c7, c10, 1       @ Clean Data Cache Line (using MVA).

	tst   r1, #CACHE_LINE_SIZE - 1
	bic   r0, r0, #CACHE_LINE_SIZE - 1
	mcrne p15, 0, r1, c7, c10, 1       @ Clean Data Cache Line (using MVA).

	mov   r2, #0
	invalidateDCacheRange_lp:
		mcr   p15, 0, r0, c7, c6, 1    @ Invalidate Data Cache Line (using MVA).
		add   r0, r0, #CACHE_LINE_SIZE
		cmp   r0, r1
		blo   invalidateDCacheRange_lp

	mcr   p15, 0, r2, c7, c10, 4       @ Data Synchronization Barrier.
	bx    lr
END_ASM_FUNC

BEGIN_ASM_FUNC invalidateBothCaches
	mov r0, #0
	mcr p15, 0, r0, c7, c7, 0  @ Invalidate Both Caches. Also flushes the branch target cache.
	mcr p15, 0, r0, c7, c10, 4 @ Data Synchronization Barrier.
	mcr p15, 0, r0, c7, c5, 4  @ Flush Prefetch Buffer.
	bx  lr
END_ASM_FUNC

BEGIN_ASM_FUNC cleanDCache
	mov r0, #0
	mcr p15, 0, r0, c7, c10, 0 @ Clean Entire Data Cache.
	mcr p15, 0, r0, c7, c10, 4 @ Data Synchronization Barrier.
	bx  lr
END_ASM_FUNC

BEGIN_ASM_FUNC cleanDCacheRange
	cmp r1, #CLEAN_THRESHOLD
	bhs cleanDCache

	add r1, r1, r0
	bic r0, r0, #CACHE_LINE_SIZE - 1
	mov r2, #0
	cleanDCacheRange_lp:
		mcr p15, 0, r0, c7, c10, 1   @ Clean Data Cache Line (using MVA).
		add r0, r0, #CACHE_LINE_SIZE
		cmp r0, r1
		blo cleanDCacheRange_lp

	mcr p15, 0, r2, c7, c10, 4       @ Data Synchronization Barrier.
	bx  lr
END_ASM_FUNC

BEGIN_ASM_FUNC flushDCache
	mov r0, #0
	mcr p15, 0, r0, c7, c14, 0 @ Clean and Invalidate Entire Data Cache.
	mcr p15, 0, r0, c7, c10, 4 @ Data Synchronization Barrier.
	bx  lr
END_ASM_FUNC

BEGIN_ASM_FUNC flushDCacheRange
	cmp r1, #FLUSH_THRESHOLD
	bhs flushDCache

	add r1, r1, r0
	bic r0, r0, #CACHE_LINE_SIZE - 1
	mov r2, #0
	flushDCacheRange_lp:
		mcr p15, 0, r0, c7, c14, 1   @ Clean and Invalidate Data Cache Line (using MVA).
		add r0, r0, #CACHE_LINE_SIZE
		cmp r0, r1
		blo flushDCacheRange_lp

	mcr p15, 0, r2, c7, c10, 4       @ Data Synchronization Barrier.
	bx  lr
END_ASM_FUNC