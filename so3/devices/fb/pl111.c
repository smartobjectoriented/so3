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
#define CLCD_PL111_CNTL	0x00000018
#define CLCD_PL111_IENB	0x0000001c

#define VRAM_BASE	0x18000000

void *mmap(int fd, uint32_t virt_addr, uint32_t page_count);

struct file_operations pl111_ops = {
	.mmap = mmap
};


/*
 * Initialisation of the PL111 CLCD Controller.
 * References to the Linux driver (amba-clcd.c) are left for documentation.
 */
int pl111_init(dev_t *dev)
{
	/* amba-clcd.c:555 clcdfb_register: disable interrupts */
	iowrite32(dev->base + CLCD_PL111_IENB, 0);

	/* amba-clcd.c:327 clcdfb_set_par: set timing registers */

	// 00100111 00010111 01011111 10011100
	// HBP: 00100111 39 0x27
	// HFP: 00010111 23 0x17
	// HSW: 01011111 95 0x5f
	// PPL:   100111 39 0x27 => 16 * (39 + 1) = 640px horizontal
	iowrite32(dev->base + CLCD_TIM0, 0x27175f9c);

	// 00100000 00001011 00000101 11011111
	// VBP:   00100000  32  0x20
	// VFP:   00001011  11   0xb
	// VSW:     000001   1   0x1
	// LPP: 0111011111 479 0x1df => 479 + 1 = 480px vertical
	iowrite32(dev->base + CLCD_TIM1, 0x200b05df);

	// 00000110 01111111 00111000 00000000
	// PCD_HI:      00000
	// BCD:             1
	// CPL:    1001111111 639 0x27f
	// Unreg (1 bit)
	// IOE:             0
	// IPC:             1
	// IHS:             1
	// IVS:             1
	// ACB:        000000
	// CLKSEL:          0
	// PDC_LO:      00000
	iowrite32(dev->base + CLCD_TIM2, 0x067f3800);

	// 0
	iowrite32(dev->base + CLCD_TIM3, 0);

	/* amba-clcd.c:62 clcdfb_set_start: write_regs fb address */
	iowrite32(dev->base + CLCD_UBAS, 0x18000000);
	iowrite32(dev->base + CLCD_LBAS, 0x1804b000);

	/* amba-clcd.c:113 clcdfb_enable: enable and power on */

	// enable
	iowrite32(dev->base + CLCD_PL111_CNTL, 0x112d);

	// power on
	// 000000000000000 0 00 01 1 0 0 1 0 0 1 0 110 1
	// WATERMARK: 0
	// Unreg (2 bits)
	// LCDVCOMP:  01 => interrupt at start of back porch
	// LCDPWR:    1 => powered on
	// BEPO:      0 => unused for 16bpp
	// BEBO:      0 => little-endian byte order
	// BGR:       1 => blue green red
	// LCDDUAL:   0 => single panel LCD
	// LCDMONO8:  0 => unused for 16bpp mode
	// LCDTFT:    1 => LCD is TFT
	// LCDBW:     0 => color (unused for TFT)
	// LCDBPP:  110 => 16 bits per pixel, 5:6:5 mode
	// LCDEN:     1 => enabled
	iowrite32(dev->base + CLCD_PL111_CNTL, 0x182d);

	if (register_fb_ops(&pl111_ops)) {
		/* TODO print error message: device init but could not register. */
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
		page = VRAM_BASE + i * PAGE_SIZE;
		create_mapping(pcb->pgtable, virt_addr + (i * PAGE_SIZE), page, PAGE_SIZE, false);
		add_page_to_proc(pcb, phys_to_page(page));
	}

	return virt_addr;
}

REGISTER_DRIVER_POSTCORE("arm,pl111", pl111_init);
