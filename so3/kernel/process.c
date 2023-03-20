/*
 * Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
 * Copyright (C) 2017 Xavier Ruppen <xavier.ruppen@heig-vd.ch>
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

#if 0
#define DEBUG
#endif

#include <types.h>
#include <common.h>
#include <thread.h>
#include <process.h>
#include <schedule.h>
#include <heap.h>
#include <elf.h>
#include <memory.h>
#include <vfs.h>
#include <wait.h>
#include <string.h>
#include <signal.h>
#include <ptrace.h>
#include <softirq.h>
#include <syscall.h>

#include <device/serial.h>

#include <asm/cacheflush.h>
#include <asm/mmu.h>
#include <asm/processor.h>
#include <asm/process.h>

static char *proc_state_strings[5] = {
	[PROC_STATE_NEW]	= "NEW",
	[PROC_STATE_READY]	= "READY",
	[PROC_STATE_RUNNING]	= "RUNNING",
	[PROC_STATE_WAITING]	= "WAITING",
	[PROC_STATE_ZOMBIE]	= "ZOMBIE",
};

char *proc_state_str(proc_state_t state) {
	return proc_state_strings[state];
}

/* We should maintain a bitmap of all pids in use in case we wrap around.
 * Just assume we won't ever spawn 2^32 processes for now. */

/* pid_current starts at 1 since pid 0 is used by fork() to test against the child process */
static uint32_t pid_current = 1;
static pcb_t *root_process = NULL; /* root process */

/* Used to update regs during fork */
extern void __save_context(tcb_t *newproc, addr_t stack_addr);

/* only the following sections are supported */
#define SUPPORTED_SECTION_COUNT 6
static const char *supported_section_names[SUPPORTED_SECTION_COUNT] = {
		".text",
		".rodata",
		".data",
		".sbss",
		".bss",
		".scommon",
};

/*
 * Find a process (pcb_t) from its pid.
 * Return NULL if no process has been found.
 */
pcb_t *find_proc_by_pid(uint32_t pid) {
	pcb_t *pcb;
	struct list_head *pos;

	list_for_each(pos, &proc_list)
	{
		pcb = list_entry(pos, pcb_t, list);
		if (pcb->pid == pid)
			return pcb;

	}

	/* Not found */
	return NULL;
}

/*
 * Look for a process which is in zombie state and requires to be cleaned.
 * This is used by waitpid() when pid arg = -1.
 * We care about keeping the existence of the PCB as long as the process still has children, that's
 * why we might have some process already cleaned up with no main_thread anymore.
 */
pcb_t *find_proc_zombie_to_clean(void) {
	pcb_t *pcb;
	struct list_head *pos;

	list_for_each(pos, &proc_list)
	{
		pcb = list_entry(pos, pcb_t, list);
		if ((pcb->state == PROC_STATE_ZOMBIE) && (pcb->main_thread != NULL))
			return pcb;

	}

	/* Not found */
	return NULL;
}

/*
 * Look for a process whose its parent is the one passed as argument to this function.
 */
pcb_t *find_proc_by_parent(pcb_t *parent) {
	pcb_t *pcb;
	struct list_head *pos;

	list_for_each(pos, &proc_list)
	{
		pcb = list_entry(pos, pcb_t, list);
		if (pcb->parent == parent)
			return pcb;
	}

	/* Not found */
	return NULL;
}

/* @brief This function will retrieve a unused fd.
 *		It will loop from the beginning of the local fd table
 *		to avoid fragmentation
 */
int proc_new_fd(pcb_t *pcb)
{
	unsigned i;

	for (i = 0; i < FD_MAX ; i++)
		if (pcb->fd_array[i] == -1)
			return i;

	return -1;
}

/*
 * Remove a process from the global list and free the PCB struct.
 */
void remove_proc(pcb_t *pcb) {
	struct list_head *pos, *p;
	pcb_t *cur;

	list_for_each_safe(pos, p, &proc_list)
	{
		cur = list_entry(pos, pcb_t, list);

		if (cur == pcb) {
			list_del(pos);

			free(cur);
			return ;
		}
	}
}

/*
 * Create a new process with its PCB and basic contents.
 */
