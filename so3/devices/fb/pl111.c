/*
 * Copyright (C) 2020 Nikolaos Garanis <nikolaos.garanis@heig-vd.ch>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <vfs.h>
#include <process.h>
#include <asm/io.h>
#include <asm/mmu.h>
#include <device/driver.h>
#include <device/fb.h>

/* Register addresses */
#define CLCD_TIM0	0x00000000
#define CLCD_TIM1	0x00000004
#define CLCD_TIM2	0x00000008
#define CLCD_TIM3	0x0000000c
#define CLCD_UBAS	0x00000010
#define CLCD_LBAS	0x00000014
#define CLCD_CNTL	0x00000018
#define CLCD_IENB	0x0000001c

/* Timing0 register values */
#define HBP (39 << 24)
#define HFP (23 << 16)
#define HSW (95 <<  8)
#define PPL (39 <<  2) /* 16 * (PPL + 1) = horizontal res */

/* Timing1 register values */
#define VBP ( 32 << 24)
#define VFP ( 11 << 16)
#define VSW (  1 << 10)
#define LPP (479 <<  0) /* LPP + 1 = vertical res */

/* Timing2 register values */
#define PCD_HI (  0 << 27)
#define BCD    (  1 << 26)
#define CPL    (639 << 16) /* for TFT: PPL - 1 */
#define IOE    (  0 << 14)
#define IPC    (  1 << 13)
#define IHS    (  1 << 12)
#define IVS    (  1 << 11)
#define ACB    (  0 <<  6)
#define CLKSEL (  0 <<  5)
#define PCD_LO (  0 <<  0)

/* Timing3 register values */
#define LEE (0 << 16)
#define LED (0 <<  0)

/* Framebuffer base addresses (first 2 bits must be 0) */
#define LCDUPBASE 0x18000000 /* upper panel, corresponds to VRAM */
#define LCDLPBASE        0x0 /* lower panel */

/* Control register values */
#define WATERMARK (0 << 16)
#define LCDVCOMP  (1 << 12) /* generate interrupt at start of back porch */
#define LCDPWR    (1 << 11) /* power display */
#define BEPO      (0 << 10) /* little-endian pixel ordering */
#define BEBO      (0 <<  9) /* little-endian byte order */
#define BGR       (0 <<  8) /* RGB */
#define LCDDUAL   (0 <<  7) /* single panel */
#define LCDMONO8  (0 <<  6) /* unused for TFT */
#define LCDTFT    (1 <<  5) /* LCD is TFT */
#define LCDBW     (0 <<  4) /* unused for TFT */
#define LCDBPP    (6 <<  1) /* 16bpp 5:6:5 mode */
#define LCDEN     (1 <<  0) /* enable display */


void *mmap(int fd, uint32_t virt_addr, uint32_t page_count);

struct file_operations pl111_ops = {
	.mmap = mmap
};


/*
 * Initialisation of the PL111 CLCD Controller.
 */
int pl111_init(dev_t *dev)
{
	/* Disable interrupts. */
	iowrite32(dev->base + CLCD_IENB, 0);

	/* Set timing registers. */
	iowrite32(dev->base + CLCD_TIM0, HBP | HFP | HSW | PPL);
	iowrite32(dev->base + CLCD_TIM1, VBP | VFP | VSW | LPP);
	iowrite32(dev->base + CLCD_TIM2, PCD_HI | BCD | CPL | IOE | IPC | IHS | IVS | ACB | CLKSEL | PCD_LO);
	iowrite32(dev->base + CLCD_TIM3, LEE | LED);

	/* Set frame address */
	iowrite32(dev->base + CLCD_UBAS, LCDUPBASE);
	if (LCDDUAL) {
		iowrite32(dev->base + CLCD_LBAS, LCDLPBASE);
	}

	/* amba-clcd.c:113 clcdfb_enable: enable and power on */
	iowrite32(dev->base + CLCD_CNTL, WATERMARK | LCDVCOMP | LCDPWR | BEPO | BEBO | BGR | LCDDUAL | LCDMONO8 | LCDTFT | LCDBW | LCDBPP | LCDEN);

	/* Register framebuffer fops. */
	if (register_fb_ops(&pl111_ops)) {
		printk("%s: pl111 initialised but could not register fops.", __func__);
		return -1;
	}

	return 0;
}

void *mmap(int fd, uint32_t virt_addr, uint32_t page_count)
{
	uint32_t i, page;
	pcb_t *pcb = current()->pcb;

	for (i = 0; i < page_count; i++) {
		/* Map a process' virtual page to the physical one (here the VRAM). */
		page = LCDUPBASE + i * PAGE_SIZE;
		create_mapping(pcb->pgtable, virt_addr + (i * PAGE_SIZE), page, PAGE_SIZE, false);
		add_page_to_proc(pcb, phys_to_page(page));
	}

	return (void *) virt_addr;
}

REGISTER_DRIVER_POSTCORE("arm,pl111", pl111_init);
