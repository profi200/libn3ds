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

#include "arm.h"
#include "types.h"


#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
{
	IRQ_DMAC_1_0      =  0u, // DMAC_1 =  NDMA
	IRQ_DMAC_1_1      =  1u,
	IRQ_DMAC_1_2      =  2u,
	IRQ_DMAC_1_3      =  3u,
	IRQ_DMAC_1_4      =  4u,
	IRQ_DMAC_1_5      =  5u,
	IRQ_DMAC_1_6      =  6u,
	IRQ_DMAC_1_7      =  7u,
	IRQ_TIMER_0       =  8u,
	IRQ_TIMER_1       =  9u,
	IRQ_TIMER_2       = 10u,
	IRQ_TIMER_3       = 11u,
	IRQ_PXI_SYNC      = 12u,
	IRQ_PXI_NOT_FULL  = 13u,
	IRQ_PXI_NOT_EMPTY = 14u,
	IRQ_AES           = 15u,
	IRQ_TMIO1         = 16u,
	IRQ_TMIO1_IRQ     = 17u,
	IRQ_TMIO3         = 18u,
	IRQ_TMIO3_IRQ     = 19u,
	IRQ_DEBUG_RECV    = 20u,
	IRQ_DEBUG_SEND    = 21u,
	IRQ_RSA           = 22u,
	IRQ_CTR_CARD_1    = 23u, // SPICARD and CTRCARD too?
	IRQ_CTR_CARD_2    = 24u,
	IRQ_CGC           = 25u,
	IRQ_CGC_DET       = 26u,
	IRQ_DS_CARD       = 27u,
	IRQ_DMAC_2        = 28u,
	IRQ_DMAC_2_ABORT  = 29u
} Interrupt;


// IRQ interrupt service routine pointer type.
// id is the interrupt ID.
typedef void (*IrqIsr)(u32 id);



/**
 * @brief      Initializes the interrupt controller. For libn3ds internal usage only.
 */
void IRQ_init(void);

/**
 * @brief      Registers an interrupt service routine and enables the IRQ.
 *
 * @param[in]  id    The interrupt ID. Must be one of the above IDs.
 * @param[in]  isr   The interrupt service routine to call.
 */
void IRQ_registerIsr(const Interrupt id, const IrqIsr isr);

/**
 * @brief      Enables a disabled but registered interrupt.
 *
 * @param[in]  id    The interrupt ID. Must be one of the above IDs.
 */
void IRQ_enable(const Interrupt id);

/**
 * @brief      Disables a registered interrupt.
 *
 * @param[in]  id    The interrupt ID. Must be one of the above IDs.
 */
void IRQ_disable(const Interrupt id);

/**
 * @brief      Unregisters the interrupt service routine and disables the IRQ.
 *
 * @param[in]  id    The interrupt ID. Must be one of the above IDs.
 */
void IRQ_unregisterIsr(const Interrupt id);

#if !__thumb__
/**
 * @brief      Saves the CPU state and disables IRQs.
 *
 * @return     The CPU state before IRQ disable.
 */
static inline u32 enterCriticalSection(void)
{
	const u32 cpsr = __getCpsr();
	__setCpsr_c(cpsr | PSR_I);
	return cpsr;
}

/**
 * @brief      Restores a saved CPU state. Whenever IRQs are enabled again depends on the state.
 *
 * @param[in]  savedState  The CPU state to restore.
 */
static inline void leaveCriticalSection(const u32 savedState)
{
	__setCpsr_c(savedState);
}
#endif

#ifdef __cplusplus
} // extern "C"
#endif