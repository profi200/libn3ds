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

#define TIMER_REGS_BASE  (MPCORE_PRIV_BASE + 0x600)

typedef struct
{
	vu32 load;          // 0x00
	vu32 counter;       // 0x04
	vu32 cnt;           // 0x08
	vu32 int_stat;      // 0x0C
	u8 _0x10[0x10];
	vu32 wd_load;       // 0x20
	vu32 wd_counter;    // 0x24
	vu32 wd_cnt;        // 0x28
	vu32 wd_int_stat;   // 0x2C
	vu32 wd_reset_stat; // 0x30
	vu32 wd_disable;    // 0x34
} Timer;
static_assert(offsetof(Timer, wd_disable) == 0x34, "Error: Member wd_disable of Timer is not at offset 0x34!");

ALWAYS_INLINE Timer* getTimerRegs(void)
{
	return (Timer*)TIMER_REGS_BASE;
}


// REG_TIMER_CNT/REG_WD_CNT
#define TIMER_EN           BIT(0)
#define TIMER_SINGLE_SHOT  (0u)
#define TIMER_AUTO_RELOAD  BIT(1)
#define TIMER_IRQ_EN       BIT(2)
#define WD_TIMER_MODE      (0u)   // Watchdog only. Watchdog in timer mode.
#define WD_WD_MODE         BIT(3) // Watchdog only. Watchdog in watchdog mode.
#define TIMER_PRESC_SHIFT  (8u)   // Shift helper for the prescalers.

// REG_WD_DISABLE
#define WD_DISABLE_MAGIC1  (0x12345678u)
#define WD_DISABLE_MAGIC2  (0x87654321u)


#define TIMER_BASE_FREQ    (268111856u / 2)

// p is the prescaler value and n the frequency.
// Note: With highest prescaler and sub-microsecond frequency
//       this may give wrong results due to overflows.
#define TIMER_FREQ(p, f)   (TIMER_BASE_FREQ / ((p) * (f)))



/**
 * @brief      Resets/initializes the timer hardware. Should not be called manually.
 */
void TIMER_init(void);

/**
 * @brief      Starts the timer.
 *
 * @param[in]  prescaler  The prescaler (1-256).
 * @param[in]  ticks      The initial number of ticks. This is also the reload
 *                        value in auto reload mode.
 * @param[in]  params     The parameters. See /regs/timer.h "REG_TIMER_CNT" defines.
 */
void TIMER_start(const u16 prescaler, const u32 ticks, const u8 params);

/**
 * @brief      Returns the current number of ticks of the timer.
 *
 * @return     The number of ticks.
 */
u32 TIMER_getTicks(void);

/**
 * @brief      Stops the timer and returns the current number of ticks.
 *
 * @return     The number of ticks.
 */
u32 TIMER_stop(void);

/**
 * @brief      Halts the CPU for ms amount of milliseconds.
 *
 * @param[in]  ms    Number of milliseconds to sleep. Up to 32038.
 */
void TIMER_sleepMs(const u32 ms);

/**
 * @brief      Halts the CPU for us amount of microseconds.
 *
 * @param[in]  us    Number of microseconds to sleep. Up to 32038622.
 */
void TIMER_sleepUs(const u32 us);

/**
 * @brief      Halts the CPU for ns amount of nanoseconds.
 *
 * @param[in]  ns    Number of nanoseconds to sleep. Up to 32038622678.
 */
void TIMER_sleepNs(const u64 ns);

#ifdef __cplusplus
} // extern "C"
#endif