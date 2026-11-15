/*
 * Copyright (C) 2026 Daniel Rossier <daniel.rossier@heig-vd.ch>
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
 * x86_64 bootstrap: CPU tables, multiboot2 boot information and the
 * device tree handed over to the generic code.
 *
 * GRUB loads the device tree and the root filesystem as multiboot2 modules
 * named "dtb" and "rootfs". The memory size and the location of the root
 * filesystem are written into the device tree, as U-Boot does on ARM.
 */

#include <common.h>
#include <memory.h>
#include <string.h>

#include <device/fdt.h>

#include <libfdt/libfdt.h>

#include <asm/io.h>
#include <asm/mmu.h>
#include <asm/processor.h>
#include <asm/semihosting.h>

#define MB2_TAG_END 0
#define MB2_TAG_CMDLINE 1
#define MB2_TAG_MODULE 3
#define MB2_TAG_MMAP 6

#define MB2_MEMORY_AVAILABLE 1

/* Room left in the device tree for the properties added at boot */

#define X86_FDT_SIZE SZ_64K

/* The early 1 GB mapping from head.S bounds what can be reached at boot */

#define X86_BOOT_MAPPED SZ_1G

struct mb2_tag {
	u32 type;
	u32 size;
};

struct mb2_tag_module {
	u32 type;
	u32 size;
	u32 mod_start;
	u32 mod_end;
	char cmdline[];
};

struct mb2_mmap_entry {
	u64 addr;
	u64 len;
	u32 type;
	u32 zero;
};

struct mb2_tag_mmap {
	u32 type;
	u32 size;
	u32 entry_size;
	u32 entry_version;
	struct mb2_mmap_entry entries[];
};

struct x86_tss {
	u32 reserved0;
	u64 rsp0;
	u64 rsp1;
	u64 rsp2;
	u64 reserved1;
	u64 ist[7];
	u64 reserved2;
	u16 reserved3;
	u16 iomap_base;
} __attribute__((packed));

struct x86_idt_entry {
	u16 offset_lo;
	u16 selector;
	u8 ist;
	u8 type_attr;
	u16 offset_mid;
	u32 offset_hi;
	u32 zero;
} __attribute__((packed));

struct x86_desc_ptr {
	u16 limit;
	u64 base;
} __attribute__((packed));

extern char x86_idt_stubs[];
extern void x86_syscall_entry(void);
extern u64 x86_kernel_rsp;

/* Device tree used by the generic code, set by early_memory_init() */

void *__fdt_addr;

static u64 gdt[7] __attribute__((aligned(16)));
static struct x86_tss tss __attribute__((aligned(16)));
static struct x86_idt_entry idt[256] __attribute__((aligned(16)));

static u8 x86_fdt[X86_FDT_SIZE] __attribute__((aligned(8)));

void kernel_start(void);
void x86_fpu_init(void);

void x86_set_kernel_stack(addr_t sp)
{
	tss.rsp0 = sp;
	x86_kernel_rsp = sp;
}

/*
 * Flat GDT: kernel code/data, then user data/code in the order SYSRET
 * expects, then the 16-byte TSS descriptor.
 */

static void gdt_init(void)
{
	struct x86_desc_ptr gdtr;
	u64 base = (u64) &tss;
	u64 limit = sizeof(tss) - 1;

	gdt[0] = 0;
	gdt[1] = 0x00af9a000000ffffUL; /* KERNEL_CS */
	gdt[2] = 0x00cf92000000ffffUL; /* KERNEL_DS */
	gdt[3] = 0x00cff2000000ffffUL; /* USER_DS */
	gdt[4] = 0x00affa000000ffffUL; /* USER_CS */

	gdt[5] = (limit & 0xffff) | ((base & 0xffffff) << 16) | (0x89UL << 40) | (((limit >> 16) & 0xf) << 48) |
		 (((base >> 24) & 0xff) << 56);
	gdt[6] = base >> 32;

	tss.iomap_base = sizeof(tss);

	gdtr.limit = sizeof(gdt) - 1;
	gdtr.base = (u64) gdt;

	asm volatile("lgdt %0\n"
		     "mov %1, %%ds\n"
		     "mov %1, %%es\n"
		     "mov %1, %%ss\n"
		     "pushq %2\n"
		     "leaq 1f(%%rip), %%rax\n"
		     "pushq %%rax\n"
		     "lretq\n"
		     "1:\n"
		     :
		     : "m"(gdtr), "r"((u64) KERNEL_DS), "i"(KERNEL_CS)
		     : "rax", "memory");

	asm volatile("ltr %w0" : : "r"(TSS_SEL));
}

