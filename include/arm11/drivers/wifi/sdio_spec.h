#pragma once

// Controller specific macros. Add controller specific bits here.
// SDIO_CMD_[response type]_[transfer type]
// Transfer type: R = read, W = write, EX = read or write.
#define SDIO_CMD_NONE(id)      (CMD_RESP_NONE | (id))
#define SDIO_CMD_R1(id)        (CMD_RESP_R1   | (id))
#define SDIO_CMD_R1b(id)       (CMD_RESP_R1b  | (id))
#define SDIO_CMD_R4(id)        (CMD_RESP_R4   | (id))
#define SDIO_CMD_R5(id)        (CMD_RESP_R5   | (id))
#define SDIO_CMD_R6(id)        (CMD_RESP_R6   | (id))
#define SDIO_CMD_R7(id)        (CMD_RESP_R7   | (id))
#define SDIO_CMD_R1_R(id)      (CMD_DIR_R | CMD_DT_EN | CMD_RESP_R1 | (id))
#define SDIO_CMD_R5_EX(id, r)  (CMD_SEC_SDIO | ((r) ? CMD_DIR_R : CMD_DIR_W) | CMD_DT_EN | CMD_RESP_R5 | (id))

#define SDIO_GO_IDLE_STATE        SDIO_CMD_NONE(0u)      //   -, [31:0] stuff bits.
#define SDIO_SEND_RELATIVE_ADDR     SDIO_CMD_R6(3u)      //  R6, [31:0] stuff bits.
#define SDIO_IO_SEND_OP_COND        SDIO_CMD_R4(5u)      //  R4, [31:25] stuff bits [24:24] S18R [23:0] I/O OCR.
#define SDIO_SELECT_CARD           SDIO_CMD_R1b(7u)      // R1b, [31:16] RCA [15:0] stuff bits.
#define SDIO_DESELECT_CARD        SDIO_CMD_NONE(7u)      //   -, [31:16] RCA [15:0] stuff bits.
#define SDIO_SEND_IF_COND           SDIO_CMD_R7(8u)      //  R7, [31:12] reserved bits [11:8] supply voltage (VHS) [7:0] check pattern.
#define SDIO_VOLTAGE_SWITCH        SDIO_CMD_R1(11u)      //  R1, [31:0] reserved bits (all 0).
#define SDIO_SEND_TUNING_BLOCK   SDIO_CMD_R1_R(19u)      //  R1, [31:0] reserved bits (all 0).
#define SDIO_IO_RW_DIRECT          SDIO_CMD_R5(52u)      //  R5, TODO.
#define SDIO_IO_RW_EXTENDED(r)  SDIO_CMD_R5_EX(53u, (r)) //  R5, TODO.


// 4.10.8 Card Status Register.
// Type:
// E: Error bit.
// S: Status bit.
// R: Detected and set for the actual command response.
// X: Detected and set during command execution. The host can get the status by issuing a command with R1 response.
//
// Clear Condition:
// A: According to the card current status.
// B: Always related to the previous command. Reception of a valid command will clear it (with a delay of one command).
// C: Clear by read.
#define SDIO_R1_STATE_IO_ONLY    (15u<<9) //   S X B
#define SDIO_R1_ERROR            (1u<<19) // E R X C, A general or an unknown error occurred during the operation.
#define SDIO_R1_ILLEGAL_COMMAND  (1u<<22) //   E R B, Previous command not legal for the card state.
#define SDIO_R1_COM_CRC_ERROR    (1u<<23) //   E R B, The CRC check of the previous command failed.
#define SDIO_R1_OUT_OF_RANGE     (1u<<31) // E R X C, The command's argument was out of the allowed range for this card.

#define SDIO_R1_ERR_ALL          (SDIO_R1_OUT_OF_RANGE | SDIO_R1_COM_CRC_ERROR | SDIO_R1_ILLEGAL_COMMAND | SDIO_R1_ERROR)

