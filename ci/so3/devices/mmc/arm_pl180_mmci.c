/*
 * ARM PrimeCell MultiMedia Card Interface - PL180
 *
 * Copyright (C) ST-Ericsson SA 2010
 *
 * Author: Ulf Hansson <ulf.hansson@stericsson.com>
 * Author: Martin Lundholm <martin.xa.lundholm@stericsson.com>
 * Ported to drivers/mmc/ by: Matt Waddel <matt.waddel@linaro.org>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#if 0
#define DEBUG
#endif

#include <delay.h>

#include <device/driver.h>

#include <asm/io.h>
#include <errno.h>
#include <mmc.h>
#include <bitops.h>
#include <heap.h>
#include <memory.h>

#include "arm_pl180_mmci.h"

struct pl180_mmc_host mmc_host;

static int wait_for_command_end(struct mmc *dev, struct mmc_cmd *cmd)
{
	u32 hoststatus, statusmask;
	struct pl180_mmc_host *host = dev->priv;

	statusmask = SDI_STA_CTIMEOUT | SDI_STA_CCRCFAIL;
	if ((cmd->resp_type & MMC_RSP_PRESENT))
		statusmask |= SDI_STA_CMDREND;
	else
		statusmask |= SDI_STA_CMDSENT;

	do
		hoststatus = ioread32(&host->base->status) & statusmask;
	while (!hoststatus);

	iowrite32(&host->base->status_clear, statusmask);
	if (hoststatus & SDI_STA_CTIMEOUT) {
		DBG("CMD%d time out\n", cmd->cmdidx);
		return TIMEOUT;
	} else if ((hoststatus & SDI_STA_CCRCFAIL) &&
		   (cmd->resp_type & MMC_RSP_CRC)) {
		printk("CMD%d CRC error\n", cmd->cmdidx);
		return -EILSEQ;
	}

	if (cmd->resp_type & MMC_RSP_PRESENT) {
		cmd->response[0] = ioread32(&host->base->response0);
		cmd->response[1] = ioread32(&host->base->response1);
		cmd->response[2] = ioread32(&host->base->response2);
		cmd->response[3] = ioread32(&host->base->response3);
		DBG("CMD%d response[0]:0x%08X, response[1]:0x%08X, "
			"response[2]:0x%08X, response[3]:0x%08X\n",
			cmd->cmdidx, cmd->response[0], cmd->response[1],
			cmd->response[2], cmd->response[3]);
	}

	return 0;
}

/* Send command to the mmc card and wait for results */
static int do_command(struct mmc *dev, struct mmc_cmd *cmd)
{
	int result;
	u32 sdi_cmd = 0;
	struct pl180_mmc_host *host = dev->priv;

	sdi_cmd = ((cmd->cmdidx & SDI_CMD_CMDINDEX_MASK) | SDI_CMD_CPSMEN);

	if (cmd->resp_type) {
		sdi_cmd |= SDI_CMD_WAITRESP;
		if (cmd->resp_type & MMC_RSP_136)
			sdi_cmd |= SDI_CMD_LONGRESP;
	}

	iowrite32(&host->base->argument, (u32)cmd->cmdarg);
	udelay(COMMAND_REG_DELAY);
	iowrite32(&host->base->command, sdi_cmd);
	result = wait_for_command_end(dev, cmd);

	/* After CMD2 set RCA to a none zero value. */
	if ((result == 0) && (cmd->cmdidx == MMC_CMD_ALL_SEND_CID))
		dev->rca = 10;

	/* After CMD3 open drain is switched off and push pull is used. */
	if ((result == 0) && (cmd->cmdidx == MMC_CMD_SET_RELATIVE_ADDR)) {
		u32 sdi_pwr = ioread32(&host->base->power) & ~SDI_PWR_OPD;
		iowrite32(&host->base->power, sdi_pwr);
	}

	return result;
}

