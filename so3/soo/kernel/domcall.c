/*
 * Copyright (C) 2016-2018 Daniel Rossier <daniel.rossier@soo.tech>
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
#include <heap.h>

#include <asm/mmu.h>
#include <asm/cacheflush.h>
#include <asm/processor.h>

#include <soo/hypervisor.h>
#include <soo/vbus.h>
#include <soo/console.h>

/**
 * Keep track of already updated base address entries for L2 page tables.
 * A base address is encoded on 22 bits, since we have to keep track of such addr (coarse page table).
 *
 * All L2 page tables are allocated in the heap. So, to determine the position in the bitmap,
 * we get the offset between the heap base address and the page table base address.
 *
 * Furthermore, a L2 page table may contain up to 256 entries. It means that the base address
 * is always aligned on 1 KB (256 * 4).
 */

/* We are able to keep track of base addresses for 2 MB RAM with 4 KB pages (1 bit per page) */

/*
 * If the heap is sized to 2 MB, 2*2^20 >> 10 = 2*2^10 = 2'048 bits, for max 2'048 possible base addresses.
 * 2'048 / 8 = 256 bytes.
 *
 */
#define BASEADDR_BITMAP_BYTES 	256

#if 0
extern addr_t __heap_base_addr;
static addr_t heap_base_vaddr = (addr_t ) &__heap_base_addr;


static unsigned char baseaddr_2nd_bitmap[BASEADDR_BITMAP_BYTES];

/* Init the bitmap */
static void init_baseaddr_2nd_bitmap(void) {
	int i;

	for (i = 0; i < BASEADDR_BITMAP_BYTES; i++)
		baseaddr_2nd_bitmap[i] = 0;
}

/*
 * Page tables can start at 1 KB-aligned address
 */
static void set_baseaddr_2nd_bitmap(unsigned int baseaddr) {
	unsigned int pos, mod;

	baseaddr = (baseaddr - (CONFIG_RAM_BASE + (heap_base_vaddr - CONFIG_KERNEL_VADDR))) >> 10;

	pos = baseaddr >> 3;
	mod = baseaddr % 8;

	BUG_ON(pos >= BASEADDR_BITMAP_BYTES);

	baseaddr_2nd_bitmap[pos] |= (1 << (7 - mod));

}

static unsigned int is_set_baseaddr_2nd_bitmap(unsigned int baseaddr) {
	unsigned int pos, mod;

	baseaddr = (baseaddr - (CONFIG_RAM_BASE + (heap_base_vaddr - CONFIG_KERNEL_VADDR))) >> 10;

	pos = baseaddr >> 3;
	mod = baseaddr % 8;

	BUG_ON(pos >= BASEADDR_BITMAP_BYTES);

	return (baseaddr_2nd_bitmap[pos] & (1 << (7 - mod)));

}


/****************************************************/

/* Page walking */

static void set_l2pte(uint32_t *l2pte, struct DOMCALL_fix_page_tables_args *args) {
	unsigned int base = 0;

	base = *l2pte & PAGE_MASK;
	base += pfn_to_phys(args->pfn_offset);

	/*
	 * Check if the physical address is really within the RAM (subject to be adujsted).
	 * I/O addresses must not be touched.
	 */
	if ((phys_to_pfn(base) >= args->min_pfn) && (phys_to_pfn(base) < args->min_pfn + args->nr_pages)) {

		*l2pte = (*l2pte & ~PAGE_MASK) | base;

		flush_pte_entry(l2pte);
	}
}

/* Process all PTEs of a 2nd-level page table */
static void process_pte_pgtable(uint32_t *l1pte, struct DOMCALL_fix_page_tables_args *args) {

	uint32_t *l2pte;
	int i;

	l2pte = (uint32_t *) __va(*l1pte & TTB_L1_PAGE_ADDR_MASK);

	/* Walk through the L2 PTEs. */

	for (i = 0; i < TTB_L2_ENTRIES; i++, l2pte++)
		if (*l2pte)
			set_l2pte(l2pte, args);
}

static void set_l1pte(uint32_t *l1pte, struct DOMCALL_fix_page_tables_args *args) {
	volatile unsigned int base;

	if (l1pte_is_sect(*l1pte))
		base = *l1pte & TTB_L1_SECT_ADDR_MASK;
	else {
		ASSERT(l1pte_is_pt(*l1pte));
		base = (*l1pte & TTB_L1_PAGE_ADDR_MASK);
	}

	base += pfn_to_phys(args->pfn_offset);

	/*
	 * Check if the physical address is really within the RAM (subject to be adjusted).
	 * I/O addresses must not be touched.
	 */
	if ((phys_to_pfn(base) >= args->min_pfn) && (phys_to_pfn(base) < args->min_pfn + args->nr_pages)) {

		if (l1pte_is_sect(*l1pte))
			*l1pte = (*l1pte & ~TTB_L1_SECT_ADDR_MASK) | base;
		else
			*l1pte = (*l1pte & ~TTB_L1_PAGE_ADDR_MASK) | base;


		flush_pte_entry(l1pte);
	}

}