pcb_t *new_process(void)
{
	unsigned int i;
	pcb_t *pcb;

	/* PCB allocation */
	pcb = malloc(sizeof(pcb_t));

	if (!pcb) {
		printk("%s: failed to allocate memory\n", __func__);
		kernel_panic();
	}
	memset(pcb, 0, sizeof(pcb_t));

	pcb->state = PROC_STATE_NEW;

	/* Reset the ptrace request indicator */
	pcb->ptrace_pending_req = PTRACE_NO_REQUEST;

	/* Initialize the mutex belonging to this process */
	for (i = 0; i < N_MUTEX; i++)
		mutex_init(&pcb->lock[i]);

	/* Init the list of pages */
	INIT_LIST_HEAD(&pcb->page_list);

	pcb->pid = pid_current++;

	for (i = 0; i < PROC_THREAD_MAX; i++)
		pcb->stack_slotID[i] = false;

	/* Init the list of child threads */
	INIT_LIST_HEAD(&pcb->threads);

	/* Process-related memory management */

	/* Create the 1st level page table */
	pcb->pgtable = new_root_pgtable();
	if (!pcb->pgtable) {
		printk("%s: failed to create level 1 page table", __func__);
		kernel_panic();
	}

	/* With AArch32, we have one page table used by TTBR0/1 without distinction. */
#ifdef CONFIG_ARCH_ARM32
	/* Preserve the mapping of kernel regions according to the arch configuration. */
	pgtable_copy_kernel_area(pcb->pgtable);
#endif
	/* Integrate the list of process */
	list_add_tail(&pcb->list, &proc_list);

	/* Initialize the completion used for managing running threads (helpful for pthread_exit()) */
	init_completion(&pcb->threads_active);

	return pcb;
}

/*
 * Initialize the (user space) process stack
 */
void reset_process_stack(pcb_t *pcb) {

	/* Set up the main process stack (including all thread stacks) */
	pcb->page_count = ALIGN_UP(PROC_STACK_SIZE, PAGE_SIZE) >> PAGE_SHIFT;

	/*
	 * The stack virtual top is under the page of arguments, from the top user space.
	 * The stack is full descending.
	 */
	pcb->stack_top = arch_get_args_base();
}

void dump_proc_pages(pcb_t *pcb){
	page_list_t *cur;

	printk("----- Dump of pages belonging to proc: %d -----\n\n", pcb->pid);
	list_for_each_entry(cur, &pcb->page_list, list)
		printk("   -- page: %p  pfn: %x   refcount: %d\n", cur->page, page_to_pfn(cur->page), cur->page->refcount);
	printk("\n");
}

void add_page_to_proc(pcb_t *pcb, page_t *page) {
	page_list_t *page_list_entry;

	page_list_entry = malloc(sizeof(page_list_t));
	if (page_list_entry == NULL) {
		printk("%s: failed to allocate memory!\n", __func__);
		kernel_panic();
	}

	page_list_entry->page = page;

	page->refcount++;

	/* Insert our page at the end of the list */
	list_add_tail(&page_list_entry->list, &pcb->page_list);

}

/*
 * Find available frames and do the mapping of a number of pages.
 */
static void allocate_page(pcb_t *pcb, addr_t virt_addr, int nr_pages, bool usr) {
	int i;
	addr_t page;

	/* Perform the mapping of a new physical memory region at the region started at @virt_addr  */
	for (i = 0; i < nr_pages; i++) {
		page = get_free_page();
		BUG_ON(!page);

		create_mapping(pcb->pgtable, virt_addr + (i * PAGE_SIZE), page, PAGE_SIZE, false);

		add_page_to_proc(pcb, phys_to_page(page));
	}
}

/**
 *
 * Create a process from scratch, without fork'd. Typically used by the kernel main
 * at the end of the bootstrap.
 *
 *
 * @param start_routine	Address of the main thread which as to be located in
 * 			the user space area
 * @param name		Name of the process
 */
