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

#include <device/fdt.h>
#include <device/ramdev.h>

#include <asm/div64.h>
#include <asm/mmu.h>
#include <asm/cacheflush.h>

static block_dev_desc_t ramdev_block_dev;
static unsigned long ramdev_size = 0;
static addr_t ramdev_start, ramdev_end;

/*
 * Check if there is a valid ramdev which could be used for rootfs.
 */
bool valid_ramdev(void) {
	return (ramdev_size > 0);
}

unsigned long get_ramdev_size(void) {
	return ramdev_size;
}

/*
 * Get the physical address of ramdev start
 */
addr_t get_ramdev_start(void) {
	return ramdev_start;
}

unsigned long ramdev_read(int dev, lbaint_t start, lbaint_t blkcnt, void *buffer) {
	uint32_t bytes_count;

	ASSERT(valid_ramdev());

	bytes_count = blkcnt * ramdev_block_dev.blksz;

	memcpy(buffer, (void *) RAMDEV_VADDR + start * ramdev_block_dev.blksz, bytes_count);

	return blkcnt;
}

unsigned long ramdev_write(int dev, lbaint_t start, lbaint_t blkcnt, const void *buffer) {
	uint32_t bytes_count;

	ASSERT(valid_ramdev());

	bytes_count = blkcnt * ramdev_block_dev.blksz;

	memcpy((void *) RAMDEV_VADDR + start * ramdev_block_dev.blksz, buffer, bytes_count);

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
static void get_ramdev(const void *fdt) {
	int nodeoffset = 0;
	const struct fdt_property *initrd_start, *initrd_end;
	int lenp;
	int depth = 0;
	bool found = false;

	while (!found) {
		nodeoffset = fdt_next_node(fdt, nodeoffset, &depth);
		if (nodeoffset < 0)
			/* No node found */
			return ;

		/*
		 * Try to find such strings since U-boot patches the dtb following
		 * this convention (two pre-defined properties).
		 */
		initrd_start = fdt_get_property(fdt, nodeoffset, "linux,initrd-start", &lenp);
		initrd_end = fdt_get_property(fdt, nodeoffset, "linux,initrd-end", &lenp);

		found = (initrd_start && initrd_end);
	}

	if (!found)
		return ;

#ifdef CONFIG_ARCH_ARM32
		ramdev_start = fdt32_to_cpu(((const fdt32_t *) initrd_start->data)[0]);
		ramdev_end = fdt32_to_cpu(((const fdt32_t *) initrd_end->data)[0]);
#else
		ramdev_start = fdt64_to_cpu(((const fdt64_t *) initrd_start->data)[0]);
		ramdev_end = fdt64_to_cpu(((const fdt64_t *) initrd_end->data)[0]);
#endif

	/*
	 * About the size of ramdev: ramdev_end is the address *after* the initrd region according to U-boot which
	 * computes the size in the same way.
	 */
	ramdev_size = ramdev_end - ramdev_start;

	/* Do the virtual mapping */
	ramdev_create_mapping(NULL, ramdev_start, ramdev_end);
}

/*
 * Main ramdev initialization function.
 * Called by devices_init() in devce.c
 */
void ramdev_init(void) {
	int i;
	addr_t ramdev_pfn_start;

	get_ramdev((void *) __fdt_addr);

	if (ramdev_size > 0) {
		printk("so3: rootfs in RAM detected (ramdev enabled) with size of %d bytes...\n", ramdev_size);

		/* Mark all pfns dedicated to the (possible) ramdev as busy. */

		ramdev_pfn_start = get_ramdev_start() >> PAGE_SHIFT;

		for (i = ramdev_pfn_start; i <= ramdev_pfn_start + (ALIGN_UP(ramdev_size, PAGE_SIZE) >> PAGE_SHIFT); i++) {
			pfn_to_page(i)->free = false;
			pfn_to_page(i)->refcount++;
		}
	}
}

