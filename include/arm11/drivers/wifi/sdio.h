#pragma once

#include "types.h"

// Possible error codes for most of the functions below.
// TODO: Merge with WiFi error codes?
enum
{
	SDIO_ERR_NONE             =  0u, // No error.
	SDIO_ERR_GO_IDLE_STATE    =  1u, // GO_IDLE_STATE CMD error.
	SDIO_ERR_IO_SEND_OP_COND  =  2u, // IO_SEND_OP_COND CMD error.
	SDIO_ERR_IO_OP_COND_TMOUT =  3u, // Card initialization timeout.
	SDIO_ERR_VOLT_SUPPORT     =  4u, // Voltage not supported.
	SDIO_ERR_SEND_RCA         =  5u, // SEND_RELATIVE_ADDR CMD error.
	SDIO_ERR_SELECT_CARD      =  6u, // SELECT_CARD CMD error.
	SDIO_ERR_INVALID_ARG      =  7u, // Invalid function argument(s).
	SDIO_ERR_IO_RW_DIRECT     =  8u, // IO_RW_DIRECT CMD error.
	SDIO_ERR_R5_ERROR         =  9u, // SDIO R5 ERROR bit set in response.
	SDIO_ERR_R5_INVALID_FUNC  = 10u, // SDIO R5 FUNCTION_NUMBER bit set in response.
	SDIO_ERR_R5_OUT_OF_RANGE  = 11u  // SDIO R5 OUT_OF_RANGE bit set in response.
};



u32 SDIO_init(void);
u32 SDIO_reset(void);
u32 SDIO_io_rw_direct(const bool write, const u8 func, const u32 addr, const u8 in, u8 *const out);

u8 SDIO_readReg8(const u8 func, const u32 addr);
u32 SDIO_writeReg8(const u8 func, const u32 addr, const u8 in);
u8 SDIO_writeReadReg8(const u8 func, const u32 addr, const u8 in);
