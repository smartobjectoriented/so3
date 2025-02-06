/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2016        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include <common.h>
#include <part.h>

#include <fat/diskio.h> /* FatFs lower layer API */

/* Definitions of physical drive number for each drive */
#define DEV_RAM 0 /* Example: Map Ramdisk to physical drive 0 */
#define DEV_MMC 1 /* Example: Map MMC/SD card to physical drive 1 */
#define DEV_USB 2 /* Example: Map USB MSD to physical drive 2 */

struct block_drvr {
	char *name;
	block_dev_desc_t *dev_desc;
	block_dev_desc_t *(*get_dev)(int dev);
};

/*
 * Basically, we manage either a rootfs in a MMC - or - in a ramdev (in RAM).
 * This is exclusve. We are currently not able to manage the two.
 */
static struct block_drvr block_drvr[] = {
#ifdef CONFIG_ROOTFS_MMC
	{
		.name = "mmc",
		.get_dev = mmc_get_dev,
	},
#endif
#ifdef CONFIG_ROOTFS_RAMDEV
	{
		.name = "ramdev",
		.get_dev = ramdev_get_dev,
	},
#endif
	{},
};

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status(BYTE pdrv /* Physical drive nmuber to identify the drive */
)
{
	return 0;
}

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS
disk_initialize(BYTE pdrv /* Physical drive nmuber to identify the drive */
)
{
	if (pdrv >= ARRAY_SIZE(block_drvr)) {
		DBG("Device %d does not exists\n", pdrv);
		return STA_NODISK;
	}

	DBG("Opening device %s\n", block_drvr[pdrv].name);
	block_drvr[pdrv].dev_desc = block_drvr[pdrv].get_dev(pdrv);

	if (!block_drvr[pdrv].dev_desc) {
		return STA_NOINIT;
	}

	return 0;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read(BYTE pdrv, /* Physical drive nmuber to identify the drive */
		  BYTE *buff, /* Data buffer to store read data */
		  DWORD sector, /* Start sector in LBA */
		  UINT count /* Number of sectors to read */
)
{
	block_dev_desc_t *drvr;

	if (pdrv >= ARRAY_SIZE(block_drvr)) {
		DBG("Device %d does not exists\n", pdrv);
		return STA_NODISK;
	}

	if (!block_drvr[pdrv].dev_desc) {
		return RES_PARERR;
	}

	drvr = block_drvr[pdrv].dev_desc;

	if (!drvr->block_read(drvr->dev, sector, count, buff)) {
		return -1;
	}

	return 0;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT disk_write(BYTE pdrv, /* Physical drive nmuber to identify the drive */
		   const BYTE *buff, /* Data to be written */
		   DWORD sector, /* Start sector in LBA */
		   UINT count /* Number of sectors to write */
)
{
	block_dev_desc_t *drvr;

	if (pdrv >= ARRAY_SIZE(block_drvr)) {
		DBG("Device %d does not exists\n", pdrv);
		return STA_NODISK;
	}

	if (!block_drvr[pdrv].dev_desc) {
		return RES_PARERR;
	}

	drvr = block_drvr[pdrv].dev_desc;

	if (!drvr->block_write(drvr->dev, sector, count, buff)) {
		return -1;
	}

	return 0;
}

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl(BYTE pdrv, /* Physical drive nmuber (0..) */
		   BYTE cmd, /* Control code */
		   void *buff /* Buffer to send/receive control data */
)
{
	DBG("IOCLT %d %p\n", cmd, buff);
	/* As for now, this is not in use */
	return 0;
}
