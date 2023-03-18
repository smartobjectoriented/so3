/*
 * Copyright (C) 2016-2017 Daniel Rossier <daniel.rossier@soo.tech>
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

#include <sizes.h>

#include <libfdt/fdt_support.h>
#include <libfdt/image.h>

#include <device/fdt.h>

#include <avz/sched.h>
#include <avz/memslot.h>

#include <avz/uapi/avz.h>

#include <asm/mmu.h>
#include <asm/cacheflush.h>
#include <asm/setup.h>

/**
 * We put all the guest domains in ELF format on top of memory so
 * that the domain_build will be able to elf-parse and load to their final destination.
 *
 * Regarding __fdt_addr: the address is still valid in the identy mapping, so we use it.
 */
void loadAgency(void)
{
	u64 dom_addr, avz_dt_addr;
	int count;
	int nodeoffset, next_node;
	uint8_t tmp[16];
	addr_t base;
	size_t size;
	int len, depth, ret;
	const char *propstring;

	void *fdt_vaddr = __fdt_addr;

#ifdef CONFIG_ARM64VT
	const struct fdt_property *initrd_start, *initrd_end;
	u64 entry_addr;
	int lenp;
#endif

	ret = fdt_check_header(fdt_vaddr);
	if (ret) {
		lprintk("!! Bad device tree: ret = %x\n", ret);
		BUG();
	}

	memslot_init();

	nodeoffset = 0;
	depth = 0;
	count = 0;
	while ((count < 2) && (nodeoffset >= 0)) {
		next_node = fdt_next_node(fdt_vaddr, nodeoffset, &depth);

		ret = fdt_property_read_string(fdt_vaddr, nodeoffset, "type", &propstring);

		/* Process the type "avz" to get the AVZ device tree */
		if ((ret != -1) && !strcmp(propstring, "avz_dt")) {

			/* According to U-boot, the <load> and <entry> properties are both on 64-bit even for aarch32 configuration. */

			ret = fdt_property_read_u64(fdt_vaddr, nodeoffset, "load", (u64 *) &avz_dt_addr);
			if (ret == -1) {
				lprintk("!! Missing load-addr in the avz_dt node !!\n");
				BUG();
			}

			count++;
		}

		/* Process the type "avz" to get the agency image */
		if ((ret != -1) && !strcmp(propstring, "avz")) {

			/* According to U-boot, the <load> and <entry> properties are both on 64-bit even for aarch32 configuration. */

			ret = fdt_property_read_u64(fdt_vaddr, nodeoffset, "load", (u64 *) &dom_addr);
			if (ret == -1) {
				lprintk("!! Missing load-addr in the agency node !!\n");
				BUG();
			}
			lprintk("ITB: Domain load addr = 0x%lx\n", dom_addr);
#ifdef CONFIG_ARM64VT
			ret = fdt_property_read_u64(fdt_vaddr, nodeoffset, "entry", (u64 *) &entry_addr);
			if (ret == -1) {
				lprintk("!! Missing entry in the agency node !!\n");
				BUG();
			}
			lprintk("ITB: Domain entry addr = 0x%lx\n", entry_addr);
#endif

			count++;
		}
		nodeoffset = next_node;
	}

	if (nodeoffset < 0) {
		lprintk("!! Unable to find a node with type avz and/or avz_dt in the FIT image... !!\n");
		BUG();
	}

	memslot[MEMSLOT_AVZ].fdt_paddr = avz_dt_addr;

	/* Set the memslot base address to a 2 MB block boundary to ease mapping with ARM64 */

	memslot[MEMSLOT_AGENCY].base_paddr = dom_addr & ~(SZ_2M - 1);
	memslot[MEMSLOT_AGENCY].fdt_paddr = (addr_t) __fdt_addr;
	memslot[MEMSLOT_AGENCY].size = fdt_getprop_u32_default(fdt_vaddr, "/agency", "domain-size", 0);

	memslot[MEMSLOT_AGENCY].entry_addr = AGENCY_VOFFSET + (dom_addr & (SZ_1M - 1));

	if (!memslot[MEMSLOT_AGENCY].size) {
		lprintk("!! Property domain-size is found at 0, maybe the agency node is missing...\n");
		BUG();
	}

#ifdef CONFIG_ARM64VT
	memslot[MEMSLOT_AGENCY].entry_addr = entry_addr;
	memslot[MEMSLOT_AGENCY].ipa_addr = entry_addr & ~(SZ_1M - 1);
	lprintk("IPA Layout: device tree located at 0x%lx\n", memslot[MEMSLOT_AGENCY].fdt_paddr);
#endif

	/* Fixup the agency device tree */

	/* find or create "/memory" node. */
	nodeoffset = fdt_find_or_add_subnode(fdt_vaddr, 0, "memory");
	BUG_ON(nodeoffset < 0);

	fdt_setprop(fdt_vaddr, nodeoffset, "device_type", "memory", sizeof("memory"));

	size = memslot[MEMSLOT_AGENCY].size;

#ifdef CONFIG_ARM64VT
	base = memslot[MEMSLOT_AGENCY].ipa_addr;
#else
	base = memslot[MEMSLOT_AGENCY].base_paddr;
#endif
	len = fdt_pack_reg(fdt_vaddr, tmp, &base, &size);

	fdt_setprop(fdt_vaddr, nodeoffset, "reg", tmp, len);

#ifdef CONFIG_ARM64VT
	/* Fixup of initrd_start and initrd_end */
	nodeoffset = 0;
	depth = 0;
	while (nodeoffset >= 0) {
		next_node = fdt_next_node(fdt_vaddr, nodeoffset, &depth);

		initrd_start = fdt_get_property(fdt_vaddr, nodeoffset, "linux,initrd-start", &lenp);

		if (initrd_start) {
			initrd_end = fdt_get_property(fdt_vaddr, nodeoffset, "linux,initrd-end", &lenp);
			BUG_ON(!initrd_end);

			base = fdt64_to_cpu(((const fdt64_t *) initrd_start->data)[0]);
			base = phys_to_ipa(memslot[MEMSLOT_AGENCY], base);
			lprintk("IPA Layout: initrd start at 0x%lx\n", base);

			fdt_setprop_u64(fdt_vaddr, nodeoffset, "linux,initrd-start", base);

			base = fdt64_to_cpu(((const fdt64_t *) initrd_end->data)[0]);
			base = phys_to_ipa(memslot[MEMSLOT_AGENCY], base);
			lprintk("IPA Layout: initrd end at 0x%lx\n", base);

			fdt_setprop_u64(fdt_vaddr, nodeoffset, "linux,initrd-end", base);

			break;
		}
		nodeoffset = next_node;
	}
#endif

	/* Update our reference to the AVZ device tree */
	__fdt_addr = (void *) memslot[MEMSLOT_AVZ].fdt_paddr;

#ifdef CONFIG_ARCH_ARM32
	/* Lets map the AVZ DT in the identity mapping */
	((uint32_t *) __sys_root_pgtable)[l1pte_index(__fdt_addr)] = (addr_t) __fdt_addr;
	set_l1_pte_sect_dcache(&((uint32_t *) __sys_root_pgtable)[l1pte_index(__fdt_addr)], L1_SECT_DCACHE_WRITEALLOC);

#endif

	/* Retrieve the memory info in the AVZ DT */
	/* Access to device tree */
	/* __fdt_addr MUST BE mapped in an identity mapping */

	/* Get the RAM information of the board */
	early_memory_init();

	lprintk("  AVZ memory descriptor : found %d MB of RAM at 0x%08X\n", mem_info.size / SZ_1M, mem_info.phys_base);

	memslot[MEMSLOT_AVZ].base_paddr = mem_info.phys_base;
	memslot[MEMSLOT_AVZ].size = mem_info.size;
	memslot[MEMSLOT_AVZ].busy = true;
}


