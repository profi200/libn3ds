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
.cpu arm946e-s
.fpu softvfp


.equ I_CACHE_SIZE,      0x2000
.equ D_CACHE_SIZE,      0x1000
.equ CACHE_LINE_SIZE,   32

@ If a cache range operation is >= threshold do it on the whole cache.
@ Whole cache invalidate is almost always faster (single instruction)
@ but it's very hard to predict how badly this affects system performance
@ so these are rough guesses at which point whole cache is overall cheaper.
@ Flush may need adjustment too.
.equ I_INVAL_THRESHOLD, 0x4800
.equ D_INVAL_THRESHOLD, 0x3800
.equ CLEAN_THRESHOLD,   D_CACHE_SIZE
.equ FLUSH_THRESHOLD,   D_CACHE_SIZE



BEGIN_ASM_FUNC invalidateICache
	mov r0, #0
	mcr p15, 0, r0, c7, c5, 0 @ Flush instruction cache.
	bx  lr
END_ASM_FUNC

BEGIN_ASM_FUNC invalidateICacheRange
	cmp r1, #I_INVAL_THRESHOLD
	bhs invalidateICache

	add r1, r1, r0
	bic r0, r0, #CACHE_LINE_SIZE - 1
	invalidateICacheRange_lp:
		mcr p15, 0, r0, c7, c5, 1    @ Flush instruction cache single entry (address).
		add r0, r0, #CACHE_LINE_SIZE
		cmp r0, r1
		blo invalidateICacheRange_lp

	bx  lr
END_ASM_FUNC

BEGIN_ASM_FUNC invalidateDCache
	mov r0, #0
	mcr p15, 0, r0, c7, c6, 0 @ Flush data cache.
	bx  lr
END_ASM_FUNC

BEGIN_ASM_FUNC invalidateDCacheRange
	cmp   r1, #D_INVAL_THRESHOLD
	bhs   flushDCache

	tst   r0, #CACHE_LINE_SIZE - 1
	add   r1, r1, r0
	mcrne p15, 0, r0, c7, c10, 1       @ Clean data cache entry (address).

	tst   r1, #CACHE_LINE_SIZE - 1
	bic   r0, r0, #CACHE_LINE_SIZE - 1
	mcrne p15, 0, r1, c7, c10, 1       @ Clean data cache entry (address).

	mov   r2, #0
	invalidateDCacheRange_lp:
		mcr   p15, 0, r0, c7, c6, 1    @ Flush data cache single entry (address).
		add   r0, r0, #CACHE_LINE_SIZE
		cmp   r0, r1
		blo   invalidateDCacheRange_lp

	mcr   p15, 0, r2, c7, c10, 4       @ Drain write buffer.
	bx    lr
END_ASM_FUNC

BEGIN_ASM_FUNC invalidateBothCaches
	mov r0, #0
	mcr p15, 0, r0, c7, c5, 0 @ Flush instruction cache.
	mcr p15, 0, r0, c7, c6, 0 @ Flush data cache.
	bx  lr
END_ASM_FUNC

BEGIN_ASM_FUNC cleanDCache
	mov  r0, #0
	mov  r1, #D_CACHE_SIZE / 4
	rsb  r1, r1, #0x40000000
	cleanDCache_idx_seg_lp:
		mcr  p15, 0, r0, c7, c10, 2   @ Clean data cache entry (index and segment).
		add  r0, r0, #CACHE_LINE_SIZE @ Increment index.
		tst  r0, #D_CACHE_SIZE / 4
		beq  cleanDCache_idx_seg_lp

		@ Increment segment.
		adds r0, r0, r1
		bne  cleanDCache_idx_seg_lp

	mcr  p15, 0, r0, c7, c10, 4       @ Drain write buffer.
	bx   lr
END_ASM_FUNC

BEGIN_ASM_FUNC cleanDCacheRange
	cmp r1, #CLEAN_THRESHOLD
	bhs cleanDCache

	add r1, r1, r0
	bic r0, r0, #CACHE_LINE_SIZE - 1
	mov r2, #0
	cleanDCacheRange_lp:
		mcr p15, 0, r0, c7, c10, 1   @ Clean data cache entry (address).
		add r0, r0, #CACHE_LINE_SIZE
		cmp r0, r1
		blo cleanDCacheRange_lp

	mcr p15, 0, r2, c7, c10, 4       @ Drain write buffer.
	bx  lr
END_ASM_FUNC

BEGIN_ASM_FUNC flushDCache
	mov  r0, #0
	mov  r1, #D_CACHE_SIZE / 4
	rsb  r1, r1, #0x40000000
	flushDCache_idx_seg_lp:
		mcr  p15, 0, r0, c7, c14, 2   @ Clean and flush data cache entry (index and segment).
		add  r0, r0, #CACHE_LINE_SIZE @ Increment index.
		tst  r0, #D_CACHE_SIZE / 4
		beq  flushDCache_idx_seg_lp

		@ Increment segment.
		adds r0, r0, r1
		bne  flushDCache_idx_seg_lp

	mcr  p15, 0, r0, c7, c10, 4       @ Drain write buffer.
	bx   lr
END_ASM_FUNC

BEGIN_ASM_FUNC flushDCacheRange
	cmp r1, #FLUSH_THRESHOLD
	bhs flushDCache

	add r1, r1, r0
	bic r0, r0, #CACHE_LINE_SIZE - 1
	mov r2, #0
	flushDCacheRange_lp:
		mcr p15, 0, r0, c7, c14, 1   @ Clean and flush data cache entry (address).
		add r0, r0, #CACHE_LINE_SIZE
		cmp r0, r1
		blo flushDCacheRange_lp

	mcr p15, 0, r2, c7, c10, 4       @ Drain write buffer.
	bx  lr
END_ASM_FUNC