static void idt_set_gate(int vec, addr_t handler, u8 type_attr)
{
	idt[vec].offset_lo = handler & 0xffff;
	idt[vec].selector = KERNEL_CS;
	idt[vec].ist = 0;
	idt[vec].type_attr = type_attr;
	idt[vec].offset_mid = (handler >> 16) & 0xffff;
	idt[vec].offset_hi = handler >> 32;
	idt[vec].zero = 0;
}

/* Interrupt gates only: IF is cleared on every entry, as on ARM. */

static void idt_init(void)
{
	struct x86_desc_ptr idtr;
	int i;

	for (i = 0; i < 48; i++)
		idt_set_gate(i, (addr_t) x86_idt_stubs + i * 16, 0x8e);

	idtr.limit = sizeof(idt) - 1;
	idtr.base = (u64) idt;

	asm volatile("lidt %0" : : "m"(idtr));
}

static void cpu_features_init(void)
{
	u64 cr0, cr4;

	/* FPU and SSE for user space; the kernel itself is built without them */

	asm volatile("mov %%cr0, %0" : "=r"(cr0));
	cr0 &= ~(X86_CR0_EM | X86_CR0_TS);
	cr0 |= X86_CR0_MP | X86_CR0_NE;
	asm volatile("mov %0, %%cr0" : : "r"(cr0));

	asm volatile("mov %%cr4, %0" : "=r"(cr4));
	cr4 |= X86_CR4_OSFXSR | X86_CR4_OSXMMEXCPT;
	asm volatile("mov %0, %%cr4" : : "r"(cr4));

	x86_fpu_init();

	/* syscall/sysret: kernel selectors in STAR[47:32], user base in STAR[63:48] */

	wrmsr(MSR_EFER, rdmsr(MSR_EFER) | EFER_SCE);
	wrmsr(MSR_STAR, ((u64) (KERNEL_DS | 3) << 48) | ((u64) KERNEL_CS << 32));
	wrmsr(MSR_LSTAR, (u64) x86_syscall_entry);
	wrmsr(MSR_SFMASK, X86_EFLAGS_IF | X86_EFLAGS_DF | X86_EFLAGS_TF);

	wrmsr(MSR_FS_BASE, 0);
	wrmsr(MSR_GS_BASE, 0);
}

static void *mb2_va(u64 paddr)
{
	return (void *) (CONFIG_KERNEL_VADDR + paddr);
}

/* End of the usable RAM region which holds the kernel */

static u64 mb2_ram_end(struct mb2_tag_mmap *mmap)
{
	struct mb2_mmap_entry *e;
	u8 *p;
	u64 kernel_paddr = __pa(__end);

	for (p = (u8 *) mmap->entries; p < (u8 *) mmap + mmap->size; p += mmap->entry_size) {
		e = (struct mb2_mmap_entry *) p;

		if ((e->type == MB2_MEMORY_AVAILABLE) && (e->addr <= kernel_paddr) && (e->addr + e->len > kernel_paddr))
			return (e->addr + e->len) & PAGE_MASK;
	}

	return 0;
}

static void fdt_check(int err, const char *what)
{
	if (err < 0) {
		lprintk("x86 boot: %s: %s\n", what, fdt_strerror(err));
		kernel_panic();
	}
}

