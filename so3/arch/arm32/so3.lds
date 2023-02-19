/*
 * Linker script for so3 kernel 
 */

OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
ENTRY(__start)

/* The sizes of the stacks used by the application. */
 
/*
 * SVC stack is used for kernel activities *and* for each newly created thread.
 * Indeed a user thread (running in the user space) will also involve kernel activities
 * with syscalls and other.
 */
SVC_STACK_SIZE = 32*1024 + 10*32*1024;  /* Initial system stack */

HEAP_SIZE = CONFIG_HEAP_SIZE * 1024 * 1024;    /* Heap dedicated to kernel heap */

TABLE_L1_SIZE = 16 * 1024;   /* 16 KB 1st-level page table */ 

/*
 * The system (1st-level) page table is located at 0xc000'4000 and occupies 16 KB (4096 32-bit entries)
 */  
 
SECTIONS
{
   
     . = ENTRY_ADDR + 0x8000;    
       
    __vectors_start = .;

    .vectors.text :
    {
        *(.vectors.text); 
    }
    
  	__vectors_end = .;
  
    . = ALIGN(16384);
    
    .head.text :
    {
        *(.head.text);
    }

    .text :
    {
        *(.text*) 
    }
    
	. = ALIGN(4096);
	
	.root_proc_prologue :
	{
		__root_proc_start = .;
		
		*(.root_proc.text)
		
		__root_proc_end = .;
		
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

    	__per_cpu_start = .;
    	
    	*(.bss.percpu)
    
     	. = ALIGN(128);
    	*(.bss.percpu.read_mostly)
    
     	. = ALIGN(128);
    	
    	__per_cpu_data_end = .;
       
     	. = ALIGN(128);
     	
     	__per_cpu_end = .;
     	
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
    
    /* Alignment is required for subsequent allocation of L1 page tables */
  
    . = ALIGN(16384);

    .heap :
    {
      __heap_base_addr = . ;
     
      . += HEAP_SIZE;
    }

    . = ALIGN(8192);

    .stack : 
    { 
        . += SVC_STACK_SIZE;
      
        __stack_top = . ;
        
    }

    .cpu1_stack : 
    { 
        . += SVC_STACK_SIZE;
      
        __cpu1_stack = . ;
        
    }
    
    .cpu2_stack : 
    { 
        . += SVC_STACK_SIZE;
      
        __cpu2_stack = . ;
        
    }
    
    .cpu3_stack : 
    { 
        . += SVC_STACK_SIZE;
      
        __cpu3_stack = . ;
        
    }
    
    __end = .;
}
