#include "arm11/drivers/wifi/sdio.h"
#include "arm11/drivers/wifi/sdio_spec.h"
#include "drivers/tmio.h"
#include "arm11/drivers/timer.h"
#include "arm11/fmt.h"


#define INIT_CLOCK     (400000u)
#define DEFAULT_CLOCK  (25000000u) // TODO: Maximum default clock for SDIO (25 MHz like SD?)? Also test overclocking.
#define HS_CLOCK       (50000000u)


#define SDIO_OCR_VOLT_MASK  (SDIO_OCR_3_2_3_3V)  // We support 3.3V only.
#define SDIO_OP_COND_ARG    (SDIO_OCR_VOLT_MASK) // No additional bits needed.


typedef struct
{
	TmioPort port;
	u16 rca;
} SdioDev;

static SdioDev g_sdioDev = {0};



u32 SDIO_init(void)
{
	// Init port, enable init clock and wait 74 clocks.
	SdioDev *const dev = &g_sdioDev;
	TmioPort *const port = &dev->port;
	TMIO_initPort(port, 0);
	TMIO_startInitClock(port, INIT_CLOCK);
	TIMER_sleepTicks(2 * TMIO_CLK2DIV(INIT_CLOCK) * 74);

	// This CMD does nothing for SDIO cards but it's used for detecting SPI mode.
	u32 res = TMIO_sendCommand(port, SDIO_GO_IDLE_STATE, 0);
	if(res != 0) return SDIO_ERR_GO_IDLE_STATE;

	// 3.1.2 Initialization by I/O Aware Host
	// "As shown in Figure 3-2 and Figure 3-3, an SDIO aware host should send CMD5 arg=0 as part of the
	//  initialization sequence after either Power On or a CMD 52 with write to I/O Reset. CMD5 arg=0 is
	//  required for using a removal card due to the following reasons:
	//  	(1) There was 3.1V-3.5V voltage range legacy card and then whether host power supply can provide
	//  	    the voltage in this range should be checked.
	//  	(2) There may be the card which requires CMD5 with WV= 0 to initialize the card.
	//
	//  CMD5 arg=0 may not be necessary for embedded system if the device does not require it."
	//
	// So for builtin SDIO devices we can skip this? Tested and works fine without.
	//res = TMIO_sendCommand(port, SDIO_IO_SEND_OP_COND, 0); // Arg = 0 is probing OCR.
	//if(res != 0) return SDIO_ERR_IO_SEND_OP_COND;

	u32 tries = 200;
	u32 ocr;
	while(1)
	{
		res = TMIO_sendCommand(port, SDIO_IO_SEND_OP_COND, SDIO_OP_COND_ARG);
		if(res != 0) return SDIO_ERR_IO_SEND_OP_COND;

		ocr = port->resp[0];
		if(!--tries || ocr & SDIO_READY) break;

		TIMER_sleepMs(5);
	}

	if(tries == 0) return SDIO_ERR_IO_OP_COND_TMOUT; // OCR at this point: 0x90FFFF00. Usually 2 tries.

	if(!(ocr & SDIO_OCR_VOLT_MASK)) return SDIO_ERR_VOLT_SUPPORT;

	// Stop clock at idle, init clock.
	TMIO_setClock(port, INIT_CLOCK);

	// TODO: Can we increase clock to default clock after this CMD? This will quit open-drain mode.
	//       Seems to work just fine.
	res = TMIO_sendCommand(port, SDIO_SEND_RELATIVE_ADDR, 0);
	if(res != 0) return SDIO_ERR_SEND_RCA;
	u32 rca = port->resp[0];
	dev->rca = rca>>16;      // RCA is usually 0x0001. TODO: Set this at the end (initialized flag).
	debug_printf("[SDIO] RCA 0x%" PRIX16 "\n", dev->rca);

	TMIO_setClock(port, DEFAULT_CLOCK);

	res = TMIO_sendCommand(port, SDIO_SELECT_CARD, rca);
	if(res != 0) return SDIO_ERR_SELECT_CARD;

	res = SDIO_writeReg8(0, 7, 1u<<7 | 2); // Disable pull-up and switch to 4 bit bus width.
	TMIO_setBusWidth(port, 4);
	ee_printf("Set bus width: res 0x%lx, out 0x%X\n", res, SDIO_readReg8(0, 7));
	res = SDIO_writeReg8(0, 2, 1u<<1); // Enable function 1.
	TIMER_sleepMs(5); // The WiFi module incorrectly reports a timeout of 0 ms. 5 ms is the lowest working.
	ee_printf("Enable IO 1: res 0x%lx, out 0x%X\n", res, SDIO_readReg8(0, 3));
	// TODO: Read regs, increase clock and set bus width to 4 bit.
	ee_puts("[SDIO] Init done.");

	return SDIO_ERR_NONE;
}