static void fdt_set_reg64(void *fdt, const char *path, const char *name, u64 val)
{
	int offset;

	offset = fdt_path_offset(fdt, path);
	if (offset == -FDT_ERR_NOTFOUND)
		offset = fdt_add_subnode(fdt, 0, path + 1);
	fdt_check(offset, path);

	fdt_check(fdt_setprop_u64(fdt, offset, name, val), name);
}

/*
 * Parse the boot information and build the device tree passed to the
 * generic code. The boot information and the modules may lie right after
 * the kernel image, where the generic code puts the device tree and the frame
 * table: everything needed is copied or moved away first.
 */

static void mb2_setup(u32 mbi_paddr)
{
	struct mb2_tag *tag;
	struct mb2_tag_module *mod;
	struct mb2_tag_module *dtb_mod = NULL, *rootfs_mod = NULL;
	struct mb2_tag_mmap *mmap = NULL;
	u64 ram_end, rootfs_size = 0, rootfs_paddr = 0;
	fdt64_t reg[2];
	int offset;

	tag = (struct mb2_tag *) mb2_va(mbi_paddr + 8);

	while (tag->type != MB2_TAG_END) {
		switch (tag->type) {
		case MB2_TAG_MODULE:
			mod = (struct mb2_tag_module *) tag;

			if (!strcmp(mod->cmdline, "dtb"))
				dtb_mod = mod;
			else if (!strcmp(mod->cmdline, "rootfs"))
				rootfs_mod = mod;
			break;

		case MB2_TAG_MMAP:
			mmap = (struct mb2_tag_mmap *) tag;
			break;
		}

		tag = (struct mb2_tag *) ALIGN_UP((addr_t) tag + tag->size, 8);
	}

	if (!dtb_mod) {
		lprintk("x86 boot: no \"dtb\" multiboot2 module, cannot go further.\n");
		kernel_panic();
	}

	if (!mmap) {
		lprintk("x86 boot: no memory map from the bootloader.\n");
		kernel_panic();
	}

	fdt_check(fdt_open_into(mb2_va(dtb_mod->mod_start), x86_fdt, sizeof(x86_fdt)), "dtb");

	ram_end = mb2_ram_end(mmap);
	if (!ram_end) {
		lprintk("x86 boot: the kernel is not in usable RAM.\n");
		kernel_panic();
	}

	if (ram_end > X86_BOOT_MAPPED)
		ram_end = X86_BOOT_MAPPED;

	/* The root filesystem goes to the top of the RAM, out of the way. */

	if (rootfs_mod) {
		rootfs_size = rootfs_mod->mod_end - rootfs_mod->mod_start;
		rootfs_paddr = (ram_end - rootfs_size) & PAGE_MASK;

		memmove(mb2_va(rootfs_paddr), mb2_va(rootfs_mod->mod_start), rootfs_size);
	}

	/* RAM seen by SO3: from 0 to the end of the region holding the kernel */

	offset = fdt_path_offset(x86_fdt, "/memory");
	fdt_check(offset, "/memory");

	reg[0] = cpu_to_fdt64(0);
	reg[1] = cpu_to_fdt64(ram_end);
	fdt_check(fdt_setprop(x86_fdt, offset, "reg", reg, sizeof(reg)), "/memory reg");

	if (rootfs_mod) {
		fdt_set_reg64(x86_fdt, "/chosen", "linux,initrd-start", rootfs_paddr);
		fdt_set_reg64(x86_fdt, "/chosen", "linux,initrd-end", rootfs_paddr + rootfs_size);
	}

	fdt_pack(x86_fdt);
}

void x86_boot(u32 mbi_paddr)
{
	gdt_init();
	idt_init();
	cpu_features_init();

	mb2_setup(mbi_paddr);

	early_memory_init((void *) __pa(x86_fdt));

	kernel_start();
}

/* QEMU isa-debug-exit device, when the machine has one (-device isa-debug-exit) */

#define X86_DEBUG_EXIT_PORT 0xf4

void semi_exit(int code)
{
	outb(X86_DEBUG_EXIT_PORT, code);

	local_irq_disable();
	while (1)
		wfi();
}
