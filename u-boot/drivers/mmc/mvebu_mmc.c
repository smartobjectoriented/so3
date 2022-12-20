// SPDX-License-Identifier: GPL-2.0+
/*
 * Marvell MMC/SD/SDIO driver
 *
 * (C) Copyright 2012-2014
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Maen Suleiman, Gerald Kerma
 */

#include <common.h>
#include <errno.h>
#include <log.h>
#include <malloc.h>
#include <dm.h>
#include <fdtdec.h>
#include <part.h>
#include <mmc.h>
#include <asm/io.h>
#include <asm/arch/cpu.h>
#include <asm/arch/soc.h>
#include <mvebu_mmc.h>
#include <dm/device_compat.h>

#define MVEBU_TARGET_DRAM 0

#define TIMEOUT_DELAY	5*CONFIG_SYS_HZ		/* wait 5 seconds */

static inline void *get_regbase(const struct mmc *mmc)
{
	struct mvebu_mmc_plat *pdata = mmc->priv;

	return pdata->iobase;
}

static void mvebu_mmc_write(const struct mmc *mmc, u32 offs, u32 val)
{
	writel(val, get_regbase(mmc) + (offs));
}

static u32 mvebu_mmc_read(const struct mmc *mmc, u32 offs)
{
	return readl(get_regbase(mmc) + (offs));
}

static int mvebu_mmc_setup_data(struct udevice *dev, struct mmc_data *data)
{
	struct mvebu_mmc_plat *pdata = dev_get_plat(dev);
	struct mmc *mmc = &pdata->mmc;
	u32 ctrl_reg;

	dev_dbg(dev, "data %s : blocks=%d blksz=%d\n",
		(data->flags & MMC_DATA_READ) ? "read" : "write",
		data->blocks, data->blocksize);

	/* default to maximum timeout */
	ctrl_reg = mvebu_mmc_read(mmc, SDIO_HOST_CTRL);
	ctrl_reg |= SDIO_HOST_CTRL_TMOUT(SDIO_HOST_CTRL_TMOUT_MAX);
	mvebu_mmc_write(mmc, SDIO_HOST_CTRL, ctrl_reg);

	if (data->flags & MMC_DATA_READ) {
		mvebu_mmc_write(mmc, SDIO_SYS_ADDR_LOW, (u32)data->dest & 0xffff);
		mvebu_mmc_write(mmc, SDIO_SYS_ADDR_HI, (u32)data->dest >> 16);
	} else {
		mvebu_mmc_write(mmc, SDIO_SYS_ADDR_LOW, (u32)data->src & 0xffff);
		mvebu_mmc_write(mmc, SDIO_SYS_ADDR_HI, (u32)data->src >> 16);
	}

	mvebu_mmc_write(mmc, SDIO_BLK_COUNT, data->blocks);
	mvebu_mmc_write(mmc, SDIO_BLK_SIZE, data->blocksize);

	return 0;
}

static int mvebu_mmc_send_cmd(struct udevice *dev, struct mmc_cmd *cmd,
			      struct mmc_data *data)
{
	ulong start;
	ushort waittype = 0;
	ushort resptype = 0;
	ushort xfertype = 0;
	ushort resp_indx = 0;
	struct mvebu_mmc_plat *pdata = dev_get_plat(dev);
	struct mmc *mmc = &pdata->mmc;

	dev_dbg(dev, "cmdidx [0x%x] resp_type[0x%x] cmdarg[0x%x]\n",
		cmd->cmdidx, cmd->resp_type, cmd->cmdarg);

	dev_dbg(dev, "cmd %d (hw state 0x%04x)\n",
		cmd->cmdidx, mvebu_mmc_read(mmc, SDIO_HW_STATE));

	/*
	 * Hardware weirdness.  The FIFO_EMPTY bit of the HW_STATE
	 * register is sometimes not set before a while when some
	 * "unusual" data block sizes are used (such as with the SWITCH
	 * command), even despite the fact that the XFER_DONE interrupt
	 * was raised.  And if another data transfer starts before
	 * this bit comes to good sense (which eventually happens by
	 * itself) then the new transfer simply fails with a timeout.
	 */
	if (!(mvebu_mmc_read(mmc, SDIO_HW_STATE) & CMD_FIFO_EMPTY)) {
		ushort hw_state, count = 0;

		start = get_timer(0);
		do {
			hw_state = mvebu_mmc_read(mmc, SDIO_HW_STATE);
			if ((get_timer(0) - start) > TIMEOUT_DELAY) {
				printf("%s : FIFO_EMPTY bit missing\n",
				       dev->name);
				break;
			}
			count++;
		} while (!(hw_state & CMD_FIFO_EMPTY));
		dev_dbg(dev, "*** wait for FIFO_EMPTY bit (hw=0x%04x, count=%d, jiffies=%ld)\n",
			hw_state, count, (get_timer(0) - (start)));
	}

	/* Clear status */
	mvebu_mmc_write(mmc, SDIO_NOR_INTR_STATUS, SDIO_POLL_MASK);
	mvebu_mmc_write(mmc, SDIO_ERR_INTR_STATUS, SDIO_POLL_MASK);

	resptype = SDIO_CMD_INDEX(cmd->cmdidx);

	/* Analyzing resptype/xfertype/waittype for the command */
	if (cmd->resp_type & MMC_RSP_BUSY)
		resptype |= SDIO_CMD_RSP_48BUSY;
	else if (cmd->resp_type & MMC_RSP_136)
		resptype |= SDIO_CMD_RSP_136;
	else if (cmd->resp_type & MMC_RSP_PRESENT)
		resptype |= SDIO_CMD_RSP_48;
	else
		resptype |= SDIO_CMD_RSP_NONE;

	if (cmd->resp_type & MMC_RSP_CRC)
		resptype |= SDIO_CMD_CHECK_CMDCRC;

	if (cmd->resp_type & MMC_RSP_OPCODE)
		resptype |= SDIO_CMD_INDX_CHECK;

	if (cmd->resp_type & MMC_RSP_PRESENT) {
		resptype |= SDIO_UNEXPECTED_RESP;
		waittype |= SDIO_NOR_UNEXP_RSP;
	}

	if (data) {
		int err = mvebu_mmc_setup_data(d