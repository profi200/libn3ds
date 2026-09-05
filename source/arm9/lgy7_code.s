/*
 *   This file is part of open_agb_firm
 *   Copyright (C) 2023 profi200
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

#include "arm.h"
#include "asm_macros.h"

.syntax unified
.cpu arm7tdmi
.fpu softvfp



BEGIN_ASM_FUNC _gba_vector_overlay
	/*
	 * Persistent GBA exception-vector overlay.
	 *
	 * The abort/reserved slots at 0x0C-0x14 host a minimal STOP trampoline.
	 * The IRQ vector at 0x18 normally continues through the GBA BIOS and is
	 * redirected to 0x0C only for a host-controlled GBA STOP transition.
	 */
	.word 0xEA000018  @ 0x00 Reset -> BIOS reset path.
	.word 0xEA000004  @ 0x04 Undefined instruction.
	.word 0xEA00004C  @ 0x08 SWI/SVC -> BIOS SVC handler.
	.word 0xE92D1004  @ 0x0C stmdb sp!, {r2, r12}.
	.word 0xEF030000  @ 0x10 SWI 03h / STOP.
	.word 0xE8BD1004  @ 0x14 ldmia sp!, {r2, r12}.
	.word 0xEA000042  @ 0x18 IRQ -> BIOS IRQ handler at 0x128.
	.word 0xE59FD1A0  @ 0x1C FIQ.

.global _gba_vector_overlay_size
_gba_vector_overlay_size = . - _gba_vector_overlay
END_ASM_FUNC

@ Must be located at 0x3007E00.
BEGIN_ASM_FUNC _gba_boot
	adr  r1, _gba_boot_thumb + 1  @ 0x3007E1D
	msr  CPSR_fsxc, #PSR_INT_OFF | PSR_SVC_MODE  @ Already set on reset.
	add  sp, r1, #0x6B  @ 0x3007E88
	msr  CPSR_fsxc, #PSR_INT_OFF | PSR_SYS_MODE
	add  sp, r1, #0x5B  @ 0x3007E78
	mov  r3, #0x4000000
	bx   r1

.thumb
_gba_boot_thumb:
	movs r0, #1
	movs r1, #3
	lsls r1, r1, #8
	strb r0, [r3, r1]  @ POSTFLG = 1. Keep the vector overlay enabled.
	@ The original ARM7 stub waits 256 cycles here (for the BIOS overlay disable?).
	@ The original ARM7 stub waits 1677800 cycles (100 ms) here for LCD/LgyFb sync.
	@ The original ARM7 stub waits for REG_VCOUNT = 160 here.

	lsls r4, r0, #26  @ 0x4000000 Needed for "function" call 0xBC below.
	movs r0, #0xFF    @ Clear WRAM, iWRAM, palette RAM, VRAM, OAM
	                  @ + reset SIO, sound and all other registers.

	swi  0x01       @ RegisterRamReset
	@ After BIOS intro REG_TM0CNT_L is set to 0xFF8C instead of 0.
	@ No other differences between direct boot and BIOS.

	movs r0, #0xBC  @ SoftReset (0xB4) but skipping r2 & r4 loading.
	movs r2, #0

	@ REG_VCOUNT should be 126 at ROM entry like after BIOS intro.
_gba_boot_vcount_lp:
	ldrb r1, [r4, #6]  @ REG_VCOUNT
	cmp  r1, #126      @ Loop until REG_VCOUNT == 126.
	bne  _gba_boot_vcount_lp

	bx   r0

.align 2
.global _gba_boot_size
_gba_boot_size = . - _gba_boot
END_ASM_FUNC
