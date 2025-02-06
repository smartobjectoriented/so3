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

#include <memory.h>
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
 * Retrieve the physical address of the AVZ device tree which is loaded by U-boot
 * as a "loadable" component.
 *
 * The same procedure will be done later on when the MMU is enabled. We do not store
 * the address to avoid relocation issue.
 *
 * @param agency_fdt_paddr
 * @return
 */
addr_t __get_avz_fdt_paddr(void *agency_fdt_paddr)
{
	int nodeoffset, next_node;
	int depth, ret;
	addr_t avz_dt_paddr;
	const char *propstring;
	bool found = false;
	volatile char *ptr;
	u64 val;
	const fdt64_t *fdt_val;
	int i;

	nodeoffset = 0;
	depth = 0;

	while (!found && (nodeoffset >= 0)) {
		next_node = fdt_next_node(agency_fdt_paddr, nodeoffset, &depth);

		ret = fdt_property_read_string(agency_fdt_paddr, nodeoffset,
					       "type", &propstring);

		/* Process the type "avz" to get the AVZ device tree. This node comes from SO3 (AVZ) ITS but is merged in the linux DTS afterwards */
		if ((ret != -1) && !strcmp(propstring, "avz_dt")) {
			/* According to U-boot, the <load> and <entry> properties are both on 64-bit even for aarch32 configuration. */

			fdt_val = fdt_getprop(agency_fdt_paddr, nodeoffset,
					      "load", NULL);

			/* We avoid to make a memory access beyond the byte,
			 * since the function is called from head.S with MMU off
			 * and memory access must be aligned.
			 */

			ptr = (char *)fdt_val;

			for (i = 0; i < 8; i++)
				*(((char *)&val) + i) = *ptr++;

			avz_dt_paddr = fdt64_to_cpu(val);

			found = true;
		}
		nodeoffset = next_node;
	}

	if (!found) {
		lprintk("!! Unable to find a node with type avz and/or avz_dt in the FIT image... !!\n");
		BUG();
	}

	/* Assign the agency DT addr to the general __fdt_addr here
	 * since loadAgency() will need it to parse the complete device tree.
	 * It will be re-adjusted at the end of loadAgency().
	 */
	__fdt_addr =
		agency_fdt_paddr; /* Mapped as with direct-mapping by mmu_configure() */

	return avz_dt_paddr;
}

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
	addr_t base;
	int depth, ret;
	const char *propstring;
	mem_info_t guest_mem_info;
	void *fdt_vaddr = __fdt_addr;

	const struct fdt_property *initrd_start, *initrd_end;
	u64 entry_addr;
	int lenp;

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

		ret = fdt_property_read_string(fdt_vaddr, nodeoffset, "type",
					       &propstring);

		/* Process the type "avz" to get the AVZ device tree */
		if ((ret != -1) && !strcmp(propstring, "avz_dt")) {
			ret = fdt_property_read_u64(fdt_vaddr, nodeoffset,
						    "load",
						    (u64 *)&avz_dt_addr);
			if (ret == -1) {
				lprintk("!! Missing load-addr in the avz_dt node !!\n");
				BUG();
			}

			count++;
		}

		/* Process the type "avz" to get the guest image */
		if ((ret != -1) && !strcmp(propstring, "guest")) {
			/* According to U-boot, the <load> and <entry> properties are both on 64-bit even for aarch32 configuration. */

			ret = fdt_property_read_u64(fdt_vaddr, nodeoffset,
						    "load", (u64 *)&dom_addr);
			if (ret == -1) {
				lprintk("!! Missing load-addr in the agency node !!\n");
				BUG();
			}
			lprintk("ITB: Domain load addr = 0x%lx\n", dom_addr);

			ret = fdt_property_read_u64(fdt_vaddr, nodeoffset,
						    "entry",
						    (u64 *)&entry_addr);
			if (ret == -1) {
				lprintk("!! Missing entry in the agency node !!\n");
				BUG();
			}
			lprintk("ITB: Domain entry addr = 0x%lx\n", entry_addr);

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
	memslot[MEMSLOT_AGENCY].base_vaddr = AGENCY_VOFFSET;

	memslot[MEMSLOT_AGENCY].fdt_paddr = (addr_t)__fdt_addr;

	/* Retrieve the memory addr and size of the guest */
	get_mem_info(fdt_vaddr, &guest_mem_info);

	memslot[MEMSLOT_AGENCY].ipa_addr = (addr_t)guest_mem_info.phys_base;
	memslot[MEMSLOT_AGENCY].size = guest_mem_info.size;

	memslot[MEMSLOT_AGENCY].entry_addr = entry_addr;

	if (!memslot[MEMSLOT_AGENCY].size) {
		lprintk("!! Property memory of the guest has a size of 0 byte...\n");
		BUG();
	}

	lprintk("IPA Layout: device tree located at 0x%lx\n",
		memslot[MEMSLOT_AGENCY].fdt_paddr);

	/* 
	 * Fixup of initrd_start and initrd_end 
	 * These addresses are *real* physical address retrieved from the ITS.
	 * However, Linux needs physical address in its virtualized RAM (IPA addresses),
	 * so we need to fixup these addresses.
	 *
	 */
	nodeoffset = 0;
	depth = 0;

	while (nodeoffset >= 0) {
		next_node = fdt_next_node(fdt_vaddr, nodeoffset, &depth);

		initrd_start = fdt_get_property(fdt_vaddr, nodeoffset,
						"linux,initrd-start", &lenp);

		if (initrd_start) {
			initrd_end = fdt_get_property(fdt_vaddr, nodeoffset,
						      "linux,initrd-end",
						      &lenp);
			BUG_ON(!initrd_end);

			base = fdt64_to_cpu(
				((const fdt64_t *)initrd_start->data)[0]);
			base = pa_to_ipa(MEMSLOT_AGENCY, base);
			lprintk("IPA Layout: initrd start at 0x%lx\n", base);

			fdt_setprop_u64(fdt_vaddr, nodeoffset,
					"linux,initrd-start", base);

			base = fdt64_to_cpu(
				((const fdt64_t *)initrd_end->data)[0]);
			base = pa_to_ipa(MEMSLOT_AGENCY, base);
			lprintk("IPA Layout: initrd end at 0x%lx\n", base);

			fdt_setprop_u64(fdt_vaddr, nodeoffset,
					"linux,initrd-end", base);

			break;
		}
		nodeoffset = next_node;
	}

	/* Update our reference to the AVZ device tree */
	__fdt_addr = (void *)memslot[MEMSLOT_AVZ].fdt_paddr;

	/* Retrieve the memory info in the AVZ DT */
	/* Access to device tree */
	/* __fdt_addr MUST BE mapped in an identity mapping */

	/* Get the RAM information of the board */
	early_memory_init(__fdt_addr);

	lprintk("  AVZ DT at physical address : %lx\n", __fdt_addr);
	lprintk("  AVZ memory descriptor : found %d MB of RAM at 0x%08X\n",
		mem_info.size / SZ_1M, mem_info.phys_base);

	memslot[MEMSLOT_AVZ].base_paddr = mem_info.phys_base;
	memslot[MEMSLOT_AVZ].base_vaddr = CONFIG_KERNEL_VADDR;

	/* Here, the memory size corresponds to the whole RAM base available in the platform */
	memslot[MEMSLOT_AVZ].size = mem_info.size;

	memslot[MEMSLOT_AVZ].busy = true;
}

