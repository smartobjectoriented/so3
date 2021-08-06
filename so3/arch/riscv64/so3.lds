/*
 * Linker script for so3 kernel
 */

OUTPUT_ARCH(riscv)
ENTRY(_vectors)

/* The sizes of the stacks used by the application. */

/*
 * SMode stack is used for kernel activities *and* for each newly created thread.
 * Indeed a user thread (running in the user space) will also involve kernel activities
 * with syscalls and other.
 */
SMODE_STACK_SIZE = 64*1024 + 32*64*1024;    /* Initial system stack + 640 KB stack available for max. 10 kernel threads */

HEAP_SIZE = 32 * 1024 * 1024;               /* 32 MB dedicated to kernel heap */

SECTIONS
{
     . = ENTRY_ADDR;

    __vectors_start = .;

    .vectors.text :
    {
        *(.vectors.text);
    }

  	__vectors_end = .;

    . = ALIGN(4096);

    .head.text :
    {
        *(.head.text);
    }

    .text :
    {
        *(.text*)
    }

    . = ALIGN(4096);
    .data  :
    {
        *(.data*)
    }

    . = ALIGN(4096);
    __bss_start = .;

    .sbss (NOLOAD) :
    {
        *(.sbss*)
    }

    .bss (NOLOAD) :
    {
        *(.bss*)
        . = ALIGN(4096);
    }

    __bss_end = .;

    . = ALIGN(4096);

    /* Initcall section for core drivers */

    __initcall_driver_initcall_t_core = .;

	.initcall_driver_initcall_t_core : {
		*(.initcall_driver_initcall_t_core);
	}

	__initcall_driver_initcall_t_core_end = .;

    /* Initcall section for post core drivers */

    __initcall_driver_initcall_t_postcore = .;

	.initcall_driver_initcall_t_postcore : {
		*(.initcall_driver_initcall_t_postcore);
	}

	__initcall_driver_initcall_t_postcore_end = .;

    /* Initcall section for pre IRQ init functions which cannot be done with IRQs on */

    __initcall_pre_irq_init_t_core = .;

	.initcall_pre_init_t_core : {
		*(.initcall_pre_irq_init_t_core);
	}

	__initcall_pre_irq_init_t_core_end = .;

	/* Initcall section for remaining init functions which can be performed with IRQs on */

    __initcall_postinit_t_core = .;

	.initcall_postinit_t_core : {
		*(.initcall_postinit_t_core);
	}

	__initcall_postinit_t_core_end = .;

    . = ALIGN(4096);

    .heap :
    {
      __heap_base_addr = . ;

      . += HEAP_SIZE;
    }

    . = ALIGN(4096);

    .stack :
    {
        . += SMODE_STACK_SIZE;

        __stack_top = . ;

    }

    __end = .;
}