void create_root_process(void)
{
	pcb_t *pcb;
	int i;

	local_irq_disable();

	pcb = new_process();

	reset_process_stack(pcb);

	/* We map the initial user space process stack here, and fork() will inherit from this mapping */

	allocate_page(pcb, pcb->stack_top - (pcb->page_count * PAGE_SIZE), pcb->page_count, true);

	DBG("Stack mapped at 0x%08x (size: %d bytes)\n", pcb->stack_top - (pcb->page_count * PAGE_SIZE), PROC_STACK_SIZE);

	/* First map the code in the user space so that
	 * the initial code can run normally in user mode.
	 */
	create_mapping(pcb->pgtable, USER_SPACE_VADDR, __pa(__root_proc_start), (void *) __root_proc_end - (void *) __root_proc_start, false);

	/* Start main thread <args> of the thread is not used in this context. */
	pcb->main_thread = user_thread((th_fn_t) USER_SPACE_VADDR, "root_proc", NULL, pcb);

	/* init process? */
	if (!root_process)
		root_process = pcb;

	/* Init file descriptors to -1 */
	for (i = 0; i < FD_MAX; i++)
		pcb->fd_array[i] = -1;

	/* Set the file descriptors */
	pcb->fd_array[STDOUT] = STDOUT;
	pcb->fd_array[STDIN] = STDIN;
	pcb->fd_array[STDERR] = STDERR;

	/* At the end, the so3-boot thread which is running this function can disappear ... */
	pcb->state = PROC_STATE_READY;

	thread_exit(0);

	/* Never reached */
	BUG();
}


/*
 * Release all pages allocated to a process
 */
static void release_proc_pages(pcb_t *pcb) {
	struct list_head *pos, *q;
	page_list_t *cur;

	list_for_each_safe(pos, q, &pcb->page_list)
	{
		cur = list_entry(pos, page_list_t, list);

		list_del(pos);

		cur->page->refcount--;
		if (!cur->page->refcount)
			free_page(page_to_phys(cur->page));

		free(cur);
	}
}

/*
 * Set up the arguments and environment variables for the current process (current()->pcb)
 */
void *preserve_args_and_env(int argc, char **argv, char **envp)
{
	char *args_p, *args_str_p;
	void *args;
	char **__args;
	int i;

	if ((argc > 0) && (argv == NULL)) {
		set_errno(EINVAL);
		return NULL;
	}

	/* Page storing the args & env strings - <args> keeps a reference to it. */
	args = malloc(PAGE_SIZE);
	BUG_ON(args == NULL);

	memset(args, 0, PAGE_SIZE);

	args_p = (char *) args;
	if (!argc)
		i = 1;
	else
		i = argc;

	memcpy(args_p, &i, sizeof(int));

	/* Number of args */
	args_p += sizeof(int);

	/* Store the array of strings for args & env */

	/* The array starts right after argc (stored on 4 bytes).
	 * We find the addresses of the var strings followed by the addresses of env strings.
	 * The var strings come after followed by the env strings.
	 */

	/* Manage the addresses of strings; determine the start of the first arg string. */

	if (!argc) /* At least one arg containing the process name */
		args_p += sizeof(char *);
	else
		args_p += sizeof(char *) * argc;

	/* Environment string addresses */
	if (!envp) {
		*((addr_t *) args_p) = 0; /* Keep array-end with NULL */
		args_p += sizeof(char *);

	} else {
		i = 0;
		do {
			args_p = args_p + sizeof(char *);
		} while (envp[i++] != NULL);
	}


	/* Manage the arg strings */

	args_str_p = args_p;
	__args = (char **) (args + sizeof(int));

	/* As said before, if argc is 0 (argv NULL), we put the process name (a kind of by-default argument) */
	if (!argc) {
		__args[0] = args_str_p;
		strcpy(__args[0], current()->pcb->name);

		args_str_p += strlen(current()->pcb->name) + 1;

	}

	/* Place the strings with their addresses - <argv> is the new address (definitive location). */

	for (i = 0; i < argc; i++) {
		__args[i] = args_str_p;
		strcpy(__args[i], argv[i]);

		args_str_p += strlen(argv[i]) + 1;

		/* We check if the pointer do not exceed the page we
		 * allocated before */
		if (((addr_t) args_str_p - (addr_t) args) > PAGE_SIZE) {
			DBG("Not enougth memory allocated\n");
			set_errno(ENOMEM);

			free(args);
			return NULL;
		}
	}

	/* Environment strings */

	/* First env. variable */
	__args = (char **) (args + sizeof(int) + sizeof(char *) * (*((int *) args)));

	/* If the environment was passed */
	if (envp) {
		i = 0;
		while (envp[i] != NULL) {
			__args[i] = args_str_p;
			strcpy(__args[i], envp[i]);

			args_str_p += strlen(envp[i]) + 1;

			/* We check if pointer do not exceed the page we
			 * allocated before. */
			if (((addr_t) args_str_p - (addr_t) args) > PAGE_SIZE) {
				DBG("Not enougth memory allocated\n");
				set_errno(ENOMEM);

				free(args);
				return NULL;
			}
			i++;
		}
	}

	return args;
}

