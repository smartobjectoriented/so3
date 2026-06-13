/*
 * Linker script for so3 kernel 
 */

OUTPUT_ARCH(aarch64)
ENTRY(__start)

STACK_SIZE = (CONFIG_SYS_STACK_SIZE_KB + CONFIG_MAX_THREADS * CONFIG_THREAD_STACK_SIZE_KB) * 1024;
HEAP_SIZE = CONFIG_HEAP_SIZE_MB * 1024 * 1024;    /* Heap dedicated to kernel heap */

SECTIONS
{
     /* We start at the same entry point than Linux */
     /* to avoid issues on boards like Raspberry Pi 4 */
     /* where some locations at low addresses are reserved */
     /* for spin tables used for starting cores. */
     
     . = ENTRY_ADDR + LOAD_OFFSET;
    
    __vectors_start = .;

    .vectors.text :
    {
        *(.vectors.text); 
    } :text
    
  	__vectors_end = .;
  
    . = ALIGN(16384);
    
    .head.text :
    {
        *(.head.text);
    } :text

    .text :
    {
        *(.text*) 
    } :text

	. = ALIGN(4096);
	
	.root_proc_prologue :
	{
		__root_proc_start = .;
		
		*(.root_proc.text)
		
		__root_proc_end = .;
		
	} :text
	
    . = ALIGN(4096);
    .data  : 
    { 
        *(.data*) 
    } :data

    . = ALIGN(4096);
    __bss_start = .;

    .sbss (NOLOAD) : 
    { 
        *(.sbss*)
    }

    .bss (NOLOAD) :
    {
        *(.bss .bss.*)

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

    /* Here is the L0 system page table */
    . = ALIGN(4096);
    
	__sys_root_pgtable = .;
	
	/* The size of this page table is 4 KB */
	
	. += 4096;
    
    /* Followed by the L1 system page table required for identity mapping */
    __sys_idmap_l1pgtable = .;
    
    . += 4096;
    
    /* Another L1 system page table required for the (first) linear mapping */
    __sys_linearmap_l1pgtable = .;
    
    . += 4096;
    
    /* As well as a L2 page table to have a better granularity. */
    __sys_linearmap_l2pgtable = .;
    
    . += 4096;
     
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
    
    /* Alignment is required for subsequent allocation of L1 page tables */
  
    . = ALIGN(16384);
    
    .heap :
    {
      __heap_base_addr = . ;

      . += HEAP_SIZE;
    } :data
    
    . = ALIGN(8192);

    /* Each CPU has its own stack area of STACK_SIZE bytes. They are
     * reserved in a contiguous way from __stack_bottom in this order: CPU#0 - CPU#1 - etc.
     */

    .stack : 
    { 
        __stack_bottom = . ;

        . += STACK_SIZE * CONFIG_NR_CPUS;
        
        __stack_top = . ;
    } :data

    __end = .;
}

 PHDRS
{
  text PT_LOAD FLAGS(5); /* R-X */
  data PT_LOAD FLAGS(6); /* RW- */
}