static int read_bytes(struct mmc *dev, u32 *dest, u32 blkcount, u32 blksize)
{
	u32 *tempbuff = dest;
	u64 xfercount = blkcount * blksize;
	struct pl180_mmc_host *host = dev->priv;
	u32 status, status_err;

	DBG("read_bytes: blkcount=%u blksize=%u\n", blkcount, blksize);

	status = ioread32(&host->base->status);
	status_err = status & (SDI_STA_DCRCFAIL | SDI_STA_DTIMEOUT |
			       SDI_STA_RXOVERR);
	while ((!status_err) && (xfercount >= sizeof(u32))) {
		if (status & SDI_STA_RXDAVL) {
			*(tempbuff) = ioread32(&host->base->fifo);
			tempbuff++;
			xfercount -= sizeof(u32);
		}
		status = ioread32(&host->base->status);
		status_err = status & (SDI_STA_DCRCFAIL | SDI_STA_DTIMEOUT |
				       SDI_STA_RXOVERR);
	}

	status_err = status &
		(SDI_STA_DCRCFAIL | SDI_STA_DTIMEOUT | SDI_STA_DBCKEND |
		 SDI_STA_RXOVERR);
	while (!status_err) {
		status = ioread32(&host->base->status);
		status_err = status &
			(SDI_STA_DCRCFAIL | SDI_STA_DTIMEOUT | SDI_STA_DBCKEND |
			 SDI_STA_RXOVERR);
	}

	if (status & SDI_STA_DTIMEOUT) {
		printk("Read data timed out, xfercount: %llu, status: 0x%08X\n",
			xfercount, status);
		return -ETIMEDOUT;
	} else if (status & SDI_STA_DCRCFAIL) {
		printk("Read data bytes CRC error: 0x%x\n", status);
		return -EILSEQ;
	} else if (status & SDI_STA_RXOVERR) {
		printk("Read data RX overflow error\n");
		return -EIO;
	}

	iowrite32(&host->base->status_clear, SDI_ICR_MASK);

	if (xfercount) {
		printk("Read data error, xfercount: %llu\n", xfercount);
		return -ENOBUFS;
	}

	return 0;
}

static int write_bytes(struct mmc *dev, u32 *src, u32 blkcount, u32 blksize)
{
	u32 *tempbuff = src;
	int i;
	u64 xfercount = blkcount * blksize;
	struct pl180_mmc_host *host = dev->priv;
	u32 status, status_err;

	DBG("write_bytes: blkcount=%u blksize=%u\n", blkcount, blksize);

	status = ioread32(&host->base->status);
	status_err = status & (SDI_STA_DCRCFAIL | SDI_STA_DTIMEOUT);
	while (!status_err && xfercount) {
		if (status & SDI_STA_TXFIFOBW) {
			if (xfercount >= SDI_FIFO_BURST_SIZE * sizeof(u32)) {
				for (i = 0; i < SDI_FIFO_BURST_SIZE; i++)
					iowrite32(&host->base->fifo, *(tempbuff + i));

				tempbuff += SDI_FIFO_BURST_SIZE;
				xfercount -= SDI_FIFO_BURST_SIZE * sizeof(u32);
			} else {
				while (xfercount >= sizeof(u32)) {
					iowrite32(&host->base->fifo, *(tempbuff));
					tempbuff++;
					xfercount -= sizeof(u32);
				}
			}
		}
		status = ioread32(&host->base->status);
		status_err = status & (SDI_STA_DCRCFAIL | SDI_STA_DTIMEOUT);
	}

	status_err = status &
		(SDI_STA_DCRCFAIL | SDI_STA_DTIMEOUT | SDI_STA_DBCKEND);
	while (!status_err) {
		status = ioread32(&host->base->status);
		status_err = status &
			(SDI_STA_DCRCFAIL | SDI_STA_DTIMEOUT | SDI_STA_DBCKEND);
	}

	if (status & SDI_STA_DTIMEOUT) {
		printk("Write data timed out, xfercount:%llu,status:0x%08X\n",
		       xfercount, status);
		return -ETIMEDOUT;
	} else if (status & SDI_STA_DCRCFAIL) {
		printk("Write data CRC error\n");
		return -EILSEQ;
	}

	iowrite32(&host->base->status_clear, SDI_ICR_MASK);

	if (xfercount) {
		printk("Write data error, xfercount:%llu", xfercount);
		return -ENOBUFS;
	}

	return 0;
}