void post_setup_image(void *args_env) {
	char **__args;
	char *args_base;
	int argc, i;

	args_base = (char *) arch_get_args_base();

	memcpy(args_base, args_env, PAGE_SIZE);

	free(args_env);

	/* Now, readjust the address of the var and env strings */

	argc = *((int *) args_base);
	__args = (char **) (args_base + sizeof(int));

	/* We get the offset based on the current location and the start of the args_env page, then
	 * we adjust it to match our final destination.
	 */
	for (i = 0; i < argc; i++)
		__args[i] = ((addr_t)__args[i] - (addr_t) args_env) + args_base;


	/* Focus on the env addresses now */
	__args = (char **) (args_base + sizeof(int) + argc*sizeof(char *));

	for (i = 0; __args[i] != NULL; i++)
		__args[i] = ((addr_t) __args[i] - (addr_t) args_env) + args_base;

}

/*
 * Set up the PCB fields related to the binary image to be loaded.
 */
int setup_proc_image_replace(elf_img_info_t *elf_img_info, pcb_t *pcb, int argc, char **argv, char **envp)
{
	uint32_t page_count;
	void *__args_env;

	/* FIXME: detect fragmented executable (error)? */
	/*
	 * Preserve the arguments issued from this configuration so that we
	 * can place them within their definitive place in the target image.
	 * Be careful to adjust the address of the string within the array; they need to
	 * refer the string within the dedicated arg/env page and no more at the original location.
	 */

	__args_env = preserve_args_and_env(argc, argv, envp);
	if (__args_env < 0)
		return -1;

	/* Reset the process stack and page count */
	reset_process_stack(pcb);

	/* Release allocated pages in case of exec() within a fork'd process */
	/* The current binary image (which is a copy of the fork'd) must disappeared.
	 * Associated physical pages must be removed and freed. Stack area will be re-initialized.
	 *
	 * We reset the contents but we keep the root page table for subsequent allocations.
	 */
	reset_root_pgtable(pcb->pgtable, false);

	/* Release all allocated pages for user space. */
	release_proc_pages(pcb);

	/* We re-init the user space process stack here, and fork() will inherit from this mapping */

	allocate_page(pcb, pcb->stack_top - (pcb->page_count * PAGE_SIZE), pcb->page_count, true);

	DBG("stack mapped at 0x%08x (size: %d bytes)\n", pcb->stack_top - (pcb->page_count * PAGE_SIZE), PROC_STACK_SIZE);

	/* Initialize the pc register */
	pcb->bin_image_entry = (uint32_t) elf_img_info->header->e_entry;

	/* The first virtual page will not be mapped since it is the zero-page which
	 * is used to detect NULL pointer access. It has to raise a data abort exception.
	 * That's the reason why the linker script of a user application must start at 0x1000 (at the lowest).
	 */

	pcb->page_count += elf_img_info->segment_page_count;

	/* Map the elementary sections (text, data, bss) */
	allocate_page(pcb, (uint32_t) elf_img_info->header->e_entry, elf_img_info->segment_page_count, true);

	DBG("entry point: 0x%08x\n", elf_img_info->header->e_entry);
	DBG("page count: 0x%08x\n", pcb->page_count);

	/* Maximum heap size */
	page_count = ALIGN_UP(HEAP_SIZE, PAGE_SIZE) >> PAGE_SHIFT;
	pcb->heap_base = (pcb->page_count + 1) * PAGE_SIZE;
	pcb->heap_pointer = pcb->heap_base;
	pcb->page_count += page_count;

	allocate_page(pcb, pcb->heap_base, page_count, true);

	DBG("heap mapped at 0x%08x (size: %d bytes)\n", pcb->heap_base, HEAP_SIZE);

	/* arguments (& env) will be stored in one more page */
	pcb->page_count++;

	allocate_page(pcb, arch_get_args_base(), 1, true);
	DBG("arguments mapped at 0x%08x (size: %d bytes)\n", arch_get_args_base(),  PAGE_SIZE);

	/* Prepare the arguments within the page reserved for this purpose. */
	if (__args_env)
		post_setup_image(__args_env);

	return 0;
}

