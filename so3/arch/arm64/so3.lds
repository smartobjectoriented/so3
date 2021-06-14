/*
 * Linker script for so3 kernel 
 */

OUTPUT_ARCH(aarch64)
ENTRY(__start)

/* The sizes of the stacks used by the application. */
 
/*
 * SVC stack is used for kernel activities *and* for each newly created thread.
 * Indeed a user thread (running in the user space) will also involve kernel activities
 * with syscalls and other.
 */
SVC_STACK_SIZE = 32 * 1024;  		/* Initial system stack */
HEAP_SIZE = 2 * 1024 * 1024;    /* 2 MB dedicated to kernel heap */
 
SECTIONS
{
     /* We start at the same entry point than Linux */
     /* to avoid issues on boards like Raspberry Pi 4 */
     /* where some firmware code is placed at 0x00000000 with a size of 0x1000 */
     
     . = ENTRY_ADDR + 0x80000;

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
 
    .bss : 
    {
       __bss_start = .;
        
        *(.bss) 
        
        . = ALIGN(4096);
        
    	__per_cpu_start = .;
    	
    	*(.bss.percpu)
    
     	. = ALIGN(128);
    	*(.bss.percpu.read_mostly)
    
     	. = ALIGN(128);
    	
    	__per_cpu_data_end = .;
       
     	. = ALIGN(128);
     	
     	__per_cpu_end = .;
   
    	__bss_end = .;  
    }
    
    /* Here is the L0 system page table */
    . = ALIGN(4096);
    
	__sys_l0pgtable = .;
	
	/* The size of this page table is 4 KB */
	
	. += 4096;
    
    /* Followed by the L1 system page table required for identity mapping */
    __sys_idmap_l1pgtable = .;
    
    . += 4096;
    
    /* Another L1 system page table required for the (first) linear mapping */
    __sys_linearmap_l1pgtable = .;
    
    . += 4096;
  
    .heap :
    {
      __heap_base_addr = . ;

      . += HEAP_SIZE;
    }
    
    . = ALIGN(4096);
    
    __end = .;
}
