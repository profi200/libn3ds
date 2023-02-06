/*
 *   This file is part of open_agb_firm
 *   Copyright (C) 2021 derrek, profi200
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

#include <stdatomic.h>
#include "types.h"
#include "drivers/tmio.h"
#include "drivers/tmio_config.h"


// Using atomic load/store produces better code than volatile
// but still ensures that the status is always read from memory.
#define GET_STATUS(ptr)       atomic_load_explicit((ptr), memory_order_relaxed)
#define SET_STATUS(ptr, val)  atomic_store_explicit((ptr), (val), memory_order_relaxed)


static u32 g_status[2] = {0};



ALWAYS_INLINE u8 port2Controller(const u8 portNum)
{
	return portNum / 2;
}

static void tmioIsr(const u32 id)
{
	const u8 controller = (id == TMIO_IRQ_ID_CONTROLLER1 ? 0 : 1);
	Tmio *const regs = getTmioRegs(controller);

	g_status[controller] |= regs->sd_status;
	regs->sd_status = STATUS_CMD_BUSY; // Never acknowledge STATUS_CMD_BUSY.

	// TODO: Some kind of event to notify the main loop for remove/insert.
}

void TMIO_init(void)
{
	// Do controller and port mapping (see tmio_config.h).
	TMIO_MAP_CONTROLLERS();

	// Register ISR and enable IRQs.
	// IRQs are only fired on the side a controller is mapped to.
	TMIO_REGISTER_ISR(tmioIsr);

	// Reset all controllers.
	for(u32 i = 0; i < TMIO_NUM_CONTROLLERS; i++)
	{
		// Setup 32 bit FIFO.
		Tmio *const regs = getTmioRegs(i);
		regs->sd_fifo32_cnt   = FIFO32_CLEAR | FIFO32_EN;
		regs->sd_blocklen32   = 512;
		regs->sd_blockcount32 = 1;
		regs->dma_ext_mode    = DMA_EXT_DMA_MODE;

		// Reset. Unlike similar controllers no delay is needed.
		// Resets the following regs:
		// REG_SD_STOP, REG_SD_RESP0-7, REG_SD_STATUS1-2, REG_SD_ERR_STATUS1-2,
		// REG_SD_CLK_CTRL, REG_SD_OPTION, REG_SDIO_STATUS.
		regs->soft_rst = SOFT_RST_RST;
		regs->soft_rst = SOFT_RST_NORST;

		regs->sd_portsel         = PORTSEL_P0;
		regs->sd_blockcount      = 1;
		regs->sd_status_mask     = STATUS_MASK_DEFAULT;
		regs->sd_clk_ctrl        = SD_CLK_DEFAULT;
		regs->sd_blocklen        = 512;
		regs->sd_option          = OPTION_BUS_WIDTH1 | OPTION_UNK14 | OPTION_DEFAULT_TIMINGS;
		regs->ext_cdet_mask      = EXT_CDET_MASK_ALL;
		regs->ext_cdet_dat3_mask = EXT_CDET_DAT3_MASK_ALL;

		// Disable SDIO.
		regs->sdio_mode        = 0;
		regs->sdio_status_mask = SDIO_STATUS_MASK_ALL;
		regs->ext_sdio_irq     = EXT_SDIO_IRQ_MASK_ALL;
	}
}

void TMIO_deinit(void)
{
	// Unregister ISR and disable IRQs.
	TMIO_UNREGISTER_ISR();

	// Mask all IRQs.
	for(u32 i = 0; i < TMIO_NUM_CONTROLLERS; i++)
	{
		// 32 bit FIFO IRQs.
		Tmio *const regs = getTmioRegs(i);
		regs->sd_fifo32_cnt = 0; // FIFO and all IRQs disabled/masked.

		// Regular IRQs.
		regs->sd_status_mask = STATUS_MASK_ALL;

		// SDIO IRQs.
		regs->sdio_status_mask = SDIO_STATUS_MASK_ALL;
	}

	// Reset controller and port mapping (see tmio_config.h).
	TMIO_UNMAP_CONTROLLERS();
}

void TMIO_initPort(TmioPort *const port, const u8 portNum)
{
	// Reset port state.
	port->portNum     = portNum;
	port->sd_clk_ctrl = SD_CLK_DEFAULT;
	port->sd_blocklen = 512;
	port->sd_option   = OPTION_BUS_WIDTH1 | OPTION_UNK14 | OPTION_DEFAULT_TIMINGS;
}

// TODO: What if we get rid of setPort() and only use one port per controller?
static void setPort(Tmio *const regs, const TmioPort *const port)
{
	// TODO: Can we somehow prevent all these reg writes each time?
	//       Maybe some kind of dirty flag + active port check?
	regs->sd_portsel    = port->portNum % 2u;
	regs->sd_clk_ctrl   = port->sd_clk_ctrl;
	const u16 blocklen = port->sd_blocklen;
	regs->sd_blocklen   = blocklen;
	regs->sd_option     = port->sd_option;
	regs->sd_blocklen32 = blocklen;
}

bool TMIO_cardDetected(void)
{
	return getTmioRegs(port2Controller(TMIO_CARD_PORT))->sd_status & STATUS_DETECT;
}

bool TMIO_cardWritable(void)
{
	return getTmioRegs(port2Controller(TMIO_CARD_PORT))->sd_status & STATUS_NO_WRPROT;
}

// TODO: This might be a little dodgy not using setPort() before changing clock.
//       It's fine as long as only one port is used per controller
//       and there is no concurrent access to it.
// TODO: Turn this into a "powerup sequence" sort of function.
void TMIO_startInitClock(TmioPort *const port, const u32 clk)
{
	const u16 sd_clk_ctrl = SD_CLK_EN | TMIO_CLK2DIV(clk)>>2;
	port->sd_clk_ctrl = sd_clk_ctrl;
	getTmioRegs(port2Controller(port->portNum))->sd_clk_ctrl = sd_clk_ctrl;
}

static void getResponse(const Tmio *const regs, TmioPort *const port, const u16 cmd)
{
	// We could check for response type none as well but it's not worth it.
	if((cmd & CMD_RESP_MASK) != CMD_RESP_R2)
	{
		port->resp[0] = regs->sd_resp[0];
	}
	else // 136 bit R2 responses need special treatment...
	{
		u32 resp[4];
		for(u32 i = 0; i < 4; i++) resp[i] = regs->sd_resp[i];

		port->resp[0] = resp[3]<<8 | resp[2]>>24;
		port->resp[1] = resp[2]<<8 | resp[1]>>24;
		port->resp[2] = resp[1]<<8 | resp[0]>>24;
		port->resp[3] = resp[0]<<8; // TODO: Add the missing CRC7 and bit 0?
	}
}

// Note: Using STATUS_DATA_END to detect transfer end doesn't work reliably
//       because STATUS_DATA_END fires before we even read anything from FIFO
//       on single block read transfer.
static void doCpuTransfer(Tmio *const regs, const u16 cmd, u32 *buf, const u32 *const statusPtr)
{
	const u32 wordBlockLen = (regs->sd_blocklen + 3) / 4; // Round up for odd sizes.
	u32 blockCount         = regs->sd_blockcount;
	vu32 *const fifo = getTmioFifo(regs);
	if(cmd & CMD_DIR_R)
	{
		while((GET_STATUS(statusPtr) & STATUS_MASK_ERR) == 0 && blockCount > 0)
		{
			if(regs->sd_fifo32_cnt & FIFO32_FULL) // RX ready.
			{
				const u32 *const blockEnd = buf + wordBlockLen;
				do
				{
					*buf++ = *fifo;
					*buf++ = *fifo;
					*buf++ = *fifo;
					*buf++ = *fifo;
				} while(buf < blockEnd);

				blockCount--;
			}
			else __wfi();
		}
	}
	else
	{
		// TODO: Write first block ahead of time?
		// gbatek Command/Param/Response/Data at bottom of page.
		while((GET_STATUS(statusPtr) & STATUS_MASK_ERR) == 0 && blockCount > 0)
		{
			if(!(regs->sd_fifo32_cnt & FIFO32_NOT_EMPTY)) // TX request.
			{
				const u32 *const blockEnd = buf + wordBlockLen;
				do
				{
					*fifo = *buf++;
					*fifo = *buf++;
					*fifo = *buf++;
					*fifo = *buf++;
				} while(buf < blockEnd);

				blockCount--;
			}
			else __wfi();
		}
	}
}

u32 TMIO_sendCommand(TmioPort *const port, const u16 cmd, const u32 arg)
{
	const u8 controller = port2Controller(port->portNum);
	Tmio *const regs = getTmioRegs(controller);

	// Clear status before sending another command.
	u32 *const statusPtr = &g_status[controller];
	SET_STATUS(statusPtr, 0);

	setPort(regs, port);
	const u16 blocks = port->blocks;
	regs->sd_blockcount = blocks;         // sd_blockcount32 doesn't need to be set.
	regs->sd_stop       = STOP_AUTO_STOP; // Auto STOP_TRANSMISSION (CMD12) on multi-block transfer.
	regs->sd_arg        = arg;

	// We don't need FIFO IRQs when using DMA. buf = NULL means DMA.
	u32 *buf = port->buf;
	u16 f32Cnt = FIFO32_CLEAR | FIFO32_EN;
	if(buf != NULL) f32Cnt |= (cmd & CMD_DIR_R ? FIFO32_FULL_IE : FIFO32_NOT_EMPTY_IE);
	regs->sd_fifo32_cnt = f32Cnt;
	regs->sd_cmd        = (blocks > 1 ? CMD_MBT | cmd : cmd); // Start.

	// TODO: Benchmark if this order is ideal?
	// Response end comes immediately after the
	// command so we need to check before __wfi().
	// On error response end still fires.
	while((GET_STATUS(statusPtr) & STATUS_RESP_END) == 0) __wfi();
	getResponse(regs, port, cmd);

	if((cmd & CMD_DT_EN) != 0)
	{
		// If we have to transfer data do so now.
		if(buf != NULL) doCpuTransfer(regs, cmd, buf, statusPtr);

		// Wait for data end if needed.
		// On error data end still fires.
		while((GET_STATUS(statusPtr) & STATUS_DATA_END) == 0) __wfi();
	}

	// STATUS_CMD_BUSY is no longer set at this point.

	return GET_STATUS(statusPtr) & STATUS_MASK_ERR;
}