/* load sections from each loadable segment into the process' virtual pages */
void load_process(elf_img_info_t *elf_img_info)
{
	unsigned long section_start, section_end;
	unsigned long segment_start, segment_end;
	int i, j, k;
	bool section_supported;

	/* Loading the different segments */
	for (i = 0; i < elf_img_info->header->e_phnum; i++)
	{
		if (elf_img_info->segments[i].p_type != PT_LOAD)
			/* Skip unloadable segments */
			continue;

		segment_start = elf_img_info->segments[i].p_offset;
		segment_end = segment_start + elf_img_info->segments[i].p_memsz;

		/* Sections */
		for (j = 0; j < elf_img_info->header->e_shnum; j++) {
			section_start = elf_img_info->sections[j].sh_offset;
			section_end = section_start + elf_img_info->sections[j].sh_size;

			/* Verify if the section is part of this segment */
			if ((section_start < segment_start) || (section_end > segment_end))
				continue;

			/* Not all sections are supported */
			section_supported = false;
			for (k = 0; k < SUPPORTED_SECTION_COUNT; k++) {
				if (!strcmp(elf_img_info->section_names[j], supported_section_names[k])) {
					section_supported = true;
					break;
				}
			}

			if (!section_supported)
				continue;

			/* Load this section into the process' virtual memory */
			if (elf_img_info->sections[j].sh_type == SHT_NOBITS)
				/* unless it were not stored in the file (.bss) */
				continue;

			/* Real load of contents in the user space memory of the process */
			memcpy((void *) elf_img_info->sections[j].sh_addr, (void *) (elf_img_info->file_buffer + elf_img_info->sections[j].sh_offset), elf_img_info->sections[j].sh_size);
		}
	}

	/* Make sure all data are flushed for future context switch. */
	flush_dcache_all();
}

int do_execve(const char *filename, char **argv, char **envp)
{
	elf_img_info_t elf_img_info;
	pcb_t *pcb;
	unsigned long flags;
	th_fn_t start_routine;
	queue_thread_t *cur;
	int ret, argc;

	/* Count the number of arguments */
	argc = 0;
	if (argv != NULL)
		while (argv[argc] != NULL)
			argc++;

	/* We do not support exec() from another thread than the main one */
	ASSERT(current() == current()->pcb->main_thread);

	flags = local_irq_save();

	/* Get the running process */
	pcb = current()->pcb;

	/* Keep the filename as process name */
	strcpy(pcb->name, filename);

	/* ELF parsing */
	elf_img_info.file_buffer = elf_load_buffer(filename);
	if (elf_img_info.file_buffer == NULL) {
		local_irq_restore(flags);
		set_errno(ENOENT);

		return -1;
	}

	elf_load_sections(&elf_img_info);
	elf_load_segments(&elf_img_info);

	/*
	 * If it is not the root process and the initial thread,
	 * replace the binary image of the process. - Prepare the page frames and mapped then in the virtual address space.
	 */
	ret = setup_proc_image_replace(&elf_img_info, pcb, argc, argv, envp);
	if (ret < 0)
		return ret;

	/* process execution */
#warning do_exec() must be completed...
	/* TODO: close all fds except stdin stdout stderr -> including pipes (so keep all, end of process will terminate fds */

	/* Load the contents of ELF into the virtual memory space */
	load_process(&elf_img_info);

	/* Release the kernel buffer used to store the ELF binary image */
	elf_clean_image(&elf_img_info);

	/* Now, we need to create the main user thread associated to this binary image. */
	/* start main thread */
	start_routine = (th_fn_t) pcb->bin_image_entry;

	/* We start the new thread */
	pcb->main_thread = user_thread(start_routine, pcb->name, (void *) arch_get_args_base(), pcb);

	/* Transfer the waiting thread if any */

	/* Make sure it is the main thread of the dying thread, i.e. another process is waiting on it (the parent) */
	if (!list_empty(&current()->joinQueue)) {
		cur = list_entry(current()->joinQueue.next, queue_thread_t, list);

		ASSERT(cur->tcb->pcb == current()->pcb->parent);

		/* Migrate the entry to the new main thread */
		list_move(&cur->list, &pcb->main_thread->joinQueue);

	}

	/* Now, make sure there is no other waiting threads on the dying thread */
	ASSERT(list_empty(&current()->joinQueue));

	/* We detach the thread from its pcb so that thread_exit() can distinguish between this kind
	 * of (replaced) thread and a standard thread which will be stay in zombie state.
	 */
	current()->pcb = NULL;

	/* Finishing the running thread. The final clean_thread() function will be called in thread_exit(). */
	thread_exit(0);

	/* IRQs never restored here... */

	return 0;
}

