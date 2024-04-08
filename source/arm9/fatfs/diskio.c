/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "fatfs/source/ff.h"			/* Obtains integer types */
#include "fatfs/source/diskio.h"		/* Declarations of disk functions */
#include "types.h"
#include "drivers/tmio.h"
#include "drivers/mmc/sdmmc.h"
#include "arm9/drivers/ndma.h"
#include "drivers/cache.h"
#include "arm9/drivers/timer.h"



/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	(void)pdrv;

	return SDMMC_getDiskStatus(SDMMC_DEV_CARD);
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive number to identify the drive */
)
{
	(void)pdrv;

	// Workaround for card detect time.
	unsigned timeout = 5;
	while(!TMIO_cardDetected() && timeout > 0)
	{
		TIMER_sleepMs(2);
		timeout--;
	}

	if(timeout == 0)
		return STA_NODISK | STA_NOINIT;

	return (SDMMC_init(SDMMC_DEV_CARD) == SDMMC_ERR_NONE ? 0 : STA_NOINIT);
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	(void)pdrv;

	// TODO: FatFs will use memcpy() for sizes under 512 bytes
	//       but larger transfers run into this check if unaligned.
	DRESULT res = RES_OK;
	if((uintptr_t)buff % 4 == 0)
	{
		// Warning! Flush before transfer only works on ARM9 (no speculative prefetching)!
		flushDCacheRange(buff, 512 * count);

		NdmaCh *const ndmaCh = getNdmaChRegs(5);
		ndmaCh->sad  = (u32)getTmioFifo(getTmioRegs(1)); // TODO: SDMMC dev to FIFO function.
		ndmaCh->dad  = (u32)buff;
		ndmaCh->wcnt = 512 / 4;
		ndmaCh->bcnt = NDMA_FASTEST;
		ndmaCh->cnt  = NDMA_EN | NDMA_START_TMIO3 | NDMA_REPEAT_MODE |
		               NDMA_BURST(64 / 4) | NDMA_SAD_FIX | NDMA_DAD_INC;

		do
		{
			const u16 blockCount = (count > 0xFFFF ? 0xFFFF : count);
			if(SDMMC_readSectors(SDMMC_DEV_CARD, sector, NULL, blockCount) != SDMMC_ERR_NONE)
			{
				res = RES_ERROR;
				break;
			}

			sector += blockCount;
			count -= blockCount;
		} while(count > 0);

		// Stop DMA.
		ndmaCh->cnt = 0;
	}
	else res = RES_PARERR;

	return res;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	(void)pdrv;

	// TODO: FatFs will use memcpy() for sizes under 512 bytes
	//       but larger transfers run into this check if unaligned.
	DRESULT res = RES_OK;
	if((uintptr_t)buff % 4 == 0)
	{
		flushDCacheRange(buff, 512 * count);

		NdmaCh *const ndmaCh = getNdmaChRegs(5);
		ndmaCh->sad  = (u32)buff;
		ndmaCh->dad  = (u32)getTmioFifo(getTmioRegs(1)); // TODO: SDMMC dev to FIFO function.
		ndmaCh->wcnt = 512 / 4;
		ndmaCh->bcnt = NDMA_FASTEST;
		ndmaCh->cnt  = NDMA_EN | NDMA_START_TMIO3 | NDMA_REPEAT_MODE |
		               NDMA_BURST(64 / 4) | NDMA_SAD_INC | NDMA_DAD_FIX;

		do
		{
			const u16 blockCount = (count > 0xFFFF ? 0xFFFF : count);
			if(SDMMC_writeSectors(SDMMC_DEV_CARD, sector, NULL, blockCount) != SDMMC_ERR_NONE)
			{
				res = RES_ERROR;
				break;
			}

			sector += blockCount;
			count -= blockCount;
		} while(count > 0);

		// Stop DMA.
		ndmaCh->cnt = 0;

		// NDMA hardware bug workaround.
		(void)*((const vu8*)buff);
	}
	else res = RES_PARERR;

	return res;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	(void)pdrv;

	DRESULT res = RES_OK;
	switch(cmd)
	{
		case GET_SECTOR_COUNT:
			*(DWORD*)buff = SDMMC_getSectors(SDMMC_DEV_CARD);
			break;
		case GET_SECTOR_SIZE:
			*(WORD*)buff = 512;
			break;
		case GET_BLOCK_SIZE:
			*(DWORD*)buff = 0x100; // Default to 128 KiB. TODO: Get this from the driver.
		case CTRL_TRIM:
			// TODO: Implement this.
		case CTRL_SYNC:
			break;
		default:
			res = RES_PARERR;
	}

	return res;
}