/**
 * The ITB image will be parsed and the components placed in their target memory location.
 * This work only with ARM64VT support.
 *
 * @param slotID
 * @param itb	ITB image
 */
void loadME(unsigned int slotID, void *itb)
{
	void *ME_vaddr;
	uint32_t dom_addr, entry_addr, fdt_paddr;
	size_t ME_size, fdt_size, initrd_size;
	void *fdt_vaddr, *initrd_vaddr;
	void *dest_ME_vaddr;
	uint32_t initrd_start, initrd_end;
	int nodeoffset, next_node, depth = 0;
	int ret;
	const char *propstring;
	mem_info_t guest_mem_info;

	/* Look for a node of ME type in the fit image */
	nodeoffset = 0;
	depth = 0;
	while (nodeoffset >= 0) {
		next_node = fdt_next_node(itb, nodeoffset, &depth);
		ret = fdt_property_read_string(itb, nodeoffset, "type",
					       &propstring);

		if ((ret != -1) && !strcmp(propstring, "guest")) {
			ret = fdt_property_read_u32(itb, nodeoffset, "load",
						    &dom_addr);
			if (ret == -1) {
				lprintk("!! Missing load-addr in the agency node !!\n");
				BUG();
			}
			lprintk("ITB: Domain load addr = 0x%lx\n", dom_addr);

			ret = fdt_property_read_u32(itb, nodeoffset, "entry",
						    &entry_addr);
			if (ret == -1) {
				lprintk("!! Missing entry in the agency node !!\n");
				BUG();
			}
			lprintk("ITB: Domain entry addr = 0x%lx\n", entry_addr);

			/* Get the pointer to the OS binary image from the ITB we got from the user space. */
			ret = fit_image_get_data_and_size(
				itb, nodeoffset, (const void **)&ME_vaddr,
				&ME_size);
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
	};

	/* Look for a node of flat_dt type in the fit image */
	nodeoffset = 0;
	depth = 0;
	while (nodeoffset >= 0) {
		next_node = fdt_next_node(itb, nodeoffset, &depth);
		ret = fdt_property_read_string(itb, nodeoffset, "type",
					       &propstring);

		if ((ret != -1) && !strcmp(propstring, "flat_dt")) {
			ret = fdt_property_read_u32(itb, nodeoffset, "load",
						    &fdt_paddr);
			if (ret == -1) {
				lprintk("!! Missing load-addr in the agency node !!\n");
				BUG();
			}

			/* Get the associated device tree. */
			ret = fit_image_get_data_and_size(
				itb, nodeoffset, (const void **)&fdt_vaddr,
				&fdt_size);
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

	/* Retrieve the guest memory information */
	get_mem_info(fdt_vaddr, &guest_mem_info);

	/* From the DTS here, we consider the physical address as the IPA base address. */
	memslot[slotID].ipa_addr = guest_mem_info.phys_base;

	/* Look for a possible node of ramdisk type in the fit image */
	nodeoffset = 0;
	depth = 0;
	while (nodeoffset >= 0) {
		next_node = fdt_next_node(itb, nodeoffset, &depth);
		ret = fdt_property_read_string(itb, nodeoffset, "type",
					       &propstring);
		if ((ret != -1) && !strcmp(propstring, "ramdisk")) {
			ret = fdt_property_read_u32(itb, nodeoffset, "load",
						    &initrd_start);
			if (ret == -1) {
				lprintk("!! Missing load-addr in the agency node !!\n");
				BUG();
			}

			/* Get the associated initrd ramfs */
			ret = fit_image_get_data_and_size(
				itb, nodeoffset, (const void **)&initrd_vaddr,
				&initrd_size);
			if (ret) {
				lprintk("!! The properties in the ramdisk node does not look good !!\n");
				BUG();
			} else
				break;
		}

		nodeoffset = next_node;
	}

	dest_ME_vaddr = (void *)memslot[slotID].base_vaddr;

	dest_ME_vaddr += L_TEXT_OFFSET;

	/* Move the kernel binary within the domain slotID. */
	memcpy(dest_ME_vaddr, ME_vaddr, ME_size);

	memslot[slotID].fdt_paddr = ipa_to_pa(slotID, fdt_paddr);

	memcpy((void *)__xva(slotID, memslot[slotID].fdt_paddr), fdt_vaddr,
	       fdt_size);

	/* <ret> still has the result of the ramdisk presence (see above). */
	if (ret == 0) {
		/* Expand the device tree */
		fdt_open_into((void *)__xva(slotID, memslot[slotID].fdt_paddr),
			      (void *)__xva(slotID, memslot[slotID].fdt_paddr),
			      fdt_size + 128);

		/* find or create "/chosen" node. */
		nodeoffset = fdt_find_or_add_subnode(
			(void *)__xva(slotID, memslot[slotID].fdt_paddr), 0,
			"chosen");
		BUG_ON(nodeoffset < 0);

		initrd_end = initrd_start + initrd_size;

		ret = fdt_setprop_u64((void *)__xva(slotID,
						    memslot[slotID].fdt_paddr),
				      nodeoffset, "linux,initrd-start",
				      (uint32_t)initrd_start);
		BUG_ON(ret != 0);

		ret = fdt_setprop_u64(
			(void *)__xva(slotID, memslot[slotID].fdt_paddr),
			nodeoffset, "linux,initrd-end", (uint32_t)initrd_end);
		BUG_ON(ret != 0);

		memcpy((void *)ipa_to_va(slotID, initrd_start), initrd_vaddr,
		       initrd_size);
	}
}
