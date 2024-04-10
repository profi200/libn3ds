/*
 *   This file is part of libn3ds
 *   Copyright (C) 2024 Aurora Wright, TuxSH, derrek, profi200
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

@ Based on https://github.com/AuroraWright/Luma3DS/blob/master/arm9/source/alignedseqmemcpy.s

#include "asm_macros.h"

.syntax unified
.cpu arm946e-s
.fpu softvfp



@ void copy32(u32 *restrict dst, const u32 *restrict src, u32 size)
BEGIN_ASM_FUNC copy32
	subs   r2, r2, #32           @ r2 -= 32;
	bcc    copy32_words          @ if(carryClear) goto copy32_words;

	@ Copy 32 bytes at a time.
	push   {r4-r9}               @ Save regs.
	copy32_block_lp:
		ldmia  r1!, {r3-r9, r12} @ r3_to_r9_r12 = *((Block32*)r1); r1 += 32;
		subs   r2, r2, #32       @ r2 -= 32; // Update flags.
		stmia  r0!, {r3-r9, r12} @ *((Block32*)r0) = r3_to_r9_r12; r0 += 32;
		bcs    copy32_block_lp   @ while(carrySet);
	pop    {r4-r9}               @ Restore regs.

copy32_words:
	ands   r12, r2, #28          @ r12 = r2 & 28u;
	beq    copy32_halfword_byte  @ if(r12 == 0) goto copy32_halfword_byte;

	@ Copy 4 bytes at a time.
	copy32_word_lp:
		ldr    r3, [r1], #4      @ r3 = *r1++; // u32.
		subs   r12, r12, #4      @ r12 -= 4;
		str    r3, [r0], #4      @ *r0++ = r3; // u32.
		bne    copy32_word_lp    @ while(r12 != 0);

	@ Copy 0-3 bytes.
copy32_halfword_byte:
	movs   r2, r2, lsl #31       @ r2 <<= 31;
	ldrhcs r3, [r1], #2          @ if(carrySet) r3 = *r1++; // u16.
	strhcs r3, [r0], #2          @ if(carrySet) *r0++ = r3; // u16.
	ldrbmi r3, [r1]              @ if(r2 < 0) r3 = *r1;     // u8.
	strbmi r3, [r0]              @ if(r2 < 0) *r0 = r3;     // u8.
	bx     lr                    @ return;
END_ASM_FUNC


@ void clear32(u32 *ptr, const u32 value, u32 size)
BEGIN_ASM_FUNC clear32
	subs   r2, r2, #32                @ r2 -= 32;
	bcc    clear32_words              @ if(carryClear) goto clear32_words;

	@ Clear 32 bytes at a time.
	push   {r4}                       @ Save r4.
	mov    r3, r1                     @ r3 = r1;
	mov    r4, r1                     @ r4 = r1;
	mov    r12, r1                    @ r12 = r1;
	clear32_block_lp:
		stmia  r0!, {r1, r3, r4, r12} @ *((Block16*)r0) = r1_r3_r4_r12; r0 += 16;
		subs   r2, r2, #32            @ r2 -= 32;
		stmia  r0!, {r1, r3, r4, r12} @ *((Block16*)r0) = r1_r3_r4_r12; r0 += 16;
		bcs    clear32_block_lp       @ while(carrySet);
	pop    {r4}                       @ Restore r4.

clear32_words:
	ands   r12, r2, #28               @ r12 = r2 & 28u;
	beq    clear32_halfword_byte      @ if(r12 == 0) goto clear32_halfword_byte;

	@ Clear 4 bytes at a time.
	clear32_word_lp:
		subs   r12, r12, #4           @ r12 -= 4;
		str    r1, [r0], #4           @ *r0++ = r1; // u32.
		bne    clear32_word_lp        @ while(r12 != 0);

	@ Clear 0-3 bytes.
clear32_halfword_byte:
	movs   r2, r2, lsl #31            @ r2 <<= 31;
	strhcs r1, [r0], #2               @ if(carrySet) *r0++ = r1; // u16.
	strbmi r1, [r0]                   @ if(r2 < 0)   *r0 = r1;   // u8.
	bx     lr                         @ return;
END_ASM_FUNC