int adjust_l1_page_tables(unsigned long addr, unsigned long end, uint32_t *pgtable, struct DOMCALL_fix_page_tables_args *args)
{
	uint32_t *l1pte;
	unsigned int sect_addr;

	l1pte = l1pte_offset(pgtable, addr);

	sect_addr = (addr >> 20) - 1;

	do {
		sect_addr++;

		if (*l1pte)
			set_l1pte(l1pte, args);

		l1pte++;

	} while (sect_addr != ((end-1) >> 20));

	return 0;
}

int adjust_l2_page_tables(unsigned long addr, unsigned long end, uint32_t *pgtable, struct DOMCALL_fix_page_tables_args *args)
{
	uint32_t *l1pte;
	unsigned int base;
	unsigned int sect_addr;

	l1pte = l1pte_offset(pgtable, addr);
	sect_addr = (addr >> 20) - 1;

	do {
		sect_addr++;

		if (l1pte_is_pt(*l1pte)) {
			base = *l1pte & TTB_L1_PAGE_ADDR_MASK;

			if (!is_set_baseaddr_2nd_bitmap(base)) {

				process_pte_pgtable(l1pte, args);

				/* We keep track of one pfn only, since they are always processed simultaneously */
				set_baseaddr_2nd_bitmap(base);
			}
		}

		l1pte++;

	} while (sect_addr != ((end-1) >> TTB_I1_SHIFT));

	return 0;
}

/****************************************************/

/**
 * Fixup the page tables belonging to processes (user & kernel space).
 *
 * @param args	arguments of this domcall
 */
static void do_fix_other_page_tables(struct DOMCALL_fix_page_tables_args *args) {
	pcb_t *pcb;
	uint32_t vaddr;
	uint32_t *l1pte, *l1pte_current;

	set_pfn_offset(args->pfn_offset);

	init_baseaddr_2nd_bitmap();

	/* All page tables used by processes must be adapted. */

	list_for_each_entry(pcb, &proc_list, list)
	{

		for (vaddr = CONFIG_KERNEL_VADDR; ((vaddr != 0) && (vaddr < 0xffffffff)); vaddr += TTB_SECT_SIZE) {
			l1pte = l1pte_offset(pcb->pgtable, vaddr);
			l1pte_current = l1pte_offset(__sys_root_pgtable, vaddr);

			*l1pte = *l1pte_current;

			flush_pte_entry((void *) l1pte);
		}

		/* Finally, remap the whole user space */

		adjust_l1_page_tables(0, CONFIG_KERNEL_VADDR, pcb->pgtable, args);
		adjust_l2_page_tables(0, CONFIG_KERNEL_VADDR, pcb->pgtable, args);

		mmu_page_table_flush((uint32_t) pcb->pgtable, (uint32_t) (pcb->pgtable + TTB_L1_ENTRIES));
	}

}

#endif

/* Main callback function used by AVZ */
void domcall(int cmd, void *arg)
{
	switch (cmd) {

	case DOMCALL_presetup_adjust_variables:
		do_presetup_adjust_variables(arg);
		break;

	case DOMCALL_postsetup_adjust_variables:
		do_postsetup_adjust_variables(arg);
		break;

	case DOMCALL_fix_other_page_tables:
		//do_fix_other_page_tables((struct DOMCALL_fix_page_tables_args *) arg);
		break;

	case DOMCALL_sync_domain_interactions:
		do_sync_domain_interactions(arg);
		break;

	case DOMCALL_sync_directcomm:
		do_sync_directcomm(arg);
		break;

	case DOMCALL_soo:
		do_soo_activity(arg);
		break;

	default:
		printk("Unknowmn domcall %#x\n", cmd);
		BUG();
		break;
	}
}


/**
 * Enable the cooperation between this ME and the other.
 */
void spad_enable_cooperate(void) {
	avz_shared->dom_desc.u.ME.spad.valid = true;
}

/**
 * Enable the cooperation between this ME and the other.
 */
void spad_disable_cooperate(void) {
	avz_shared->dom_desc.u.ME.spad.valid = false;
}

