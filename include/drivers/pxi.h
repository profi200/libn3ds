#pragma once

/*
 *   This file is part of libn3ds
 *   Copyright (C) 2024 derrek, profi200
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

#ifdef __ARM9__
#define PXI_REGS_BASE  (IO_AHB_BASE + 0x8000)
#elif __ARM11__
#define PXI_REGS_BASE  (IO_COMMON_BASE + 0x63000)
#endif // #ifdef __ARM9__

typedef struct
{
	union
	{
		struct
		{
			vu8 sync_recvd; // 0x0 Received. Set by remote CPU via SENT.
			vu8 sync_sent;  // 0x1 Write-only, sent.
			u8 _0x2;
			vu8 sync_irq;   // 0x3
		};
		vu32 sync;          // 0x0
	};
	vu32 cnt;               // 0x4
	vu32 send;              // 0x8 Send FIFO.
	const vu32 recv;        // 0xC Receive FIFO.
} Pxi;
static_assert(offsetof(Pxi, recv) == 0xC, "Error: Member recv of Pxi is not at offset 0xC!");

ALWAYS_INLINE Pxi* getPxiRegs(void)
{
	return (Pxi*)PXI_REGS_BASE;
}


// REG_PXI_SYNC
// Note: SENT and RECV in REG_PXI_SYNC do not count
//       the amount of bytes sent or received through the FIFOs!
//       They are simply CPU read-/writable fields.
#define PXI_SYNC_RECVD                 (REG_PXI_SYNC & 0xFFu)
#define PXI_SYNC_SENT(sent)            ((REG_PXI_SYNC & ~(0xFFu<<8)) | (sent)<<8)
#ifdef __ARM9__
#define PXI_SYNC_IRQ                   BIT(29) // Trigger interrupt on ARM11.
#define PXI_SYNC_IRQ2                  BIT(30) // Another, separate interrupt trigger for ARM11.
#elif __ARM11__
// 29 unused unlike ARM9 side.
#define PXI_SYNC_IRQ                   BIT(30) // Trigger interrupt on ARM9.
#endif // #ifdef __ARM9__
#define PXI_SYNC_IRQ_EN                BIT(31) // Enable interrupt(s) from remote CPU.

// REG_PXI_SYNC_IRQ (byte 3 of REG_PXI_SYNC)
#ifdef __ARM9__
#define PXI_SYNC_IRQ_IRQ               BIT(5) // Trigger interrupt on ARM11.
#define PXI_SYNC_IRQ_IRQ2              BIT(6) // Another, separate interrupt trigger for ARM11.
#elif __ARM11__
// 29 unused unlike ARM9 side.
#define PXI_SYNC_IRQ_IRQ               BIT(6) // Trigger interrupt on ARM9.
#endif // #ifdef __ARM9__
#define PXI_SYNC_IRQ_IRQ_EN            BIT(7) // Enable interrupt(s) from remote CPU.

// REG_PXI_CNT
// Status bits: 0 = no, 1 = yes.
#define PXI_CNT_SEND_EMPTY             BIT(0)  // Send FIFO empty status.
#define PXI_CNT_SEND_FULL              BIT(1)  // Send FIFO full status.
#define PXI_CNT_SEND_NOT_FULL_IRQ_EN   BIT(2)  // Send FIFO not full interrupt enable. TODO: Test the behavior.
#define PXI_CNT_FLUSH_SEND             BIT(3)  // Flush send FIFO.
#define PXI_CNT_RECV_EMPTY             BIT(8)  // Receive FIFO empty status.
#define PXI_CNT_RECV_FULL              BIT(9)  // Receive FIFO full status.
#define PXI_CNT_RECV_NOT_EMPTY_IRQ_EN  BIT(10) // Receive FIFO not empty interrupt enable. TODO: Test the behavior.
#define PXI_CNT_FIFO_ERROR             BIT(14) // Receive FIFO underrun or send FIFO overrun error flag. Acknowledge by writing 1.
#define PXI_CNT_EN_FIFOS               BIT(15) // Enables REG_PXI_SEND and REG_PXI_RECV FIFOs.



// All of the below functions for libn3ds internal usage only.
void PXI_init(void);
void PXI_deinit(void);
u32 PXI_sendCmd(u32 cmd, const u32 *buf, u32 words);

#ifdef __cplusplus
} // extern "C"
#endif