/* Copy all relevant fields of PCB */
pcb_t *duplicate_process(pcb_t *parent)
{
	pcb_t *pcb;

	/* Initialize a new PCB */
	pcb = new_process();

	/* Update the page count based on the parent */
	pcb->page_count = parent->page_count;

	/* Affiliate the new process to the parent */
	pcb->parent = parent;

	/* Set up the same stack origin */
	pcb->stack_top = parent->stack_top;

	/* Update the heap pointer from the parent */
	pcb->heap_base = parent->heap_base;
	pcb->heap_pointer = parent->heap_pointer;

	/* Duplicate the array of allocated stack slots dedicated to user threads */
	memcpy(pcb->stack_slotID, parent->stack_slotID, sizeof(parent->stack_slotID));

	/* Clone all file descriptors */
	if (vfs_clone_fd(parent->fd_array, pcb->fd_array)) {
		printk("!! Error while cloning fds\n");
		kernel_panic();
	}

	return pcb;
}

/*
 * For a new process from the current running process.
 */
int do_fork(void)
{
	pcb_t *newp, *parent;
	unsigned long flags;

	flags = local_irq_save();

	parent = current()->pcb;

	/* For the time being, we *only* authorize to fork() from the main thread */
	if (current() != parent->main_thread) {
		printk("%s: forking from a thread other than the main thread is not allowed so far ...\n", __func__);
		return -1;
	}

	/* Duplicate the elements of the parent process into the child */
	newp = duplicate_process(parent);

	/* Copy the user space area of the parent process */
	duplicate_user_space(parent, newp);

	/* At the moment, we spawn the main_thread only in the child. In the future, we will have to create a thread for each existing threads
	 * in the parent process.
	 */
	sprintf(newp->name, "%s_child_%d", parent->name, newp->pid);

	newp->main_thread = user_thread(NULL, newp->name, (void *) arch_get_args_base(), newp);

	/* Copy the kernel stack of the main thread */
	memcpy((void *) get_kernel_stack_top(newp->main_thread->stack_slotID) - THREAD_STACK_SIZE,
	       (void *) get_kernel_stack_top(parent->main_thread->stack_slotID) - THREAD_STACK_SIZE, THREAD_STACK_SIZE);

	/*
	 * Preserve the current value of all registers concerned by this execution so that
	 * the new thread will be able to pursue its execution once scheduled.
	 */

	__save_context(newp->main_thread, get_kernel_stack_top(newp->main_thread->stack_slotID));

	/* The main process thread is ready to be scheduled for its execution.*/
	newp->state = PROC_STATE_READY;
	
	BUG_ON(!local_irq_is_disabled());

	/* Prepare to perform scheduling to check if a context switch is required. */
	raise_softirq(SCHEDULE_SOFTIRQ);

	local_irq_restore(flags);

	/* Return the PID of the child process. The child will do not execute this code, since
	 * it jumps to the ret_from_fork in context.S
	 */

	return newp->pid;
}

/*
 * Terminates a process.
 * All allocated resources should be released except its PCB which still contains the exit code.
 */
void do_exit(int exit_status) {
	pcb_t *pcb;
	unsigned i;

	pcb = current()->pcb;

	/* Never finish the root process */
	if (pcb->parent == NULL) {
		printk("<kernel> %s: cannot finish the root process ...\n", __func__);
		kernel_panic();
	}

	/* Close the file descriptors - IRQs must remain on since we need locking in the low layers. */

	for (i = 0; i < FD_MAX; i++)
		do_close(i);

	local_irq_disable();

	/* Now, set the process state to zombie, before definitively die... */
	pcb->state = PROC_STATE_ZOMBIE;

	/* Set the exit code */
	pcb->exit_status = exit_status;

	/* Finish properly the serial related components */
	serial_cleanup();

	/* Release all allocated pages for user space */
	release_proc_pages(pcb);

#ifdef CONFIG_IPC_SIGNAL

	/* Send the SIGCHLD signal to the parent */
	do_kill(pcb->parent->pid, SIGCHLD);

#endif /* CONFIG_IPC_SIGNAL */

	/*
	 * We are finished ... Properly terminate the process main thread, which will lead to
	 * the wake up of the parent.
	 */

	thread_exit(NULL);
}

