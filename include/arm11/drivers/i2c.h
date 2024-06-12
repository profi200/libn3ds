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
#define I2C_STOP            BIT(0)
#define I2C_START           BIT(1)
#define I2C_ERROR           BIT(2)
#define I2C_ACK             BIT(4)
#define I2C_DIR_S           (0u)    // Direction send.
#define I2C_DIR_R           BIT(5)  // Direction receive.
#define I2C_IRQ_EN          BIT(6)
#define I2C_EN              BIT(7)

// REG_I2C_CNTEX
#define I2C_SCL_STATE_MASK  BIT(0)  // Read-only SCL line state?
#define I2C_CLK_STRETCH_EN  BIT(1)  // Enables clock stretching.
#define I2C_UNK_CNTEX15     BIT(15) // "LGCY" Legacy related?

// REG_I2C_SCL
#define I2C_DELAYS(high, low)  ((high)<<8 | (low)) // "PRD" TODO: How long and when does it delay?


typedef enum
{
	I2C_DEV_TWL_MCU =  0u, // DSi mode MCU.
	I2C_DEV_CAMERA1 =  1u, // Internal self-facing camera.
	I2C_DEV_CAMERA2 =  2u, // External right camera.
	I2C_DEV_CTR_MCU =  3u, // 3DS mode MCU.
	I2C_DEV_CAMERA3 =  4u, // External left camera.
	I2C_DEV_LCD1    =  5u, // Upper LCD.
	I2C_DEV_LCD2    =  6u, // Lower LCD.
	I2C_DEV_UNK7    =  7u, // Debug?
	I2C_DEV_UNK8    =  8u, // Debug?
	I2C_DEV_GYRO1   =  9u, // Unknown Gyroscope vendor.
	I2C_DEV_GYRO2   = 10u, // Old 3DS only? Invensense ITG-3270 MEMS Gyroscope.
	I2C_DEV_GYRO3   = 11u, // New 3DS only? Unknown Gyroscope vendor.
	I2C_DEV_UNK12   = 12u, // HID "DebugPad"?
	I2C_DEV_IR      = 13u, // NXP SC16IS750 infrared transmitter/receiver.
	I2C_DEV_EEPROM  = 14u, // Dev unit only. Hardware calibration data.
	I2C_DEV_NFC     = 15u, // Broadcom BCM20791 (New 2/3DS only).
	I2C_DEV_IO_EXP  = 16u, // Texas Instruments TCA6416A I/O expander (New 3DS only).
	I2C_DEV_EXTHID  = 17u  // C-Stick and ZL/ZR buttons (New 2/3DS only).
} I2cDevice;

#define I2C_NO_REG_VAL  (0x100u)



/**
 * @brief      Initializes the I²C buses. Call this only once.
 */
void I2C_init(void);

/**
 * @brief      Reads data from a device via I²C and stores it in a buffer.
 *
 * @param[in]  devId    The device ID.
 * @param[in]  regAddr  The start register address. If I2C_NO_REG_VAL use direct transfer (no register).
 * @param      out      The output buffer pointer.
 * @param[in]  size     The buffer size.
 *
 * @return     Returns true on success and false on failure.
 */
bool I2C_readArray(const I2cDevice devId, const u32 regAddr, void *out, u32 size);

/**
 * @brief      Writes data from a buffer to a device via I²C.
 *
 * @param[in]  devId    The device ID.
 * @param[in]  regAddr  The start register address. If I2C_NO_REG_VAL use direct transfer (no register).
 * @param[in]  in       The input buffer pointer.
 * @param[in]  size     The buffer size.
 *
 * @return     Returns true on success and false on failure.
 */
bool I2C_writeArray(const I2cDevice devId, const u32 regAddr, const void *in, u32 size);

/**
 * @brief      Reads a byte from a device via I²C.
 *
 * @param[in]  devId    The device ID.
 * @param[in]  regAddr  The register address. If I2C_NO_REG_VAL use direct transfer (no register).
 *
 * @return     Returns the register data on success otherwise 0xFF.
 */
u8 I2C_read(const I2cDevice devId, const u32 regAddr);

/**
 * @brief      Writes a byte to a device via I²C.
 *
 * @param[in]  devId    The device ID.
 * @param[in]  regAddr  The register address. If I2C_NO_REG_VAL use direct transfer (no register).
 * @param[in]  data     The data to write.
 *
 * @return     Returns true on success and false on failure.
 */
bool I2C_write(const I2cDevice devId, const u32 regAddr, const u8 data);
// ---------------------------------------------------------------- //

/**
 * @brief      Writes data from a buffer to an array of registers via I²C without interrupts.
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
 * @brief      Writes a byte to a register via I²C without interrupts.
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