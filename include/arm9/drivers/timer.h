#pragma once

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

#include "types.h"
#include "mem_map.h"


#define TIMER_REGS_BASE  (IO_AHB_BASE + 0x3000)

typedef struct
{
	vu16 val; // 0x0
	vu16 cnt; // 0x2
} Timer;
static_assert(offsetof(Timer, cnt) == 2, "Error: Member cnt of Timer is not at offset 2!");

ALWAYS_INLINE Timer* getTimerRegs(u8 timer)
{
	return &((Timer*)TIMER_REGS_BASE)[timer];
}


#define TIMER_BASE_FREQ     (67027964)

// REG_TIMER_CNT
#define TIMER_PRESC_1       (0u)
#define TIMER_PRESC_64      (1u)
#define TIMER_PRESC_256     (2u)
#define TIMER_PRESC_1024    (3u)
#define TIMER_COUNT_UP      (1u<<2) // For cascading at least 2 timers.
#define TIMER_IRQ_EN        (1u<<6)
#define TIMER_EN            (1u<<7)

// Convenience macros for calculating the ticks. Based on libnds.
#define TIMER_FREQ(f)       (-TIMER_BASE_FREQ / (f))
#define TIMER_FREQ_64(f)    TIMER_FREQ(64 * (f))
#define TIMER_FREQ_256(f)   TIMER_FREQ(256 * (f))
#define TIMER_FREQ_1024(f)  TIMER_FREQ(1024 * (f))



/**
 * @brief      Resets/initializes the timer hardware. Should not be called manually.
 */
void TIMER_init(void);

/**
 * @brief      Starts a timer.
 *
 * @param[in]  tmr     The timer to start (0-3). Timer 3 is reserved.
 * @param[in]  ticks   The initial number of ticks. This is also the reload
 *                     value on overflow.
 * @param[in]  params  The parameters. See REG_TIMER_CNT defines above.
 */
void TIMER_start(const u8 tmr, const u16 ticks, const u8 params);

/**
 * @brief      Returns the current number of ticks of the timer.
 *
 * @param[in]  tmr   The timer get the ticks from (0-3). Timer 3 is reserved.
 *
 * @return     The number of ticks.
 */
u16 TIMER_getTicks(const u8 tmr);

/**
 * @brief      Stops a timer and returns the current number of ticks.
 *
 * @param[in]  tmr   The timer to stop (0-3). Timer 3 is reserved.
 *
 * @return     The number of ticks.
 */
u16 TIMER_stop(const u8 tmr);

/**
 * @brief      Halts the CPU for the given number of milliseconds.
 *
 * @param[in]  ms    The number of milliseconds to sleep.
 */
void TIMER_sleepMs(const u32 ms);