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

/*
 * Driver for the PL111 CLCD controller.
 *
 * Documentation:
 *   http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0293c/index.html
 */

#include <vfs.h>
#include <process.h>
#include <asm/io.h>
#include <asm/mmu.h>
#include <device/driver.h>

#define IOCTL_HRES 1
#define IOCTL_VRES 2
#define IOCTL_SIZE 3

#define HRES 1024
#define VRES  768

/* Register address offsets */
#define CLCD_TIM0	0x000
#define CLCD_TIM1	0x004
#define CLCD_TIM2	0x008
#define CLCD_TIM3	0x00c
#define CLCD_UBAS	0x010
#define CLCD_LBAS	0x014
#define CLCD_CNTL	0x018
#define CLCD_IENB	0x01c

/* Timing0 register values */
#define HBP (151 << 24)
#define HFP ( 47 << 16)
#define HSW (103 <<  8)
#define PPL ((HRES / 16 - 1) <<  2) /* PPL = horizonzal res / 16 - 1 */

/* Timing1 register values */
#define VBP (  22 << 24)
#define VFP (   2 << 16)
#define VSW (   3 << 10)
#define LPP (VRES <<  0) /* LPP = vertical res - 1 */

/* Timing2 register values */
#define PCD_HI (   0 << 27)
#define BCD    (   1 << 26)
#define CPL    (HRES << 16) /* for TFT: PPL - 1 */
#define IOE    (   0 << 14)
#define IPC    (   1 << 13)
#define IHS    (   1 << 12)
#define IVS    (   1 << 11)
#define ACB    (   0 <<  6)
#define CLKSEL (   0 <<  5)
#define PCD_LO (   0 <<  0)

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
#define BGR       (1 <<  8) /* 0: RGB, 1: BGR */
#define LCDDUAL   (0 <<  7) /* single panel */
#define LCDMONO8  (0 <<  6) /* unused for TFT */
#define LCDTFT    (1 <<  5) /* LCD is TFT */
#define LCDBW     (0 <<  4) /* unused for TFT */
#define LCDBPP    (5 <<  1) /* 5: 24bpp, 6: 16bpp565 */
#define LCDEN     (1 <<  0) /* enable display */


void *fb_mmap(int fd, addr_t virt_addr, uint32_t page_count);
int fb_ioctl(int fd, unsigned long cmd, unsigned long args);

struct file_operations pl111_fops = {
	.mmap = fb_mmap,
	.ioctl = fb_ioctl
};

struct devclass pl111_cdev = {
	.class = DEV_CLASS_FB,
	.type = VFS_TYPE_DEV_FB,
	.fops = &pl111_fops,
};


/*
 * Initialisation of the PL111 CLCD Controller.
 * Linux driver: video/fbdev/amba-clcd.c
 */
static int pl111_init(dev_t *dev, int fdt_offset)
{
	const struct fdt_property *prop;
	int prop_len;
	void *base;

	prop = fdt_get_property(__fdt_addr, fdt_offset, "reg", &prop_len);
	BUG_ON(!prop);
	BUG_ON(prop_len != 2 * sizeof(unsigned long));

	/* Mapping the device properly */
#ifdef CONFIG_ARCH_ARM32
	base = (void *) io_map(fdt32_to_cpu(((const fdt32_t *) prop->data)[0]), fdt32_to_cpu(((const fdt32_t *) prop->data)[1]));
#else
	base = (void *) io_map(fdt64_to_cpu(((const fdt64_t *) prop->data)[0]), fdt64_to_cpu(((const fdt64_t *) prop->data)[1]));

#endif

	/* Disable interrupts. */
	iowrite32(base + CLCD_IENB, 0);

	/* Set timing registers. */
	iowrite32(base + CLCD_TIM0, HBP | HFP | HSW | PPL);
	iowrite32(base + CLCD_TIM1, VBP | VFP | VSW | LPP);
	iowrite32(base + CLCD_TIM2, PCD_HI | BCD | CPL | IOE | IPC | IHS | IVS | ACB | CLKSEL | PCD_LO);
	iowrite32(base + CLCD_TIM3, LEE | LED);

	/* Set framebuffer addresses. */
	iowrite32(base + CLCD_UBAS, LCDUPBASE);
	if (LCDDUAL) {
		iowrite32(base + CLCD_LBAS, LCDLPBASE);
	}

	/* Configure, enable and power on the controller. */
	iowrite32(base + CLCD_CNTL, WATERMARK | LCDVCOMP | LCDPWR | BEPO | BEBO | BGR | LCDDUAL | LCDMONO8 | LCDTFT | LCDBW | LCDBPP | LCDEN);

	/* Register the framebuffer so it can be accessed from user space. */
	devclass_register(dev, &pl111_cdev);

	return 0;
}

void *fb_mmap(int fd, addr_t virt_addr, uint32_t page_count)
{
	uint32_t i, page;
	pcb_t *pcb = current()->pcb;

	for (i = 0; i < page_count; i++) {
		/* Map a process' virtual page to the physical one (here the VRAM). */
		page = LCDUPBASE + i * PAGE_SIZE;
		create_mapping(pcb->pgtable, virt_addr + (i * PAGE_SIZE), page, PAGE_SIZE, false);
	}

	return (void *) virt_addr;
}

int fb_ioctl(int fd, unsigned long cmd, unsigned long args)
{
	switch (cmd) {

	case IOCTL_HRES:
		*((uint32_t *) args) = HRES;
		return 0;

	case IOCTL_VRES:
		*((uint32_t *) args) = VRES;
		return 0;

	case IOCTL_SIZE:
		*((uint32_t *) args) = HRES * VRES * 4; /* assume 24bpp */
		return 0;

	default:
		/* Unknown command. */
		return -1;
	}
}

REGISTER_DRIVER_POSTCORE("arm,pl111", pl111_init);
