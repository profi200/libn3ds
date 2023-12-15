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


#ifdef __ARM9__
// ITCM.
#define ITCM_BASE          (0x00000000)
#define ITCM_KERN9_MIRROR  (0x01FF8000)
#define ITCM_BOOT9_MIRROR  (0x07FF8000)
#define ITCM_SIZE          (0x00008000) // 32 KiB.

// AHB RAM (ARM9 memory).
#define AHB_RAM_BASE       (0x08000000)
#define AHB_RAM_EXT_BASE   (AHB_RAM_BASE + AHB_RAM_SIZE)
#define AHB_RAM_SIZE       (0x00100000) // 1 MiB.
#define AHB_RAM_EXT_SIZE   (0x00080000) // 512 KiB.

// AHB memory mapped IO registers.
#define IO_AHB_BASE        (0x10000000)
#define IO_AHB_SIZE        (0x00100000) // 1 MiB.

// DTCM.
#define DTCM_BASE          (0xFFF00000)
#define DTCM_SIZE          (0x00004000) // 16 KiB.

// ARM9 bootrom.
#define BOOT9_BASE         (0xFFFF0000)
#define BOOT9_SIZE         (0x00010000) // 64 KiB.
#endif // #ifdef __ARM9__

#ifdef __ARM11__
// ARM11 bootrom.
#define BOOT11_BASE        (0x00000000)
#define BOOT11_LO_MIRROR   (0x00010000)
#define BOOT11_HI_MIRROR   (0xFFFF0000)
#define BOOT11_SIZE        (0x00010000) // 64 KiB.

// AXI memory mapped IO registers.
#define IO_AXI_BASE        (0x10200000)
#define IO_AXI_SIZE        (0x00300000) // 3 MiB (guess).

// ARM11 MPCore private memory region.
#define MPCORE_PRIV_BASE   (0x17E00000)
#define MPCORE_PRIV_SIZE   (0x00002000) // 8 KiB.

// L2C-310 Level 2 Cache Controller.
#define L2C_BASE           (0x17E10000)
#define L2C_SIZE           (0x00001000) // 4 KiB.

// QTM RAM.
#define QTM_RAM_BASE       (0x1F000000)
#define QTM_RAM_SIZE       (0x00400000) // 4 MiB.
#endif // #ifdef __ARM11__


// Common memory mapped IO registers.
#define IO_COMMON_BASE     (0x10100000)
#define IO_COMMON_SIZE     (0x00100000) // 1 MiB.

// VRAM.
#define VRAM_BASE          (0x18000000)
#define VRAM_BANK0         (VRAM_BASE)
#define VRAM_BANK1         (VRAM_BASE + VRAM_SIZE / 2)
#define VRAM_SIZE          (0x00600000) // 6 MiB.
#define VRAM_BANK_SIZE     (VRAM_SIZE / 2)

// DSP RAM (DSP memory).
#define DSP_RAM_BASE       (0x1FF00000)
#define DSP_RAM_SIZE       (0x00080000) // 512 KiB.

// AXI RAM (AXIWRAM).
#define AXI_RAM_BASE       (0x1FF80000)
#define AXI_RAM_SIZE       (0x00080000) // 512 KiB.

// FCRAM.
#define FCRAM_BASE         (0x20000000)
#define FCRAM_EXT_BASE     (FCRAM_BASE + FCRAM_SIZE)
#define FCRAM_SIZE         (0x08000000) // 128 MiB.
#define FCRAM_EXT_SIZE     (FCRAM_SIZE)


// Custom mappings.
#ifdef __ARM9__
#define A9_VECTORS_START     (AHB_RAM_BASE)
#define A9_VECTORS_SIZE      (0x40)
#define A9_STUB_ENTRY        (ITCM_KERN9_MIRROR + ITCM_SIZE - 0x200)
#define A9_STUB_SIZE         (0x200)
#define A9_HEAP_END          (AHB_RAM_BASE + AHB_RAM_SIZE)
#define A9_STACK_START       (DTCM_BASE)
#define A9_STACK_END         (DTCM_BASE + DTCM_SIZE - 0x400)
#define A9_IRQ_STACK_START   (DTCM_BASE + DTCM_SIZE - 0x400)
#define A9_IRQ_STACK_END     (DTCM_BASE + DTCM_SIZE)
#define A9_EXC_STACK_START   (ITCM_KERN9_MIRROR + (ITCM_SIZE / 2))
#define A9_EXC_STACK_END     (ITCM_KERN9_MIRROR + ITCM_SIZE)
#define FIRM_LOAD_ADDR       (VRAM_BASE + 0x200000)
#define RAM_FIRM_BOOT_ADDR   (FCRAM_BASE + 0x1000)
#endif // #ifdef __ARM9__

#ifdef __ARM11__
#define A11_C0_STACK_START   (AXI_RAM_BASE)                // Core 0 stack.
#define A11_C0_STACK_END     (A11_C0_STACK_START + 0x2000)
#define A11_C1_STACK_START   (A11_C0_STACK_END)            // Core 1 stack.
#define A11_C1_STACK_END     (A11_C1_STACK_START + 0x2000)
// WARNING: The stacks for core 2/3 are temporary.
#define A11_C2_STACK_START   (FCRAM_BASE - 0x600)         // Core 2 stack.
#define A11_C2_STACK_END     (FCRAM_BASE - 0x400)
#define A11_C3_STACK_START   (FCRAM_BASE - 0x400)         // Core 3 stack.
#define A11_C3_STACK_END     (FCRAM_BASE - 0x200)
#define A11_EXC_STACK_START  (VRAM_BASE + VRAM_SIZE - 0x200000)
#define A11_EXC_STACK_END    (VRAM_BASE + VRAM_SIZE - 0x100000)
#define A11_MMU_TABLES_BASE  (A11_C1_STACK_END)
#define	A11_VECTORS_START    (AXI_RAM_BASE + AXI_RAM_SIZE - 0x60)
#define A11_VECTORS_SIZE     (0x60)
#define A11_FALLBACK_ENTRY   (AXI_RAM_BASE + AXI_RAM_SIZE - 0x4)
#define A11_STUB_ENTRY       (AXI_RAM_BASE + AXI_RAM_SIZE - 0x200)
#define	A11_STUB_SIZE        (0x1A0) // Don't overwrite the vectors.
#define A11_HEAP_END         (AXI_RAM_BASE + AXI_RAM_SIZE)
#endif // #ifdef __ARM11__