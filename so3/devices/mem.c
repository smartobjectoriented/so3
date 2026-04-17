/*
 * Copyright (C) 2025 Clement Dieperink <clement.dieperink@heig-vd.ch>
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

#include <errno.h>
#include <vfs.h>
#include <common.h>
#include <process.h>
#include <asm/io.h>
#include <asm/mmu.h>
#include <device/driver.h>

#define DEV_CLASS_MEM "mem"

static int mem_mmap(int fd, addr_t virt_addr, uint32_t page_count, off_t offset)
{
	const size_t MEM_START = mem_info.phys_base;
	const size_t MEM_END = mem_info.phys_base + mem_info.size;
	pcb_t *pcb;
	addr_t next_phys_addr = offset;
	addr_t next_virt_addr = virt_addr;
	size_t remaining_size = page_count * PAGE_SIZE;
	size_t map_size;

	/* Detect physical or virtual address overflow */
	if (((offset + remaining_size) < offset) || ((virt_addr + remaining_size) < virt_addr)) {
		return -EINVAL;
	}

	if (!virt_addr) {
		return -EINVAL;
	}

	pcb = current()->pcb;
	BUG_ON(pcb == NULL);

	/* Mapping before the RAM */
	if ((remaining_size > 0) && (next_phys_addr < MEM_START)) {
		map_size = min(remaining_size, MEM_START - next_phys_addr);
		create_mapping(pcb->pgtable, next_virt_addr, next_phys_addr, map_size, true);

		next_virt_addr += map_size;
		next_phys_addr += map_size;
		remaining_size -= map_size;
	}

	/* Mapping in the RAM */
	if ((remaining_size > 0) && (next_phys_addr < MEM_END)) {
		map_size = min(remaining_size, MEM_END - next_phys_addr);
		create_mapping(pcb->pgtable, next_virt_addr, next_phys_addr, map_size, false);

		next_virt_addr += map_size;
		next_phys_addr += map_size;
		remaining_size -= map_size;
	}

	/* Mapping after the RAM */
	if (remaining_size > 0)
		create_mapping(pcb->pgtable, next_virt_addr, next_phys_addr, remaining_size, true);

	return 0;
}

static struct file_operations mem_fops = {
	.mmap = mem_mmap,
};

static struct devclass mem_cdev = {
	.class = DEV_CLASS_MEM,
	.type = VFS_TYPE_DEV_CHAR,
	.fops = &mem_fops,
};

static int mem_init(dev_t *dev, int fdt_offset)
{
	devclass_register(dev, &mem_cdev);
	return 0;
}
REGISTER_DRIVER_POSTCORE("mem", mem_init);
