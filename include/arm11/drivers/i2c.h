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

#include "types.h"
#include "mem_map.h"


#ifdef __cplusplus
extern "C"
{
#endif

#define I2C1_REGS_BASE  (IO_COMMON_BASE + 0x61000)
#define I2C2_REGS_BASE  (IO_COMMON_BASE + 0x44000)
#define I2C3_REGS_BASE  (IO_COMMON_BASE + 0x48000)

typedef struct
{
	vu8  data;  // 0x0
	vu8  cnt;   // 0x1
	vu16 cntex; // 0x2
	vu16 scl;   // 0x4
} I2cBus;
static_assert(offsetof(I2cBus, scl) == 4, "Error: Member scl of I2cBus is not at offset 4!");

enum
{
	I2C_BUS1 = 0u,
	I2C_BUS2 = 1u,
	I2C_BUS3 = 2u
};

ALWAYS_INLINE I2cBus* getI2cBusRegs(const u8 busId)
{
	switch(busId)
	{
		case I2C_BUS1:
			return (I2cBus*)I2C1_REGS_BASE;
		case I2C_BUS2:
			return (I2cBus*)I2C2_REGS_BASE;
		case I2C_BUS3:
			return (I2cBus*)I2C3_REGS_BASE;
		default:
			return NULL;
	}
}


// REG_I2C_CNT
#define I2C_STOP           (1u)
#define I2C_START          (1u<<1)
#define I2C_ERROR          (1u<<2)
#define I2C_ACK            (1u<<4)
#define I2C_DIR_S          (0u)    // Direction send.
#define I2C_DIR_R          (1u<<5) // Direction receive.
#define I2C_IRQ_EN         (1u<<6)
#define I2C_EN             (1u<<7)

// REG_I2C_CNTEX
#define I2C_SCL_STATE_MASK (1u)     // Read-only SCL line state?
#define I2C_CLK_STRETCH_EN (1u<<1)  // Enables clock stretching.
#define I2C_UNK_CNTEX15    (1u<<15) // "LGCY" Legacy related?

// REG_I2C_SCL
#define I2C_DELAYS(high, low)  ((high)<<8 | (low)) // "PRD" TODO: How long and when does it delay?


typedef enum
{
	I2C_DEV_TWL_MCU   =  0u, // DSi mode MCU
	I2C_DEV_CAMERA1   =  1u, // Internal self-facing camera
	I2C_DEV_CAMERA2   =  2u, // External right camera
	I2C_DEV_CTR_MCU   =  3u,
	I2C_DEV_CAMERA3   =  4u, // External left camera
	I2C_DEV_LCD0      =  5u, // Upper LCD
	I2C_DEV_LCD1      =  6u, // Lower LCD
	I2C_DEV_UNK7      =  7u, // Debug?
	I2C_DEV_UNK8      =  8u, // Debug?
	I2C_DEV_UNK9      =  9u, // HID debug?
	I2C_DEV_GYRO_OLD  = 10u, // Old 3DS only?
	I2C_DEV_GYRO_NEW  = 11u, // New 3DS only?
	I2C_DEV_UNK12     = 12u, // HID "DebugPad"?
	I2C_DEV_IR        = 13u, // Infrared (IrDA)
	I2C_DEV_EEPROM    = 14u, // Dev unit only?
	I2C_DEV_NFC       = 15u,
	I2C_DEV_QTM       = 16u, // IO expander chip (New 3DS only)
	I2C_DEV_N3DS_HID  = 17u  // C-Stick and ZL/ZR buttons
} I2cDevice;



/**
 * @brief      Initializes the I2C buses. Call this only once.
 */
void I2C_init(void);

/**
 * @brief      Reads data from multiple registers to a buffer via I2C.
 *
 * @param[in]  devId    The device ID.
 * @param[in]  regAddr  The start register address.
 * @param      out      The output buffer pointer.
 * @param[in]  size     The buffer size.
 *
 * @return     Returns true on success and false on failure.
 */
bool I2C_readRegArray(const I2cDevice devId, const u8 regAddr, void *out, u32 size);

bool I2C_readArray(const I2cDevice devId, void *out, u32 size);

/**
 * @brief      Writes an array of bytes to an array of registers via I2C.
 *
 * @param[in]  devId    The device ID.
 * @param[in]  regAddr  The start register address.
 * @param[in]  in       The input buffer pointer.
 * @param[in]  size     The buffer size.
 *
 * @return     Returns true on success and false on failure.
 */
bool I2C_writeRegArray(const I2cDevice devId, const u8 regAddr, const void *in, u32 size);

/**
 * @brief      Reads a byte from a register via I2C.
 *
 * @param[in]  devId    The device ID.
 * @param[in]  regAddr  The register address.
 *
 * @return     Returns the register data on success otherwise 0xFF.
 */
u8 I2C_readReg(const I2cDevice devId, const u8 regAddr);

/**
 * @brief      Writes a byte to a register via I2C.
 *
 * @param[in]  devId    The device ID.
 * @param[in]  regAddr  The register address.
 * @param[in]  data     The data to write.
 *
 * @return     Returns true on success and false on failure.
 */
bool I2C_writeReg(const I2cDevice devId, const u8 regAddr, const u8 data);
// ---------------------------------------------------------------- //

/**
 * @brief      Writes an array of bytes to an array of registers via I2C without interrupts.
 *
 * @param[in]  devId    The device ID. Use the enum above.
 * @param[in]  regAddr  The register address.
 * @param[in]  in       The input buffer pointer.
 * @param[in]  size     The buffer size.
 *
 * @return     Returns true on success and false on failure.
 */
bool I2C_writeRegArrayIntSafe(const I2cDevice devId, const u8 regAddr, const void *in, u32 size);

/**
 * @brief      Writes a byte to a register via I2C without interrupts.
 *
 * @param[in]  devId    The device ID. Use the enum above.
 * @param[in]  regAddr  The register address.
 * @param[in]  data     The data to write.
 *
 * @return     Returns true on success and false on failure.
 */
bool I2C_writeRegIntSafe(const I2cDevice devId, const u8 regAddr, const u8 data);

#ifdef __cplusplus
} // extern "C"
#endif