// 5.2 IO_RW_DIRECT Response (R5).
// Also for IO_RW_EXTENDED.
// For type and clear condition see Card Status Register above.
#define SDIO_R5_OUT_OF_RANGE     (1u<<8)  // E R X C, ER: The command's argument was out of the allowed range for this card. EX: Out of range occurs during execution of CMD53.
#define SDIO_R5_FUNCTION_NUMBER  (1u<<9)  //   E R C, An invalid function number was requested.
#define SDIO_R5_ERROR            (1u<<11) // E R X C, E R X for CMD53. A general or an unknown error occurred during the operation.
#define SDIO_R5_STATE_DIS        (0u<<12) //     S B, DIS=Disabled: Initialize, Standby and Inactive States (card not selected).
#define SDIO_R5_STATE_CMD        (1u<<12) //     S B, CMD=DAT lines free: 1. Command waiting (No transaction suspended). 2. Command waiting (All CMD53 transactions suspended). 3. Executing CMD52 in CMD State.
#define SDIO_R5_STATE_TRN        (2u<<12) //     S B, TRN=Transfer: Command executing with data transfer using DAT[0] or DAT[3:0] lines.
#define SDIO_R5_STATE_RFU        (3u<<12) //     S B
#define SDIO_R5_ILLEGAL_COMMAND  (1u<<14) //   E R B, Command not legal for the card State.
#define SDIO_R5_COM_CRC_ERROR    (1u<<15) //   E R B, The CRC check of the previous command failed.

// Table 3-1 : OCR Values for CMD5.
#define SDIO_OCR_2_0_2_1V    (1u<<8)  // 2.0-2.1V.
#define SDIO_OCR_2_1_2_2V    (1u<<9)  // 2.1-2.2V.
#define SDIO_OCR_2_2_2_3V    (1u<<10) // 2.2-2.3V.
#define SDIO_OCR_2_3_2_4V    (1u<<11) // 2.3-2.4V.
#define SDIO_OCR_2_4_2_5V    (1u<<12) // 2.4-2.5V.
#define SDIO_OCR_2_5_2_6V    (1u<<13) // 2.5-2.6V.
#define SDIO_OCR_2_6_2_7V    (1u<<14) // 2.6-2.7V.
#define SDIO_OCR_2_7_2_8V    (1u<<15) // 2.7-2.8V.
#define SDIO_OCR_2_8_2_9V    (1u<<16) // 2.8-2.9V.
#define SDIO_OCR_2_9_3_0V    (1u<<17) // 2.9-3.0V.
#define SDIO_OCR_3_0_3_1V    (1u<<18) // 3.0-3.1V.
#define SDIO_OCR_3_1_3_2V    (1u<<19) // 3.1-3.2V.
#define SDIO_OCR_3_2_3_3V    (1u<<20) // 3.2-3.3V.
#define SDIO_OCR_3_3_3_4V    (1u<<21) // 3.3-3.4V.
#define SDIO_OCR_3_4_3_5V    (1u<<22) // 3.4-3.5V.
#define SDIO_OCR_3_5_3_6V    (1u<<23) // 3.5-3.6V.
// The following bits are not listed as being part of the OCR
// but they are used together in the OCR response.
#define SDIO_S18A            (1u<<24) // Switching to 1.8V Accepted (Supported in SD mode only).
#define SDIO_MEMORY_PRESENT  (1u<<27) // Set to 1 if the card also contains SD memory. Set to 0 if the card is I/O only.
// TODO: 29-30 Number of I/O Functions.
#define SDIO_READY           (1u<<31) // Set to 1 if Card is ready to operate after initialization.

// Argument bits for IO_SEND_OP_COND (CMD5).
// For voltage bits see OCR register above.
#define SDIO_CMD5_S18R  (1u<<24) // Switching to 1.8V Request.

// TODO: CMD52/53 arg bits.