/*
 * Returns the PID of the current process
 */
uint32_t do_getpid(void) {
	return current()->pcb->pid;
}

/*
 * Waitpid implementation - do_waitpid() does the following operations:
 * - Suspend the current process until the child process finished its execution (exit())
 * If the pid argument is -1, waitpid() looks for a possible terminated process and performs the operation.
 * If no process is finished (in zombie state), waitpid() will return 0 (if pid argument is -1).
 * - Get the exit code from the child PCB
 * - Clean the PCB and page tables
 * - Return the pid if successful operation
 */
int do_waitpid(int pid, uint32_t *wstatus, uint32_t options) {
	pcb_t *child;
	unsigned long flags;

	flags = local_irq_save();

	if (pid == -1) {
		child = find_proc_zombie_to_clean();
		if (!child) {
			set_errno(ECHILD);
			local_irq_restore(flags);

			return -1;
		}
		pid = child->pid;
	} else  {
		/* Get the child process identified by its pid. */
		child = find_proc_by_pid(pid);
		if (!child) {
			if (wstatus != NULL)
				*wstatus = ~0x7f; /* !WTERMSIG -> WIFEXITED true */
			set_errno(ECHILD);
			return -1;
		}
	}
	/* In the case the of NOHANG if the child did not change state,
	 * the waitpid return 0;
	 */
	if (options & WNOHANG)
		if (child->state != PROC_STATE_ZOMBIE)
			return 0;

	/* Must the child be resumed after being stopped due to a ptrace request ? */
	if ((child->ptrace_pending_req != PTRACE_NO_REQUEST) && (child->ptrace_pending_req != PTRACE_TRACEME)) {

		/* Resume the child process being stopped previously. */

		child->state = PROC_STATE_READY;
		ready(child->main_thread);
	}

	if (child->state != PROC_STATE_WAITING)
		/* Wait on the main_thread of this process */
		thread_join(child->main_thread);

	/* Before joining, we need to check the state of child because it could have been finished before this call. */
	if ((child->state == PROC_STATE_ZOMBIE) && (child->ptrace_pending_req == PTRACE_NO_REQUEST)) {

		/* Free the page tables used for this process */
		reset_root_pgtable(child->pgtable, true);

		/* Get the exit code left in the PCB by the child */
		if (wstatus) {
			*wstatus = ~0x7f; /* !WTERMSIG -> WIFEXITED true */
			*wstatus = ((char) child->exit_status) << 8;
		}

		/*
		 * SO3 approach consists in avoiding having orphan process.
		 * The process will be removed from the system definitively only if it has no children.
		 */
		if (!find_proc_by_parent(child))
			remove_proc(child);

	} else {

		if (child->ptrace_pending_req != PTRACE_NO_REQUEST) {
			/* In this case, the child has been stopped in the context of the ptrace syscall */

			/* Reset the ptrace request */
			child->ptrace_pending_req = PTRACE_NO_REQUEST;

			if (wstatus) {
				*wstatus = 0x17f; /* WIFSTOPPED true */
				*wstatus |= ((char) child->exit_status) << 8;
			}
		} else {

			/* Free the page tables used for this process */
			reset_root_pgtable(child->pgtable, true);

			/* Get the exit code left in the PCB by the child */
			if (wstatus) {
				*wstatus = ~0x7f; /* !WTERMSIG -> WIFEXITED true */
				*wstatus |= ((char) child->exit_status) << 8;
			}

			/* Finally remove the process from the system definitively as long as there is no children from there */
			if (!find_proc_by_parent(child))
				remove_proc(child);
		}
	}

	local_irq_restore(flags);

	return pid;
}