static int do_data_transfer(struct mmc *dev,
			    struct mmc_cmd *cmd,
			    struct mmc_data *data)
{
	int error = -ETIMEDOUT;
	struct pl180_mmc_host *host = dev->priv;
	u32 blksz = 0;
	u32 data_ctrl = 0;
	u32 data_len = (u32) (data->blocks * data->blocksize);

	if (!host->version2) {
		blksz = find_first_bit(&data->blocksize, BITS_PER_INT);
		data_ctrl |= ((blksz << 4) & SDI_DCTRL_DBLKSIZE_MASK);
	} else {
		blksz = data->blocksize;
		data_ctrl |= (blksz << SDI_DCTRL_DBLOCKSIZE_V2_SHIFT);
	}
	data_ctrl |= SDI_DCTRL_DTEN | SDI_DCTRL_BUSYMODE;

	iowrite32(&host->base->datatimer, SDI_DTIMER_DEFAULT);
	iowrite32(&host->base->datalength, data_len);
	udelay(DATA_REG_DELAY);

	if (data->flags & MMC_DATA_READ) {
		data_ctrl |= SDI_DCTRL_DTDIR_IN;
		iowrite32(&host->base->datactrl, data_ctrl);

		error = do_command(dev, cmd);
		if (error)
			return error;

		error = read_bytes(dev, (u32 *)data->dest, (u32)data->blocks,
				   (u32)data->blocksize);
	} else if (data->flags & MMC_DATA_WRITE) {
		error = do_command(dev, cmd);
		if (error)
			return error;

		iowrite32(&host->base->datactrl, data_ctrl);
		error = write_bytes(dev, (u32 *)data->src, (u32)data->blocks,
							(u32)data->blocksize);
	}

	return error;
}

static int host_request(struct mmc *dev,
			struct mmc_cmd *cmd,
			struct mmc_data *data)
{
	int result;

	if (data)
		result = do_data_transfer(dev, cmd, data);
	else
		result = do_command(dev, cmd);

	return result;
}

/* MMC uses open drain drivers in the enumeration phase */
static int mmc_host_reset(struct mmc *dev)
{
	struct pl180_mmc_host *host = dev->priv;

	iowrite32(&host->base->power, host->pwr_init);

	return 0;
}

static void host_set_ios(struct mmc *dev)
{
	struct pl180_mmc_host *host = dev->priv;
	u32 sdi_clkcr;

	sdi_clkcr = ioread32(&host->base->clock);

	/* Ramp up the clock rate */
	if (dev->clock) {
		u32 clkdiv = 0;
		u32 tmp_clock;

		if (dev->clock >= dev->cfg->f_max) {
			clkdiv = 0;
			dev->clock = dev->cfg->f_max;
		} else {
			clkdiv = (host->clock_in / dev->clock) - 2;
		}

		tmp_clock = host->clock_in / (clkdiv + 2);
		while (tmp_clock > dev->clock) {
			clkdiv++;
			tmp_clock = host->clock_in / (clkdiv + 2);
		}

		if (clkdiv > SDI_CLKCR_CLKDIV_MASK)
			clkdiv = SDI_CLKCR_CLKDIV_MASK;

		tmp_clock = host->clock_in / (clkdiv + 2);
		dev->clock = tmp_clock;
		sdi_clkcr &= ~(SDI_CLKCR_CLKDIV_MASK);
		sdi_clkcr |= clkdiv;
	}

	/* Set the bus width */
	if (dev->bus_width) {
		u32 buswidth = 0;

		switch (dev->bus_width) {
		case 1:
			buswidth |= SDI_CLKCR_WIDBUS_1;
			break;
		case 4:
			buswidth |= SDI_CLKCR_WIDBUS_4;
			break;
		case 8:
			buswidth |= SDI_CLKCR_WIDBUS_8;
			break;
		default:
			printk("Invalid bus width: %d\n", dev->bus_width);
			break;
		}
		sdi_clkcr &= ~(SDI_CLKCR_WIDBUS_MASK);
		sdi_clkcr |= buswidth;
	}

	iowrite32(&host->base->clock, sdi_clkcr);
	udelay(CLK_CHANGE_DELAY);
}