/**
 * The ITB image will be parsed and the components placed in their target memory location.
 *
 * @param slotID
 * @param itb	ITB image
 */
void loadME(unsigned int slotID, void *itb) {
	void *ME_vaddr;
	size_t ME_size, size, fdt_size, initrd_size;
	void *fdt_vaddr, *initrd_vaddr;
	void *dest_ME_vaddr;
	uint32_t initrd_start, initrd_end;
	int nodeoffset, next_node, depth = 0;
	int ret;
	const char *propstring;
	uint8_t tmp[16];
	addr_t base;
	int len;

	/* Look for a node of ME type in the fit image */
	nodeoffset = 0;
	depth = 0;
	while (nodeoffset >= 0) {
		next_node = fdt_next_node(itb, nodeoffset, &depth);
		ret = fdt_property_read_string(itb, nodeoffset, "type", &propstring);

		if ((ret != -1) && !strcmp(propstring, "ME")) {

			/* Get the pointer to the OS binary image from the ITB we got from the user space. */
			ret = fit_image_get_data_and_size(itb, nodeoffset, (const void **) &ME_vaddr, &ME_size);
			if (ret) {
				lprintk("!! The properties in the ME node does not look good !!\n");
				BUG();
			} else
				break;
		}
		nodeoffset = next_node;
	}

	if (nodeoffset < 0) {
		lprintk("!! Unable to find a node with type ME in the FIT image... !!\n");
		BUG();
	}

	/* Look for a node of flat_dt type in the fit image */
	nodeoffset = 0;
	depth = 0;
	while (nodeoffset >= 0) {
		next_node = fdt_next_node(itb, nodeoffset, &depth);
		ret = fdt_property_read_string(itb, nodeoffset, "type", &propstring);
		if ((ret != -1) && !strcmp(propstring, "flat_dt")) {

			/* Get the associated device tree. */
			ret = fit_image_get_data_and_size(itb, nodeoffset, (const void **) &fdt_vaddr, &fdt_size);
			if (ret) {
				lprintk("!! The properties in the device tree node does not look good !!\n");
				BUG();
			} else
				break;
		}
		nodeoffset = next_node;
	}

	if (nodeoffset < 0) {
		lprintk("!! Unable to find a node with type flat_dt in the FIT image... !!\n");
		BUG();
	}

	/* Fixup the ME device tree */

	/* find or create "/memory" node. */
	nodeoffset = fdt_find_or_add_subnode(fdt_vaddr, 0, "memory");
	BUG_ON(nodeoffset < 0);

	fdt_setprop(fdt_vaddr, nodeoffset, "device_type", "memory", sizeof("memory"));

	size = memslot[slotID].size;

#ifdef CONFIG_ARM64VT
	base = memslot[slotID].ipa_addr;
#else
	base = memslot[slotID].base_paddr;
#endif
	len = fdt_pack_reg(fdt_vaddr, tmp, &base, &size);

	fdt_setprop(fdt_vaddr, nodeoffset, "reg", tmp, len);

	/* Look for a possible node of ramdisk type in the fit image */
	nodeoffset = 0;
	depth = 0;
	while (nodeoffset >= 0) {
		next_node = fdt_next_node(itb, nodeoffset, &depth);
		ret = fdt_property_read_string(itb, nodeoffset, "type", &propstring);
		if ((ret != -1) && !strcmp(propstring, "ramdisk")) {

			/* Get the associated device tree. */
			ret = fit_image_get_data_and_size(itb, nodeoffset, (const void **) &initrd_vaddr, &initrd_size);
			if (ret) {
				lprintk("!! The properties in the ramdisk node does not look good !!\n");
				BUG();
			} else
				break;
		}
		nodeoffset = next_node;
	}

	dest_ME_vaddr = (void *) __lva(memslot[slotID].base_paddr);

	dest_ME_vaddr += L_TEXT_OFFSET;

	/* Move the kernel binary within the domain slotID. */
	memcpy(dest_ME_vaddr, ME_vaddr, ME_size);

	/* Put the FDT device tree close to the top of memory allocated to the domain.
	 * Since there is the initial domain stack at the top, we put the FDT one page (PAGE_SIZE) lower.
	 */
	memslot[slotID].fdt_paddr = memslot[slotID].base_paddr + memslot[slotID].size - PAGE_SIZE;

	memcpy((void *) __lva(memslot[slotID].fdt_paddr), fdt_vaddr, fdt_size);

	/* We put then the initrd (if any) right under the device tree. */

	if (ret == 0) {
		/* Expand the device tree */
		fdt_open_into((void *) __lva(memslot[slotID].fdt_paddr), (void *) __lva(memslot[slotID].fdt_paddr), fdt_size+128);

		/* find or create "/chosen" node. */
		nodeoffset = fdt_find_or_add_subnode((void *) __lva(memslot[slotID].fdt_paddr), 0, "chosen");
		BUG_ON(nodeoffset < 0);

		initrd_start = memslot[slotID].fdt_paddr - initrd_size;
		initrd_start = ALIGN_DOWN(initrd_start, PAGE_SIZE);
		initrd_end = initrd_start + initrd_size;

		ret = fdt_setprop_u32((void *) __lva(memslot[slotID].fdt_paddr), nodeoffset, "linux,initrd-start", (uint32_t) initrd_start);
		BUG_ON(ret != 0);

		ret = fdt_setprop_u32((void *) __lva(memslot[slotID].fdt_paddr), nodeoffset, "linux,initrd-end", (uint32_t) initrd_end);
		BUG_ON(ret != 0);

		memcpy((void *) __lva(initrd_start), initrd_vaddr, initrd_size);
	}
}


