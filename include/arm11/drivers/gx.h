#pragma once

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

#include "types.h"
#include "mem_map.h"


#ifdef __cplusplus
extern "C"
{
#endif

#define GX_REGS_BASE  (IO_AXI_BASE + 0x200000)

// PICA screen clear(?) registers.
typedef struct
{
	vu32 s_addr; // 0x0 Start address.
	vu32 e_addr; // 0x4 End address.
	vu32 val;    // 0x8 Fill value.
	vu32 cnt;    // 0xC
} PscFill;
static_assert(offsetof(PscFill, cnt) == 0xC, "Error: Member cnt of PscFill is not at offset 0xC!");

// PICA display controller registers.
// Pitfall warning: The 3DS LCDs are physically rotated 90° CCW.
typedef struct
{
	// Horizontal timings.
	vu32 h_total;        // 0x00
	vu32 h_start;        // 0x04 Horizontal picture start.
	vu32 h_border;       // 0x08 Horizontal border start.
	vu32 h_blank;        // 0x0C Horizontal blanking start.
	vu32 h_sync;         // 0x10 Horizontal sync start.
	vu32 h_back_porch;   // 0x14 Horizontal back porch start.
	vu32 h_left_border;  // 0x18 Horizontal left border start.
	vu32 h_irq_range;    // 0x1C Horizontal interrupt position start/end.
	vu32 h_dma_pos;      // 0x20 ?

	// Vertical timings.
	vu32 v_total;        // 0x24
	vu32 v_start;        // 0x28 Vertical picture start.
	vu32 v_border;       // 0x2C Vertical border start.
	vu32 v_blank;        // 0x30 Vertical blanking start.
	vu32 v_sync;         // 0x34 Vertical sync start.
	vu32 v_back_porch;   // 0x38 Vertical back porch start.
	vu32 v_top_border;   // 0x3C Vertical top border start.
	vu32 v_irq_range;    // 0x40 Vertical interrupt position start/end.
	vu32 v_incr_h_pos;   // 0x44 ??

	vu32 signal_pol;     // 0x48 H-/V-sync signal polarity.
	vu32 border_color;   // 0x4C
	const vu32 h_count;  // 0x50
	const vu32 v_count;  // 0x54
	u8 _0x58[4];
	vu32 pic_dim;        // 0x5C Output picture dimensions in pixels.
	vu32 pic_border_h;   // 0x60 Horizontal border start/end?
	vu32 pic_border_v;   // 0x64 Vertical border start/end?
	vu32 fb_a0;          // 0x68 3D left eye frame buffer 0. TODO: Rename all to l0, r0 ect.
	vu32 fb_a1;          // 0x6C 3D left eye frame buffer 1.
	vu32 fb_fmt;         // 0x70 Frame buffer format.
	vu32 cnt;            // 0x74
	vu32 swap;           // 0x78 Frame buffer select and IRQ status/acknowledge.
	const vu32 stat;     // 0x7C
	vu32 color_lut_idx;  // 0x80 Color lookup table index.
	vu32 color_lut_data; // 0x84 Color lookup table data.
	u8 _0x88[8];
	vu32 fb_stride;      // 0x90 Frame buffer row (H) stride in bytes.
	vu32 fb_b0;          // 0x94 3D right eye frame buffer 0.
	vu32 fb_b1;          // 0x98 3D right eye frame buffer 1.
	vu32 latch_pos;      // 0x9C Latch positions for (most of) above registers.
} Pdc;
static_assert(offsetof(Pdc, latch_pos) == 0x9C, "Error: Member latch_pos of Pdc is not at offset 0x9C!");

// PICA picture formatter(?) registers.
typedef struct
{
	vu32 in_addr;   // 0x00
	vu32 out_addr;  // 0x04
	vu32 dt_outdim; // 0x08 Display transfer output dimensions.
	vu32 dt_indim;  // 0x0C Display transfer input dimensions.
	vu32 flags;     // 0x10
	vu32 unk14;     // 0x14 Transfer interval?
	vu32 cnt;       // 0x18
	vu32 irq_pos;   // 0x1C ?
	vu32 len;       // 0x20 Texture copy size in bytes.
	vu32 tc_indim;  // 0x24 Texture copy input width and gap in 16 byte units.
	vu32 tc_outdim; // 0x28 Texture copy output width and gap in 16 byte units.
} Ppf;
static_assert(offsetof(Ppf, tc_outdim) == 0x28, "Error: Member tc_outdim of Ppf is not at offset 0x28!");

typedef struct
{
	const vu32 revision;          // 0x0000 ?
	vu32 gpu_clk;                 // 0x0004 Bit 0 screws up memory timings. Each bit seems to be a different clock setting.
	u8 _0x8[8];

	// Memory fill engines.
	PscFill psc_fill0;            // 0x0010
	PscFill psc_fill1;            // 0x0020

	vu32 psc_vram;                // 0x0030 gsp mudule only changes bit 8-11.
	const vu32 psc_irq_stat;      // 0x0034
	u8 _0x38[0x18];
	vu32 psc_dma_prio0;           // 0x0050 ?
	vu32 psc_dma_prio1;           // 0x0054 ?
	const vu32 unk58;             // 0x0058 Debug related?
	u8 _0x5c[0x14];

	// Undocumented memory bandwidth counters.
	const vu32 psc_ext_reads;     // 0x0070 Total reads from external memory.
	const vu32 psc_ext_writes;    // 0x0074 Total writes to external memory.
	const vu32 psc_vram_a_reads;  // 0x0078 Total reads from VRAM A.
	const vu32 psc_vram_a_writes; // 0x007C Total writes to VRAM A.
	const vu32 psc_vram_b_reads;  // 0x0080 Total reads from VRAM B.
	const vu32 psc_vram_b_writes; // 0x0084 Total writes to VRAM B.
	const vu32 p3d_vtx_reads;     // 0x0088 Vertex buffer reads? Both vertex and index buffers apparently.
	const vu32 p3d_tex_reads;     // 0x008C Texture unit reads (without cached reads).
	const vu32 p3d_depth_reads;   // 0x0090 Depth buffer reads.
	const vu32 p3d_depth_writes;  // 0x0094 Depth buffer writes.
	const vu32 p3d_color_reads;   // 0x0098 Color buffer reads.
	const vu32 p3d_color_writes;  // 0x009C Color buffer writes.
	const vu32 pdc0_reads;        // 0x00A0 PDC0 reads for both frame buffer A/B.
	const vu32 pdc1_reads;        // 0x00A4 PDC1 reads for both frame buffer A/B.
	const vu32 ppf_reads;         // 0x00A8 PPF source reads.
	const vu32 ppf_writes;        // 0x00AC PPF destination writes.
	const vu32 psc_fill0_writes;  // 0x00B0 PSC_FILL0 writes.
	const vu32 psc_fill1_writes;  // 0x00B4 PSC_FILL1 writes.
	const vu32 ext_vram_reads;    // 0x00B8 External (CPU, DMA ect.) reads from VRAM A/B.
	const vu32 ext_vram_writes;   // 0x00BC External (CPU, DMA ect.) writes to VRAM A/B.
	u8 _0xc0[0x10];

	vu32 unkD0;                   // 0x00D0 ?
	u8 _0xd4[0x32c];

	// Display controllers.
	Pdc pdc0;                     // 0x0400
	u8 _0x4a0[0x60];
	Pdc pdc1;                     // 0x0500
	u8 _0x5a0[0x660];

	// Texture/frame buffer DMA engine.
	Ppf ppf;                      // 0x0C00
	u8 _0xc2c[0x3d4];

	// Internal GPU registers (P3D). See gpu_regs.h.
	vu32 p3d[0x300];              // 0x1000
} GxRegs;
static_assert(offsetof(GxRegs, p3d[0x2FF]) == 0x1BFC, "Error: Member p3d[0x2FF] of GxRegs is not at offset 0x1BFC!");

ALWAYS_INLINE GxRegs* getGxRegs(void)
{
	return (GxRegs*)GX_REGS_BASE;
}


// Universal enums/defines.
enum
{
	GX_RGBA8  = 0u,
	GX_BGR8   = 1u,
	GX_R5G6B5 = 2u,
	GX_RGB5A1 = 3u,
	GX_RGBA4  = 4u
};

// REG_GX_PSC_DMA_PRIO0
// TODO: Figure this out + testing.
// 4 bits for each priority.
#define PSC_DMA_PRIO0(p7, p6, p5, p4, p3, p2, p1, p0)  ((p7)<<28 | (p6)<<24 | (p5)<<20 | (p4)<<16 | (p3)<<12 | (p2)<<8 | (p1)<<4 | (p0))

// REG_GX_PSC_DMA_PRIO1
// TODO: Figure this out + testing.
// 4 bits for each priority.
#define PSC_DMA_PRIO1(p2, p1, p0)  ((p2)<<8 | (p1)<<4 | (p0))

// PSC fill register defines.
// REG_GX_PSC_FILL_CNT
#define PSC_FILL_EN              (1u)
#define PSC_FILL_IRQ_MASK        (1u<<1)
#define PSC_FILL_16_BITS         (0u)
#define PSC_FILL_24_BITS         (1u<<8)
#define PSC_FILL_32_BITS         (2u<<8)
// Bits 16-20 burst length?

// REG_GX_PSC_VRAM
// Bit 0 unknown.
#define PSC_VRAM_BANK0_DIS       (1u<<8)
#define PSC_VRAM_BANK1_DIS       (1u<<9)
#define PSC_VRAM_BANK2_DIS       (1u<<10)
#define PSC_VRAM_BANK3_DIS       (1u<<11)
#define PSC_VRAM_BANK_DIS_ALL    (PSC_VRAM_BANK3_DIS | PSC_VRAM_BANK2_DIS | PSC_VRAM_BANK1_DIS | PSC_VRAM_BANK0_DIS)
// Bits 16-17 unknown.

// REG_GX_PSC_IRQ_STAT
// Bit 0 and 1 exist. Unknown purpose.
#define IRQ_STAT_PSC0            (1u<<26) // PSC fill 0 IRQ active.
#define IRQ_STAT_PSC1            (1u<<27) // PSC fill 1 IRQ active.
#define IRQ_STAT_PDC0            (1u<<28) // PDC0 IRQ active.
#define IRQ_STAT_PDC1            (1u<<29) // PDC1 IRQ active.
#define IRQ_STAT_PPF             (1u<<30) // PPF IRQ active.
#define IRQ_STAT_P3D             (1u<<31) // P3D IRQ active.
#define IRQ_STAT_ALL             (IRQ_STAT_P3D | IRQ_STAT_PPF | IRQ_STAT_PDC1 | IRQ_STAT_PDC0 | IRQ_STAT_PSC1 | IRQ_STAT_PSC0)

// PDC register defines.
#define PDC_RANGE(start, end)    ((end)<<16 | (start))    // For PDC registers containing start/end ranges.
#define PDC_COLOR_RGB(r, g, b)   ((b)<<16 | (g)<<8 | (r))

// REG_GX_PDC_SIGNAL_POL
#define PDC_SIGNAL_POL_H_ACT_LO  (0u)
#define PDC_SIGNAL_POL_H_ACT_HI  (1u)
#define PDC_SIGNAL_POL_V_ACT_LO  (0u)
#define PDC_SIGNAL_POL_V_ACT_HI  (1u<<4)

// REG_GX_PDC_FB_FMT
// TODO: DMA bursts need more testing.
#define PDC_FB_FMT(fmt)          ((fmt) & PDC_FB_FMT_MASK)
#define PDC_FB_FMT_MASK          (7u)
#define PDC_FB_OUT_A             (0u)      // No interleave.
#define PDC_FB_OUT_AA            (1u<<4)   // Interleaves with itself.
#define PDC_FB_OUT_AB            (2u<<4)   // Interleaves buffer A with B.
#define PDC_FB_OUT_BA            (3u<<4)   // Interleaves buffer B with A.
#define PDC_FB_DOUBLE_V          (1u<<6)   // Double vertical resolution. Not the same as AA output mode.
#define PDC_FB_BURST_4           (0u)      // Bytes per burst: RGB8:  -, Other:  4.
#define PDC_FB_BURST_6_8         (1u<<8)   // Bytes per burst: RGB8:  6, Other:  8.
#define PDC_FB_BURST_16          (2u<<8)   // Bytes per burst: RGB8:  -, Other: 16.
#define PDC_FB_BURST_24_32       (3u<<8)   // Bytes per burst: RGB8: 24, Other: 32.
#define PDC_FB_DMA_INT(i)        ((i)<<16) // DMA interval setting? Unit unknown.

// REG_GX_PDC_CNT
#define PDC_CNT_EN               (1u)     // Enables the display controller.
#define PDC_CNT_NO_IRQ_H         (1u<<8)  // Disables H(Blank?) IRQs.
#define PDC_CNT_NO_IRQ_V         (1u<<9)  // Disables VBlank IRQs.
#define PDC_CNT_NO_IRQ_ERR       (1u<<10) // Disables error IRQs. FIFO?
#define PDC_CNT_NO_IRQ_ALL       (PDC_CNT_NO_IRQ_ERR | PDC_CNT_NO_IRQ_V | PDC_CNT_NO_IRQ_H)
#define PDC_CNT_OUT_EN           (1u<<16) // Output enable?

// REG_GX_PDC_SWAP
#define PDC_SWAP_NEXT_MASK       (1u)     // Next framebuffer.
#define PDC_SWAP_CUR_MASK        (1u<<4)  // Currently displaying framebuffer?
#define PDC_SWAP_RST_FIFO        (1u<<8)  // Internal DMA FIFO reset/clear?
#define PDC_SWAP_IRQ_ACK_H       (1u<<16) // Acknowledges H(Blank?) IRQ.
#define PDC_SWAP_IRQ_ACK_V       (1u<<17) // Acknowledges VBlank IRQ.
#define PDC_SWAP_IRQ_ACK_ERR     (1u<<18) // Acknowledges error IRQ. FIFO?
#define PDC_SWAP_IRQ_ACK_ALL     (PDC_SWAP_IRQ_ACK_ERR | PDC_SWAP_IRQ_ACK_V | PDC_SWAP_IRQ_ACK_H)

// PPF register defines.
// REG_GX_PPF_DT_OUTDIM, REG_GX_PPF_DT_INDIM, REG_GX_PPF_TC_INDIM, REG_GX_PPF_TC_OUTDIM.
#define PPF_DIM(w, h)            ((h)<<16 | (w))

// REG_GX_PPF_FLAGS
#define PPF_NO_FLIP              (0u)
#define PPF_V_FLIP               (1u)
#define PPF_OUT_LINEAR           (0u)
#define PPF_OUT_TILED            (1u<<1)
#define PPF_CROP_EN              (1u<<2)
#define PPF_TEXCOPY              (1u<<3)
#define PPF_NO_TILED_2_LINEAR    (1u<<5)     // Raw copy together with PPF_OUT_LINEAR.
#define PPF_I_FMT(fmt)           ((fmt)<<8)
#define PPF_O_FMT(fmt)           ((fmt)<<12)
#define PPF_8X8_TILES            (0u)
#define PPF_32X32_TILES          (1u<<16)
#define PPF_NO_AA                (0u)
#define PPF_AA_X                 (1u<<24)
#define PPF_AA_XY                (2u<<24)

// REG_GX_PPF_CNT
#define PPF_EN                   (1u)
#define PPF_IRQ_MASK             (1u<<8)

#ifdef __cplusplus
} // extern "C"
#endif