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

#include <generated/autoconf.h>
#include <linkage.h>

#include <asm/processor.h>

#define MB2_MAGIC 0xe85250d6
#define MB2_ARCH_I386 0
#define MB2_BOOTLOADER_MAGIC 0x36d76289

#define PHYS(x) ((x) - CONFIG_KERNEL_VADDR)

#define SYS_STACK_SIZE (CONFIG_SYS_STACK_SIZE_KB * 1024)

.section ".multiboot", "a"
.align 8

mb2_header:
	.long	MB2_MAGIC
	.long	MB2_ARCH_I386
	.long	mb2_header_end - mb2_header
	.long	-(MB2_MAGIC + MB2_ARCH_I386 + (mb2_header_end - mb2_header))

	/* Entry address tag: GRUB jumps to the physical 32-bit entry */

	.align 8
	.short	3
	.short	0
	.long	12
	.long	PHYS(__start32)

	/* End tag */

	.align 8
	.short	0
	.short	0
	.long	8
mb2_header_end:

/*
 * GRUB enters here in 32-bit protected mode, paging off, with
 * eax = multiboot2 magic and ebx = physical address of the boot info.
 */

.section ".head.text", "ax"
.code32

ENTRY(__start32)
	cli
	cld

	mov	%eax, %edi
	mov	%ebx, %esi

	mov	$PHYS(__stack_bottom + SYS_STACK_SIZE), %esp

	cmp	$MB2_BOOTLOADER_MAGIC, %edi
	jne	__boot_hang

	/* Clear the four boot page tables */

	push	%edi
	mov	$PHYS(__sys_root_pgtable), %edi
	mov	$(4 * 4096 / 4), %ecx
	xor	%eax, %eax
	rep stosl
	pop	%edi

	/*
	 * PML4[0]   -> idmap PDPT,  PDPT[0]   -> PD   (identity for this code)
	 * PML4[511] -> kernel PDPT, PDPT[510] -> PD   (0xffffffff80000000)
	 * The PD maps the first GB with 2 MB pages.
	 */

	mov	$PHYS(__sys_idmap_l1pgtable + 3), %eax
	mov	%eax, PHYS(__sys_root_pgtable)

	mov	$PHYS(__sys_linearmap_l1pgtable + 3), %eax
	mov	%eax, PHYS(__sys_root_pgtable + 511 * 8)

	mov	$PHYS(__sys_linearmap_l2pgtable + 3), %eax
	mov	%eax, PHYS(__sys_idmap_l1pgtable)
	mov	%eax, PHYS(__sys_linearmap_l1pgtable + 510 * 8)

	mov	$PHYS(__sys_linearmap_l2pgtable), %ebx
	mov	$0x83, %eax
	mov	$512, %ecx
1:
	mov	%eax, (%ebx)
	add	$0x200000, %eax
	add	$8, %ebx
	loop	1b

	/* PAE, then long mode, then paging */

	mov	%cr4, %eax
	or	$X86_CR4_PAE, %eax
	mov	%eax, %cr4

	mov	$PHYS(__sys_root_pgtable), %eax
	mov	%eax, %cr3

	mov	$MSR_EFER, %ecx
	rdmsr
	or	$(EFER_LME | EFER_SCE), %eax
	wrmsr

	mov	%cr0, %eax
	or	$(X86_CR0_PG | X86_CR0_PE | X86_CR0_WP), %eax
	mov	%eax, %cr0

	lgdt	PHYS(boot_gdt_desc)

	ljmp	$KERNEL_CS, $PHYS(__start64_low)

__boot_hang:
	hlt
	jmp	__boot_hang

.code64

__start64_low:
	movabs	$__start64, %rax
	jmp	*%rax

.text

__start64:
	xor	%eax, %eax
	mov	%eax, %ds
	mov	%eax, %es
	mov	%eax, %ss
	mov	%eax, %fs
	mov	%eax, %gs

	movabs	$(__stack_bottom + SYS_STACK_SIZE), %rsp

	/* Reload the GDT pointer through its high address */

	movabs	$boot_gdt_desc_high, %rax
	lgdt	(%rax)

	/* The boot info address is a 32-bit physical address */

	mov	%esi, %edi
	call	x86_boot

1:
	hlt
	jmp	1b

.section ".rodata"
.align 16

boot_gdt:
	.quad	0
	.quad	0x00af9a000000ffff	/* 0x08: 64-bit kernel code */
	.quad	0x00cf92000000ffff	/* 0x10: kernel data */
boot_gdt_end:

boot_gdt_desc:
	.short	boot_gdt_end - boot_gdt - 1
	.long	PHYS(boot_gdt)

.align 8
boot_gdt_desc_high:
	.short	boot_gdt_end - boot_gdt - 1
	.quad	boot_gdt

.section .note.GNU-stack, "", @progbits