static const struct mmc_ops arm_pl180_mmci_ops = {
	.send_cmd = host_request,
	.set_ios = host_set_ios,
	.init = mmc_host_reset,
};

/*
 * mmc_host_init - initialize the mmc controller.
 * Set initial clock and power for mmc slot.
 * Initialize mmc struct and register with mmc framework.
 */
static int arm_pl180_mmci_init(dev_t *dev, int fdt_offset) {
 	struct mmc *mmc;
	u32 sdi_u32;
	const struct fdt_property *prop;
	int prop_len;

	prop = fdt_get_property(__fdt_addr, fdt_offset, "reg", &prop_len);
	BUG_ON(!prop);

	BUG_ON(prop_len != 2 * sizeof(unsigned long));

#ifdef CONFIG_ARCH_ARM32
	mmc_host.base = (struct sdi_registers *) io_map(fdt32_to_cpu(((const fdt32_t *) prop->data)[0]), fdt32_to_cpu(((const fdt32_t *) prop->data)[1]));
#else
	mmc_host.base = (struct sdi_registers *) io_map(fdt64_to_cpu(((const fdt64_t *) prop->data)[0]), fdt64_to_cpu(((const fdt64_t *) prop->data)[1]));
#endif

	mmc_host.pwr_init = fdt_get_int(__fdt_addr, dev, "power");
	mmc_host.clkdiv_init = fdt_get_int(__fdt_addr, dev, "clkdiv");
	mmc_host.caps = fdt_get_int(__fdt_addr, dev, "caps");
	mmc_host.voltages = fdt_get_int(__fdt_addr, dev, "voltages");
	mmc_host.clock_min = fdt_get_int(__fdt_addr, dev, "clock_min");
	mmc_host.clock_max = fdt_get_int(__fdt_addr, dev, "clock_max");
	mmc_host.b_max = fdt_get_int(__fdt_addr, dev, "b_max");

	iowrite32(&mmc_host.base->power, mmc_host.pwr_init);
	iowrite32(&mmc_host.base->clock, mmc_host.clkdiv_init);
	udelay(CLK_CHANGE_DELAY);

	/* Disable mmc interrupts */
	sdi_u32 = ioread32(&mmc_host.base->mask0) & ~SDI_MASK0_MASK;
	iowrite32(&mmc_host.base->mask0, sdi_u32);

	mmc_host.cfg.name = "mmc-pl180";

	mmc_host.cfg.ops = &arm_pl180_mmci_ops;

	/* TODO remove the duplicates */
	mmc_host.cfg.host_caps = mmc_host.caps;
	mmc_host.cfg.voltages = mmc_host.voltages;
	mmc_host.cfg.f_min = mmc_host.clock_min;
	mmc_host.cfg.f_max = mmc_host.clock_max;
	if (mmc_host.b_max != 0)
		mmc_host.cfg.b_max = mmc_host.b_max;
	else
		mmc_host.cfg.b_max = CONFIG_SYS_MMC_MAX_BLK_COUNT;

	mmc = mmc_create(&mmc_host.cfg, &mmc_host);
	if (mmc == NULL)
		return -1;

	/* Ready to initialize the mmc subsystem */
	mmc_initialize();

	DBG("registered mmc interface number is:%d\n", mmc->block_dev.dev);

	return 0;
}

REGISTER_DRIVER_POSTCORE("vexpress,mmc-pl180", arm_pl180_mmci_init);