u32 SDIO_reset(void)
{
	// 4.4 Reset for SDIO.
	// Linux does a read-modify-write but this reg is write-only.
	return SDIO_writeReg8(0, 6, 1u<<3);
}

u32 SDIO_io_rw_direct(const bool write, const u8 func, const u32 addr, const u8 in, u8 *const out)
{
	if(func > 7 || addr >= 0x20000) return SDIO_ERR_INVALID_ARG;

	TmioPort *const port = &g_sdioDev.port;

	const u32 arg = write<<31 | func<<28 | (write && out != NULL ? 1u<<27 : 0) | addr<<9 | in;
	const u32 res = TMIO_sendCommand(port, SDIO_IO_RW_DIRECT, arg);
	if(res != 0) return SDIO_ERR_IO_RW_DIRECT;

	const u32 resp = port->resp[0];
	if(resp & SDIO_R5_ERROR)           return SDIO_ERR_R5_ERROR;
	if(resp & SDIO_R5_FUNCTION_NUMBER) return SDIO_ERR_R5_INVALID_FUNC;
	if(resp & SDIO_R5_OUT_OF_RANGE)    return SDIO_ERR_R5_OUT_OF_RANGE;

	if(out != NULL) *out = resp & 0xFFu;

	return SDIO_ERR_NONE;
}

// TODO: Instead of count + size only use size (block mode if size > block size)?
u32 SDIO_io_rw_extended(const bool write, const u8 func, const u32 addr, const bool incAddr, u8 *buf, const u16 count, const u16 size)
{
	if(func > 7 || addr >= 0x20000) return SDIO_ERR_INVALID_ARG;

	TmioPort *const port = &g_sdioDev.port;

	u32 arg = write<<31;
	arg |= func<<28;
	arg |= incAddr<<26;
	arg |= addr<<9;
	arg |= (count == 0 ? (size == 512 ? 0 : size) : 1u<<27 | count);
	const u32 res = TMIO_sendCommand(port, SDIO_IO_RW_EXTENDED(!write), arg);
	if(res != 0) return SDIO_ERR_IO_RW_EXTENDED;

	const u32 resp = port->resp[0];
	if(resp & SDIO_R5_ERROR)           return SDIO_ERR_R5_ERROR;
	if(resp & SDIO_R5_FUNCTION_NUMBER) return SDIO_ERR_R5_INVALID_FUNC;
	if(resp & SDIO_R5_OUT_OF_RANGE)    return SDIO_ERR_R5_OUT_OF_RANGE;

	return SDIO_ERR_NONE;
}

u8 SDIO_readReg8(const u8 func, const u32 addr)
{
	u8 out;
	if(SDIO_io_rw_direct(false, func, addr, 0, &out) != SDIO_ERR_NONE)
		out = 0xFF;

	return out;
}

u32 SDIO_writeReg8(const u8 func, const u32 addr, const u8 in)
{
	return SDIO_io_rw_direct(true, func, addr, in, NULL);
}

u8 SDIO_writeReadReg8(const u8 func, const u32 addr, const u8 in)
{
	u8 out;
	if(SDIO_io_rw_direct(true, func, addr, in, &out) != SDIO_ERR_NONE)
		out = 0xFF;

	return out;
}
