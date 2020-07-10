/*
 * Copyright (C) 2014-2017 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#include <common.h>
#include <memory.h>
#include <part.h>
#include <div64.h>

#include <device/fdt/fdt.h>
#include <device/ramdev.h>

#include <asm/mmu.h>
#include <asm/cacheflush.h>

static block_dev_desc_t ramdev_block_dev;
static int ramdev_size = 0;

extern const void *fdt_getprop(const void *fdt, int nodeoffset,
                const char *name, int *lenp);
extern int fdt_next_node(const void *fdt, int offset, int *depth);

/*
 * Check if there is a valid ramdev which could be used for rootfs.
 */
bool valid_ramdev(void) {
	return (ramdev_size > 0);
}

uint32_t get_ramdev_size(void) {
	return ramdev_size;
}

unsigned long ramdev_read(int dev, lbaint_t start, lbaint_t blkcnt, void *buffer) {
	uint32_t bytes_count;

	ASSERT(valid_ramdev());

	bytes_count = blkcnt * ramdev_block_dev.blksz;

	memcpy(buffer, (void *) RAMDEV_VIRT_BASE + start * ramdev_block_dev.blksz, bytes_count);

	return blkcnt;
}

unsigned long ramdev_write(int dev, lbaint_t start, lbaint_t blkcnt, const void *buffer) {
	uint32_t bytes_count;

	ASSERT(valid_ramdev());

	bytes_count = blkcnt * ramdev_block_dev.blksz;

	memcpy((void *) RAMDEV_VIRT_BASE + start * ramdev_block_dev.blksz, buffer, bytes_count);

	return blkcnt;
}

unsigned long ramdev_erase(int dev, lbaint_t start, lbaint_t blkcnt) {

	ASSERT(valid_ramdev());

	return blkcnt;
}

block_dev_desc_t *ramdev_get_dev(int dev)
{

	/* Setup the universal parts of the block interface just once */
	ramdev_block_dev.if_type = IF_TYPE_RAMDEV;

	ramdev_block_dev.dev = 1;

	ramdev_block_dev.removable = 0;

	ramdev_block_dev.block_read = ramdev_read;
	ramdev_block_dev.block_write = ramdev_write;
	ramdev_block_dev.block_erase = ramdev_erase;


	ramdev_block_dev.lun = 0;
	ramdev_block_dev.type = 0;
	ramdev_block_dev.blksz = 512;
	ramdev_block_dev.log2blksz = LOG2(ramdev_block_dev.blksz);
	ramdev_block_dev.lba = lldiv(ramdev_size, 512);

	ramdev_block_dev.vendor[0] = 0;
	ramdev_block_dev.product[0] = 0;
	ramdev_block_dev.revision[0] = 0;

	return &ramdev_block_dev;
}

/*
 * Get the ramdev if any.
 * Returns the size of the ramdev if found.
 */
size_t get_ramdev(const void *fdt) {
	int nodeoffset = 0;
	const fdt32_t *initrd_start, *initrd_end;
	uint32_t ramdev_start, ramdev_end;
	int lenp;
	int depth;
	bool found = false;

	while (!found) {
		nodeoffset = fdt_next_node(fdt, nodeoffset, &depth);
		if (nodeoffset < 0)
			/* No node found */
			return -1;

		/*
		 * Try to find such strings since U-boot patches the dtb following
		 * this convention (two pre-defined properties).
		 */
		initrd_start = fdt_getprop(fdt, nodeoffset, "linux,initrd-start", &lenp);
		initrd_end = fdt_getprop(fdt, nodeoffset, "linux,initrd-end", &lenp);

		found = (initrd_start && initrd_end);
	}

	if (!found)
		return 0;

	ramdev_start = fdt32_to_cpu(initrd_start[0]);
	ramdev_end = fdt32_to_cpu(initrd_end[0]);

	/* Do the virtual mapping */

	create_mapping(NULL, RAMDEV_VIRT_BASE, ramdev_start, ramdev_end-ramdev_start, false, false);

	flush_tlb_all();
	cache_clean_flush();

	return ramdev_end-ramdev_start;
}

/*
 * Main ramdev initialization function.
 * Called by devices_init() in devce.c
 */
void ramdev_init(void) {

	ramdev_size = get_ramdev((void *) _fdt_addr);

	if (ramdev_size > 0)
		printk("so3: rootfs in RAM detected (ramdev enabled) with size of %d bytes...\n", ramdev_size);
}

