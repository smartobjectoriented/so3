/*
 * Linker script for the SO3 x86_64 kernel
 *
 * The kernel runs in the top 2 GB (KERNEL_VADDR = 0xffffffff80000000,
 * required by -mcmodel=kernel) and is loaded by GRUB at the physical
 * address LOAD_OFFSET, so every section has LMA = VMA - KERNEL_VADDR.
 */

OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(__start32)

X86_LOAD_OFFSET = 0x200000;

STACK_SIZE = (CONFIG_SYS_STACK_SIZE_KB + CONFIG_MAX_THREADS * CONFIG_THREAD_STACK_SIZE_KB) * 1024;
HEAP_SIZE = CONFIG_HEAP_SIZE_MB * 1024 * 1024;

SECTIONS
{
	. = ENTRY_ADDR + X86_LOAD_OFFSET;

	__kernel_start = .;

	/* The multiboot2 header must lie in the first 32 KB of the file */

	.head.text : AT(ADDR(.head.text) - ENTRY_ADDR)
	{
		KEEP(*(.multiboot))
		*(.head.text)
	} :text

	.text : AT(ADDR(.text) - ENTRY_ADDR)
	{
		*(.text*)
	} :text

	.rodata : AT(ADDR(.rodata) - ENTRY_ADDR)
	{
		*(.rodata*)
	} :text

	. = ALIGN(4096);

	.root_proc_prologue : AT(ADDR(.root_proc_prologue) - ENTRY_ADDR)
	{
		__root_proc_start = .;

		*(.root_proc.text)

		__root_proc_end = .;
	} :text

	. = ALIGN(4096);

	.data : AT(ADDR(.data) - ENTRY_ADDR)
	{
		*(.data*)
	} :data

	/* Initcall sections, kept before .bss so that they are file-backed */

	.initcall_driver_initcall_t_core : AT(ADDR(.initcall_driver_initcall_t_core) - ENTRY_ADDR)
	{
		__initcall_driver_initcall_t_core = .;
		KEEP(*(.initcall_driver_initcall_t_core));
		__initcall_driver_initcall_t_core_end = .;
	} :data

	.initcall_driver_initcall_t_postcore : AT(ADDR(.initcall_driver_initcall_t_postcore) - ENTRY_ADDR)
	{
		__initcall_driver_initcall_t_postcore = .;
		KEEP(*(.initcall_driver_initcall_t_postcore));
		__initcall_driver_initcall_t_postcore_end = .;
	} :data

	.initcall_pre_init_t_core : AT(ADDR(.initcall_pre_init_t_core) - ENTRY_ADDR)
	{
		__initcall_pre_irq_init_t_core = .;
		KEEP(*(.initcall_pre_irq_init_t_core));
		__initcall_pre_irq_init_t_core_end = .;
	} :data

	.initcall_postinit_t_core : AT(ADDR(.initcall_postinit_t_core) - ENTRY_ADDR)
	{
		__initcall_postinit_t_core = .;
		KEEP(*(.initcall_postinit_t_core));
		__initcall_postinit_t_core_end = .;
	} :data

	. = ALIGN(4096);

	__bss_start = .;

	.bss (NOLOAD) : AT(ADDR(.bss) - ENTRY_ADDR)
	{
		*(.sbss*)
		*(.bss .bss.*)
		*(COMMON)

		. = ALIGN(4096);

		__per_cpu_start = .;

		*(.percpu_data)

		. = ALIGN(128);
		*(.percpu_data.read_mostly)

		. = ALIGN(128);

		__per_cpu_data_end = .;

		. = ALIGN(128);

		__per_cpu_end = .;
	} :data

	__bss_end = .;

	/*
	 * Boot page tables: PML4, PDPT for the identity map, PDPT for the
	 * kernel map and one page directory shared by both (1 GB of 2 MB pages).
	 */

	.pgtables (NOLOAD) : AT(ADDR(.pgtables) - ENTRY_ADDR)
	{
		. = ALIGN(4096);

		__sys_root_pgtable = .;
		. += 4096;

		__sys_idmap_l1pgtable = .;
		. += 4096;

		__sys_linearmap_l1pgtable = .;
		. += 4096;

		__sys_linearmap_l2pgtable = .;
		. += 4096;
	} :data

	. = ALIGN(16384);

	.heap (NOLOAD) : AT(ADDR(.heap) - ENTRY_ADDR)
	{
		__heap_base_addr = .;

		. += HEAP_SIZE;
	} :data

	. = ALIGN(8192);

	/*
	 * Each CPU has its own stack area of STACK_SIZE bytes, reserved in a
	 * contiguous way from __stack_bottom: CPU#0 - CPU#1 - etc.
	 */

	.stack (NOLOAD) : AT(ADDR(.stack) - ENTRY_ADDR)
	{
		__stack_bottom = .;

		. += STACK_SIZE * CONFIG_NR_CPUS;

		__stack_top = .;
	} :data

	__end = .;

	/DISCARD/ :
	{
		*(.note.gnu.property)
		*(.comment)
		*(.eh_frame)
	}
}

PHDRS
{
	text PT_LOAD FLAGS(5); /* R-X */
	data PT_LOAD FLAGS(6); /* RW- */
}