/* @brief Currently this function does not allocate pages.
 *		page are already allocated. this function will only
 *		increase the heap_pointer and make sure it does not 
 *		overflow the heap. If there is no memory left it will
 *		set the errno to ENONMEM and return -1;
 * @param increment the amount of data to increase < decrease. If the value is 0
 *		function will return the current position of the program break (end of heap);
 *
 * @return  This function will the position of the end of the heap / program break before increment
 */
int do_sbrk(int increment)
{
	pcb_t *pcb = current()->pcb;
	int ret_pointer;
	int req_sz = 0;
	int cur_sz;

	if (!pcb) {
		/* case there is no pcb context */
		set_errno(ESRCH);
		return -1;
	}

	ret_pointer = pcb->heap_pointer;

	/* we make sure the future size of the heap is not overflowing / underflowing*/
	cur_sz = pcb->heap_pointer - pcb->heap_base;
	req_sz = cur_sz + increment;

	if ((req_sz >= HEAP_SIZE) || (req_sz < 0)) {
		set_errno(ENOMEM);
		return -1;
	}

#if 0
	/* This is the the code allocation will be done automatically*/
	/* Case of the first page */
	if (!cur_sz) {
		if (req_sz) {
			pcb->page_count += 1;
			allocate_page(pcb, pcb->heap_base, page_count);
			pcb->heap_pointer = pcb->heap_base + PAGE_SIZE;
		} 
		return -1;
	}

	int i;
	int page_start;
	req_nbr_pages = (int) (req_sz / PAGE_SIZE) - (int) (cur_sz / PAGE_SIZE);
	/* Test if we need to allocate/free one more page on heap*/
	pcb->page_count += req_nbr_pages;

	if (req_nbr_pages < 0) {
		page_start = pcb->heap_pointer / PAGE_SIZE + req_nbr_pages;
		for (i = 0; i > req_nbr_pages; i++) {
			release_page((page_start + i) * PAGE_SIZE);
		}
	} else if (req_nbr_pages > 0) {
		page_start = pcb->heap_pointer / PAGE_SIZE + 1;
		allocate_page(pcb, () (page_start * PAGE_SIZE), req_nbr_pages);
	}
#endif

	pcb->heap_pointer = pcb->heap_pointer + increment;
	return ret_pointer;
}


#ifdef CONFIG_PRIORITY_SCHEDULER
void do_ps()
{
	pcb_t *pcb = NULL;
	tcb_t *tcb = NULL;
	struct list_head *proc_pos, *thread_pos;

	printk("\n****************************************************\n");
	if (list_empty(&proc_list)) {
		printk(" process list is <empty>\n");
	        printk("\n****************************************************\n\n");
		return ;
	}

	list_for_each(proc_pos, &proc_list)
	{
                /* find and print main thread */
		pcb = list_entry(proc_pos, pcb_t, list);
                printk(" [pid %02d] [main_tid %02d : priority %03d : %s]", pcb->pid, pcb->main_thread->tid, pcb->main_thread->priority, pcb->main_thread->name);

                /* find and print other threads */
                if (list_empty(&pcb->threads)) {
	                printk("\n****************************************************\n\n");
                	return ;
                }
                list_for_each(thread_pos, &pcb->threads)
                {
                        tcb = list_entry(thread_pos, tcb_t, list);
                        printk(" [other_tid %02d : priority %03d : %s]\n", tcb->tid, tcb->priority, tcb->name);
                }

	}
	printk("\n****************************************************\n\n");
}
#endif

/* @brief This function looks for a valid fd.
 */
int proc_register_fd(int gfd)
{
	int fd;
	pcb_t *pcb = current()->pcb;

	if (!pcb)
		return -1;

	fd = proc_new_fd(pcb);

	if (fd < 0) {
		set_errno(ENFILE);
		DBG("Number of local fd reached\n");
		return fd;
	}

	pcb->fd_array[fd] = gfd;

	return fd;
}

void dump_proc(void) {
	pcb_t *pcb = NULL;

	printk("********* List of processes **********\n\n");

	list_for_each_entry(pcb, &proc_list, list)
	{
		/* Based on process main thread. */

		printk(" [pid %d state: %s] [main_tid: %d name: %s]\n", pcb->pid, proc_state_str(pcb->state),
				((pcb->main_thread != NULL) ? pcb->main_thread->tid : -1), pcb->main_thread->name);
	}
}
