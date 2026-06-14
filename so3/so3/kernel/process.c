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

#include <common.h>
#include <elf.h>
#include <heap.h>
#include <memory.h>
#include <process.h>
#include <schedule.h>
#include <signal.h>
#include <softirq.h>
#include <string.h>
#include <syscall.h>
#include <thread.h>
#include <types.h>
#include <vfs.h>
#include <wait.h>
#include <log.h>
#include <timer.h>

#include <uapi/linux/auxvec.h>

#include <device/serial.h>

#include <asm/cacheflush.h>
#include <asm/mmu.h>
#include <asm/process.h>
#include <asm/processor.h>
#include <asm/hwcap.h>
#ifndef CONFIG_ARCH_ARM32
#include <asm/semihosting.h>
#endif

/* Structure to temporary save exec args/env */
typedef struct {
	int argc;
	int envc;
	size_t strings_size;
	char arg_env[PAGE_SIZE];
} args_env_t;

static char *proc_state_strings[5] = {
	[PROC_STATE_NEW] = "NEW",	  [PROC_STATE_READY] = "READY",	  [PROC_STATE_RUNNING] = "RUNNING",
	[PROC_STATE_WAITING] = "WAITING", [PROC_STATE_ZOMBIE] = "ZOMBIE",
};

char *proc_state_str(proc_state_t state)
{
	return proc_state_strings[state];
}

/* We should maintain a bitmap of all pids in use in case we wrap around.
 * Just assume we won't ever spawn 2^32 processes for now. */

/* pid_current starts at 1 since pid 0 is used by fork() to test against the
 * child process */
static uint32_t pid_current = 1;
static pcb_t *root_process = NULL; /* root process */

/*
 * Foreground console process: the process that should receive console-driven
 * signals (e.g. SIGINT on Ctrl-C). It is updated to the child a process is
 * blocked waiting on (see sys_do_wait4) — i.e. the shell's foreground job —
 * and restored to the waiter when that child exits. Used by the serial IRQ
 * (devices/serial/pl011.c) instead of current(), which at IRQ time is merely
 * whatever thread happened to be running (usually the idle thread when the
 * foreground app is asleep). NULL until the first foreground wait.
 */
pcb_t *fg_pcb = NULL;

/*
 * Find a process (pcb_t) from its pid.
 * Return NULL if no process has been found.
 */
pcb_t *find_proc_by_pid(uint32_t pid)
{
	pcb_t *pcb;
	struct list_head *pos;

	list_for_each(pos, &proc_list) {
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
 * We care about keeping the existence of the PCB as long as the process still
 * has children, that's why we might have some process already cleaned up with
 * no main_thread anymore.
 */
pcb_t *find_proc_zombie_to_clean(void)
{
	pcb_t *pcb;
	struct list_head *pos;

	list_for_each(pos, &proc_list) {
		pcb = list_entry(pos, pcb_t, list);
		if ((pcb->state == PROC_STATE_ZOMBIE) && (pcb->main_thread != NULL))
			return pcb;
	}

	/* Not found */
	return NULL;
}

/*
 * Look for a process whose its parent is the one passed as argument to this
 * function.
 */
pcb_t *find_proc_by_parent(pcb_t *parent)
{
	pcb_t *pcb;
	struct list_head *pos;

	list_for_each(pos, &proc_list) {
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

	for (i = 0; i < FD_MAX; i++)
		if (pcb->fd_array[i] == -1)
			return i;

	return -1;
}

/*
 * Remove a process from the global list and free the PCB struct.
 */
void remove_proc(pcb_t *pcb)
{
	struct list_head *pos, *p;
	pcb_t *cur;

	list_for_each_safe(pos, p, &proc_list) {
		cur = list_entry(pos, pcb_t, list);

		if (cur == pcb) {
			list_del(pos);

			free(cur);
			return;
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
		LOG_CRITICAL("%s: failed to allocate memory\n", __func__);
		kernel_panic();
	}
	memset(pcb, 0, sizeof(pcb_t));

	pcb->state = PROC_STATE_NEW;

	pcb->next_anon_start = USER_ANONYMOUS_VADDR;

	/* The spinlock inside the mutex must aligned in aarch64 */
	pcb->lock = memalign(N_MUTEX * sizeof(mutex_t), 8);

	if (!pcb->lock) {
		LOG_CRITICAL("%s: failed to allocate memory for process mutex\n", __func__);
		kernel_panic();
	}

	/* Initialize the mutex belonging to this process */
	for (i = 0; i < N_MUTEX; i++) {
		mutex_init(&pcb->lock[i]);
	}
	/* Init the list of pages */
	INIT_LIST_HEAD(&pcb->page_list);

	/* Init the futex */
	INIT_LIST_HEAD(&pcb->futex);
	spin_lock_init(&pcb->futex_lock);

	pcb->pid = pid_current++;

	/* Init the list of child threads */
	INIT_LIST_HEAD(&pcb->threads);

	/* Process-related memory management */

	/* Create the 1st level page table */
	pcb->pgtable = new_root_pgtable();
	if (!pcb->pgtable) {
		LOG_CRITICAL("%s: failed to create level 1 page table", __func__);
		kernel_panic();
	}

	/* With AArch32, we have one page table used by TTBR0/1 without
         * distinction. */
#ifdef CONFIG_ARCH_ARM32
	/* Preserve the mapping of kernel regions according to the arch
         * configuration. */
	pgtable_copy_kernel_area(pcb->pgtable);
#endif
	/* Integrate the list of process */
	list_add_tail(&pcb->list, &proc_list);

	/* Initialize the completion used for managing running threads (helpful
         * for pthread_exit()) */
	init_completion(&pcb->threads_active);

	return pcb;
}

/*
 * Initialize the (user space) process stack
 */
void reset_process_stack(pcb_t *pcb)
{
	/* Set up the main process stack (including all thread stacks) */
	pcb->page_count = ALIGN_UP(INITIAL_STACK_SIZE, PAGE_SIZE) >> PAGE_SHIFT;

	/*
         * The stack virtual top is under the page of arguments, from the top
         * user space. The stack is full descending.
         */
	pcb->stack_top = arch_get_args_base();
}

void dump_proc_pages(pcb_t *pcb)
{
	page_list_t *cur;

	LOG_INFO("----- Dump of pages belonging to proc: %d -----\n\n", pcb->pid);
	list_for_each_entry(cur, &pcb->page_list, list)
		LOG_INFO("   -- page: %p  pfn: %lx   refcount: %d\n", cur->page, page_to_pfn(cur->page), cur->page->refcount);
	LOG_INFO("\n");
}

void add_page_to_proc(pcb_t *pcb, page_t *page)
{
	page_list_t *page_list_entry;

	page_list_entry = malloc(sizeof(page_list_t));
	if (page_list_entry == NULL) {
		LOG_CRITICAL("%s: failed to allocate memory!\n", __func__);
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
static void allocate_page(pcb_t *pcb, addr_t virt_addr, int nr_pages, bool usr)
{
	int i;
	addr_t page;

	/* Perform the mapping of a new physical memory region at the region
         * started at @virt_addr  */
	for (i = 0; i < nr_pages; i++) {
		page = get_free_page();
		BUG_ON(!page);

		create_mapping(pcb->pgtable, virt_addr + (i * PAGE_SIZE), page, PAGE_SIZE, false);

		add_page_to_proc(pcb, phys_to_page(page));
	}
}

/**
 *
 * Create a process from scratch, without fork'd. Typically used by the kernel
 * main at the end of the bootstrap.
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
	clone_args_t thread_args;

	local_irq_disable();

	pcb = new_process();

	reset_process_stack(pcb);

	/* We map the initial user space process stack here, and fork() will
         * inherit from this mapping */

	allocate_page(pcb, pcb->stack_top - (pcb->page_count * PAGE_SIZE), pcb->page_count, true);

#ifndef CONFIG_AVZ
	LOG_DEBUG("Stack mapped at 0x%08x (size: %d bytes)\n", pcb->stack_top - (pcb->page_count * PAGE_SIZE), PROC_STACK_SIZE);
#endif
	/* First map the code in the user space so that
         * the initial code can run normally in user mode.
         */
	create_mapping(pcb->pgtable, USER_SPACE_VADDR, __pa(__root_proc_start),
		       (void *) __root_proc_end - (void *) __root_proc_start, false);

	/* Start main user thread. */
	thread_args = (clone_args_t) {
		.name = "root_proc",
		.pcb = pcb,
		.stack = pcb->stack_top,
		.fn = (th_fn_t) USER_SPACE_VADDR,
	};
	pcb->main_thread = user_thread(&thread_args);

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

	/* At the end, the so3-boot thread which is running this function can
         * disappear ... */
	pcb->state = PROC_STATE_READY;

	thread_exit(0);

	/* Never reached */
	BUG();
}

/*
 * Release all pages allocated to a process
 */
static void release_proc_pages(pcb_t *pcb)
{
	struct list_head *pos, *q;
	page_list_t *cur;

	list_for_each_safe(pos, q, &pcb->page_list) {
		cur = list_entry(pos, page_list_t, list);

		list_del(pos);

		cur->page->refcount--;
		if (!cur->page->refcount)
			free_page(page_to_phys(cur->page));

		free(cur);
	}
}

/*
 * Set up the arguments and environment variables for the current process
 * (current()->pcb)
 */
addr_t preserve_args_and_env(int argc, char **argv, char **envp)
{
	size_t str_len;
	args_env_t *saved;
	int i;

	if ((argc > 0) && (argv == NULL)) {
		return -EINVAL;
	}

	/* Allocate kernel memory to preserve the args/env */
	saved = malloc(sizeof(args_env_t));
	BUG_ON(saved == NULL);

	memset(saved->arg_env, 0, PAGE_SIZE);

	saved->strings_size = 0;

	/* Save args strings */
	if (!argc) {
		/* If no arguments, add one with process name */
		saved->argc = 1;
		strcpy(&saved->arg_env[saved->strings_size], current()->pcb->name);
		saved->strings_size += strlen(current()->pcb->name) + 1;
	} else {
		/* Copy all arguments strings */
		saved->argc = argc;
		for (i = 0; i < argc; i++) {
			str_len = strlen(argv[i]) + 1;

			/* Ensure the newly copied string will not exceed the buffer size. */
			if (saved->strings_size + str_len > PAGE_SIZE) {
				LOG_CRITICAL("Not enougth memory allocated\n");

				free(saved);
				return -ENOMEM;
			}

			strcpy(&saved->arg_env[saved->strings_size], argv[i]);
			saved->strings_size += str_len;
		}
	}

	/* Save env strings and count how many there are */
	saved->envc = 0;
	if (envp) {
		while (envp[saved->envc]) {
			str_len = strlen(envp[saved->envc]) + 1;

			/* Ensure the newly copied string will not exceed the buffer size. */
			if (saved->strings_size + str_len > PAGE_SIZE) {
				LOG_CRITICAL("Not enougth memory allocated\n");

				free(saved);
				return -ENOMEM;
			}

			strcpy(&saved->arg_env[saved->strings_size], envp[saved->envc]);
			saved->strings_size += str_len;

			saved->envc++;
		}
	}

	return (addr_t) saved;
}

void post_setup_image(args_env_t *args_env, elf_img_info_t *elf_img_info)
{
	char **argv_p_base;
	char *str_p;
	char **env_p_base;
	char *args_base;
	int i;
	elf_addr_t *aux_elf;

	args_base = (char *) arch_get_args_base();

	/* Save argc as first arguments */
	*((long *) args_base) = args_env->argc;

	/* Get the base address for the array of pointer for args, env and aux */
	argv_p_base = (char **) (args_base + sizeof(long));
	/* Add one to account for the null termination of env */
	env_p_base = (char **) ((addr_t) argv_p_base + (args_env->argc + 1) * sizeof(char *));
	/* Add one to account for the null termination of env */
	aux_elf = (elf_addr_t *) ((addr_t) env_p_base + (args_env->envc + 1) * sizeof(char *));

	/* Adds auxiliary before args and env to get the starting address for the strings.
	 * Each auxiliary entry have an id and a value, the following temporary helper allows
	 * to easily add an entry without missing something, or adding unecessary functions.
	 * This is copied from linux/fs/binfmt_elf.c */
#define NEW_AUX_ENT(id, val)      \
	do {                      \
		*aux_elf++ = id;  \
		*aux_elf++ = val; \
	} while (0)

	/* Adds up auxiliary array. */
	NEW_AUX_ENT(AT_PAGESZ, PAGE_SIZE);
	NEW_AUX_ENT(AT_CLKTCK, clocksource_timer.rate);
	NEW_AUX_ENT(AT_ENTRY, elf_img_info->header->e_entry);

	/* By default, MUSL will use argv[0], so this isn't required as it con */
	NEW_AUX_ENT(AT_EXECFN, 0);

	/* This should contains a bitmask of hardware capabilities */
	NEW_AUX_ENT(AT_HWCAP, HWCAP_ELF);

	/* Set PHDR information, so userspace can retrieve TLS information.
	 * Due to how userspace application are built the PHDR segments is missing and so
	 * we need to manually compute its address.
	 */
	NEW_AUX_ENT(AT_PHDR, elf_img_info->header->e_phoff + (elf_img_info->segment_start_vaddr & PAGE_MASK));
	NEW_AUX_ENT(AT_PHENT, sizeof(*elf_img_info->segments));
	NEW_AUX_ENT(AT_PHNUM, elf_img_info->header->e_phnum);

	/* NULL termination */
	NEW_AUX_ENT(AT_NULL, 0);

	/* Remove the temporary helper. */
#undef NEW_AUX_ENT

	/* Copy all strings into their final destination and set pointers to them */

	/* Strings will be put just after aux */
	str_p = (char *) aux_elf;

	/* Ensure that the strings will not overflow */
	BUG_ON(args_env->strings_size + ((addr_t) str_p - (addr_t) args_base) > PAGE_SIZE);

	memcpy(str_p, args_env->arg_env, args_env->strings_size);

	/* Set the correct addresses into argv array */
	for (i = 0; i < args_env->argc; i++) {
		argv_p_base[i] = str_p;
		str_p += strlen(str_p) + 1;
	}

	/* Set the correct addresses into env array */
	for (i = 0; i < args_env->envc; i++) {
		env_p_base[i] = str_p;
		str_p += strlen(str_p) + 1;
	}
	/* Ensure the NULL terminated list */
	env_p_base[args_env->envc] = NULL;

	free(args_env);
}

/*
 * Set up the PCB fields related to the binary image to be loaded.
 */
int setup_proc_image_replace(elf_img_info_t *elf_img_info, pcb_t *pcb, int argc, char **argv, char **envp)
{
	uint32_t page_count;
	addr_t ret;
	args_env_t *__args_env;

	/* FIXME: detect fragmented executable (error)? */
	/*
         * Preserve the arguments issued from this configuration so that we
         * can place them within their definitive place in the target image.
         * Be careful to adjust the address of the string within the array; they
         * need to refer the string within the dedicated arg/env page and no
         * more at the original location.
         */

	ret = preserve_args_and_env(argc, argv, envp);
	if (ret < 0)
		return ret;

	__args_env = (args_env_t *) ret;

	/* Reset the process stack and page count */
	reset_process_stack(pcb);

	/* Release allocated pages in case of exec() within a fork'd process */
	/* The current binary image (which is a copy of the fork'd) must
         * disappeared. Associated physical pages must be removed and freed.
         * Stack area will be re-initialized.
         *
         * We reset the contents but we keep the root page table for subsequent
         * allocations.
         */
	reset_root_pgtable(pcb->pgtable, false);

	/* Release all allocated pages for user space. */
	release_proc_pages(pcb);

	/* We re-init the user space process stack here, and fork() will inherit
         * from this mapping */

	allocate_page(pcb, pcb->stack_top - (pcb->page_count * PAGE_SIZE), pcb->page_count, true);

#ifndef CONFIG_AVZ
	LOG_DEBUG("stack mapped at 0x%08x (size: %d bytes)\n", pcb->stack_top - (pcb->page_count * PAGE_SIZE), PROC_STACK_SIZE);
#endif
	/* Initialize the pc register */
	pcb->bin_image_entry = (uint32_t) elf_img_info->header->e_entry;

	/* The first virtual page will not be mapped since it is the zero-page
         * which is used to detect NULL pointer access. It has to raise a data
         * abort exception. That's the reason why the linker script of a user
         * application must start at 0x1000 (at the lowest).
         */

	page_count = ((elf_img_info->segment_end_vaddr - elf_img_info->segment_start_vaddr) >> PAGE_SHIFT) + 1;
	pcb->page_count += page_count;

	/* Map the elementary sections (text, data, bss) */
	allocate_page(pcb, elf_img_info->segment_start_vaddr, page_count, true);

	LOG_DEBUG("entry point: 0x%08x\n", elf_img_info->header->e_entry);
	LOG_DEBUG("page count: 0x%08x\n", pcb->page_count);

	/* Maximum heap size */
	page_count = ALIGN_UP(HEAP_SIZE, PAGE_SIZE) >> PAGE_SHIFT;
	pcb->heap_base = (elf_img_info->segment_end_vaddr & PAGE_MASK) + PAGE_SIZE;
	pcb->heap_pointer = pcb->heap_base;
	pcb->page_count += page_count;

	allocate_page(pcb, pcb->heap_base, page_count, true);

	LOG_DEBUG("heap mapped at 0x%08x (size: %d bytes)\n", pcb->heap_base, HEAP_SIZE);

	/* arguments (& env) will be stored in one more page */
	pcb->page_count++;

	allocate_page(pcb, arch_get_args_base(), 1, true);
	LOG_DEBUG("arguments mapped at 0x%08x (size: %d bytes)\n", arch_get_args_base(), PAGE_SIZE);

	/* Prepare the arguments within the page reserved for this purpose. */
	if (__args_env)
		post_setup_image(__args_env, elf_img_info);

	return 0;
}

/* load sections from each loadable segment into the process' virtual pages */
void load_process(elf_img_info_t *elf_img_info)
{
	int i;

	/* Manually copy the PHDR as no segement/section contains it. */
	memcpy((void *) (elf_img_info->header->e_phoff + (elf_img_info->segment_start_vaddr & PAGE_MASK)),
	       elf_img_info->file_buffer + elf_img_info->header->e_phoff,
	       elf_img_info->header->e_phnum * elf_img_info->header->e_phentsize);

	/* Loading the different segments */
	for (i = 0; i < elf_img_info->header->e_phnum; i++) {
		if (elf_img_info->segments[i].p_type != PT_LOAD)
			/* Skip unloadable segments */
			continue;

		/* Copy all required data. */
		memcpy((void *) elf_img_info->segments[i].p_vaddr,
		       (void *) (elf_img_info->file_buffer + elf_img_info->segments[i].p_offset),
		       elf_img_info->segments[i].p_filesz);

		/* Set remaining byte (bss) to zero. */
		memset((void *) (elf_img_info->segments[i].p_vaddr + elf_img_info->segments[i].p_filesz), 0,
		       elf_img_info->segments[i].p_memsz - elf_img_info->segments[i].p_filesz);
	}

	/* Make sure all data are flushed for future context switch. */
	flush_dcache_all();
}

SYSCALL_DEFINE3(execve, const char *, filename, char **, argv, char **, envp)
{
	elf_img_info_t elf_img_info;
	pcb_t *pcb;
	unsigned long flags;
	th_fn_t start_routine;
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
		return -ENOENT;
	}

	elf_load_sections(&elf_img_info);
	elf_load_segments(&elf_img_info);

	/*
         * If it is not the root process and the initial thread,
         * replace the binary image of the process. - Prepare the page frames
         * and mapped then in the virtual address space.
         */
	ret = setup_proc_image_replace(&elf_img_info, pcb, argc, argv, envp);
	if (ret < 0)
		return ret;

	/* process execution */
#warning do_exec() must be completed...
	/* TODO: close all fds except stdin stdout stderr -> including pipes (so
         * keep all, end of process will terminate fds */

	/* Load the contents of ELF into the virtual memory space */
	load_process(&elf_img_info);

	/* Release the kernel buffer used to store the ELF binary image */
	elf_clean_image(&elf_img_info);

	/* Now, we need to restart the user thread from the new program starting address */
	start_routine = (th_fn_t) pcb->bin_image_entry;

	/* Rename thread to inlcude new PCB name */
	snprintf(pcb->main_thread->name, THREAD_NAME_LEN, "%s_%d", pcb->name, pcb->main_thread->tid);

	/* Arguments base is used as stack address and arguments will be retrieved from it by MUSL. */
	arch_restart_user_thread(pcb->main_thread, start_routine, arch_get_args_base());

	local_irq_restore(flags);

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

	/* Clone all file descriptors */
	if (vfs_clone_fd(parent->fd_array, pcb->fd_array)) {
		LOG_CRITICAL("!! Error while cloning fds\n");
		kernel_panic();
	}

	return pcb;
}

static long do_clone(clone_args_t *args)
{
	/* Flags used by pthread_create in MUSL, only those are supported. */
	static const unsigned long THREAD_FLAGS = CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_THREAD |
						  CLONE_SYSVSEM | CLONE_SETTLS | CLONE_PARENT_SETTID | CLONE_CHILD_CLEARTID |
						  CLONE_DETACHED;

	unsigned long flags;
	long ret;
	pcb_t *new_pcb, *current_pcb;
	tcb_t *new_tcb;

	flags = local_irq_save();
	current_pcb = current()->pcb;

	if (args->flags & CLONE_THREAD) {
		if (args->flags != THREAD_FLAGS) {
			LOG_WARNING("Only pthread_create clone flags are supported for thread\n");
			ret = -EINVAL;
			goto error;
		}

		/* Don't need to create a new PCB, use current as "new" PCB to create the thread. */
		new_pcb = current_pcb;
	} else {
		if (args->flags != 0) {
			LOG_WARNING("Clone flags not support for forking\n");
			ret = -EINVAL;
			goto error;
		}

		/* For the time being, we *only* authorize to fork() from the main
		 * thread */
		if (current() != current_pcb->main_thread) {
			LOG_WARNING("%s: forking from a thread other than the main thread "
				    "is not allowed so far ...\n",
				    __func__);
			ret = -EINVAL;
			goto error;
		}

		/* Duplicate the elements of the parent process into the child */
		new_pcb = duplicate_process(current_pcb);

		/* Copy the user space area of the parent process */
		duplicate_user_space(current_pcb, new_pcb);

		/* At the moment, we spawn the main_thread only in the child. In the
		 * future, we will have to create a thread for each existing threads in
		 * the parent process.
		 */
		snprintf(new_pcb->name, PROC_NAME_LEN, "%s_child_%d", current_pcb->name, new_pcb->pid);
	}
	args->pcb = new_pcb;

	/* Use PCB name as thread name. The TID will be appended to it. */
	args->name = new_pcb->name;

#ifdef CONFIG_IPC_SIGNAL
	/* Copy current signal mask to new threads */
	args->sig_mask = current()->sig_mask;
#endif

	new_tcb = user_thread(args);

	/* Child thread will return in ret_from_fork and so only parent thread reachs this */
	ret = new_tcb->tid;

	if (!(args->flags & CLONE_THREAD)) {
		/* The main process thread is ready to be scheduled for its execution.*/
		new_pcb->state = PROC_STATE_READY;

		BUG_ON(!local_irq_is_disabled());

		/* Prepare to perform scheduling to check if a context switch is
		 * required. */
		raise_softirq(SCHEDULE_SOFTIRQ);

		/* Process clone should return the PID instead of the TID. */
		ret = new_pcb->pid;
	}

error:
	local_irq_restore(flags);
	return ret;
}

/*
 * For a new process from the current running process.
 */
SYSCALL_DEFINE0(fork)
{
	clone_args_t args = {};

	return do_clone(&args);
}

SYSCALL_DEFINE5(clone, unsigned long, flags, unsigned long, newsp, int *, parent_tid, unsigned long, tls, int *, child_tid)
{
	clone_args_t args = {
		.flags = flags & ~CSIGNAL,
		.stack = newsp,
		.parent_tid = parent_tid,
		.tls = tls,
		.child_tid = child_tid,
	};

	return do_clone(&args);
}

/*
 * Terminates a process.
 * All allocated resources should be released except its PCB which still
 * contains the exit code.
 */
SYSCALL_DEFINE1(exit_group, int, exit_status)
{
	pcb_t *pcb;
	unsigned i;

#ifdef CONFIG_ARCH_ARM32
	register int reg0 asm("r0");
	register int reg1 asm("r1");
#endif

	pcb = current()->pcb;

	/* Never finish the root process */
	if (pcb->parent == NULL) {
#ifdef CONFIG_ARCH_ARM32
		reg0 = 0x18; /* SYSEXIT */
		reg1 = 0x20026; /* ADP_Stopped_ApplicationExit */

		asm("svc 0x00123456"); // make semihosting call
#else
		semi_exit(0);

#endif
		LOG_CRITICAL("<kernel> %s: cannot finish the root process ...\n", __func__);
		kernel_panic();
	}

	/* Close the file descriptors - IRQs must remain on since we need
         * locking in the low layers. */

	for (i = 0; i < FD_MAX; i++)
		sys_do_close(i);

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
	sys_do_kill(pcb->parent->pid, SIGCHLD);

#endif /* CONFIG_IPC_SIGNAL */

	/*
         * We are finished ... Properly terminate the process main thread, which
         * will lead to the wake up of the parent.
         */

	thread_exit(0);

	return 0;
}

/*
 * Returns the PID of the current process
 */
SYSCALL_DEFINE0(getpid)
{
	return current()->pcb->pid;
}

/*
 * Waitpid implementation via wait4 - sys_do_wait4() does the following operations:
 * - Suspend the current process until the child process finished its execution
 * (exit()) If the pid argument is -1, waitpid() looks for a possible terminated
 * process and performs the operation. If no process is finished (in zombie
 * state), waitpid() will return 0 (if pid argument is -1).
 * - Get the exit code from the child PCB
 * - Clean the PCB and page tables
 * - Return the pid if successful operation
 *
 * rusage is ignored
 */
SYSCALL_DEFINE4(wait4, int, pid, uint32_t *, wstatus, uint32_t, options, void *, rusage)
{
	pcb_t *child;
	unsigned long flags;

	flags = local_irq_save();

	if (pid == -1) {
		child = find_proc_zombie_to_clean();
		if (!child) {
			local_irq_restore(flags);
			return -ECHILD;
		}
		pid = child->pid;
	} else {
		/* Get the child process identified by its pid. */
		child = find_proc_by_pid(pid);
		if (!child) {
			if (wstatus != NULL)
				*wstatus = ~0x7f; /* !WTERMSIG -> WIFEXITED true */
			return -ECHILD;
		}
	}
	/* In the case the of NOHANG if the child did not change state,
         * the waitpid return 0;
         */
	if (options & WNOHANG)
		if (child->state != PROC_STATE_ZOMBIE)
			return 0;

	if (child->state != PROC_STATE_WAITING) {
		/* This child is now the foreground process: console signals
		 * (e.g. Ctrl-C -> SIGINT) must target it while we block on it.
		 * Restore the foreground to ourselves (the waiter, e.g. the
		 * shell) once it has finished. */
		fg_pcb = child;

		/* Wait on the main_thread of this process */
		thread_join(child->main_thread);

		fg_pcb = current()->pcb;
	}

	/* Free the page tables used for this process */
	reset_root_pgtable(child->pgtable, true);

	/* Get the exit code left in the PCB by the child */
	if (wstatus) {
		*wstatus = ~0x7f; /* !WTERMSIG -> WIFEXITED true */
		*wstatus |= ((char) child->exit_status) << 8;
	}

	/*
	 * SO3 approach consists in avoiding having orphan process.
	 * The process will be removed from the system definitively only
	 * if it has no children.
	 */
	if (!find_proc_by_parent(child))
		remove_proc(child);

	local_irq_restore(flags);

	return pid;
}

/* @brief Currently this function does not allocate pages.
 *		page are already allocated. this function will only
 *		increase the heap_pointer and make sure it does not
 *		overflow the heap. If there is no memory left it will
 *		return ENONMEM;
 * @param addr New end address requested by userspace.
 *
 * @return End address of the available heap.
 */
SYSCALL_DEFINE1(brk, void *, addr)
{
	pcb_t *pcb = current()->pcb;
	int req_sz = 0;

	if (!pcb)
		/* case there is no pcb context */
		return -ESRCH;

	if (addr == NULL)
		return pcb->heap_pointer;

	/* we make sure the future size of the heap is not overflowing /
         * underflowing*/
	req_sz = (addr_t) addr - pcb->heap_base;

	if ((req_sz >= HEAP_SIZE) || (req_sz < 0))
		return -ENOMEM;

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

	pcb->heap_pointer = (addr_t) addr;
	return pcb->heap_pointer;
}

#ifdef CONFIG_PRIORITY_SCHEDULER
void do_ps()
{
	pcb_t *pcb = NULL;
	tcb_t *tcb = NULL;
	struct list_head *proc_pos, *thread_pos;

	LOG_INFO("\n****************************************************\n");
	if (list_empty(&proc_list)) {
		printk(" process list is <empty>\n");
		printk("\n****************************************************"
		       "\n\n");
		return;
	}

	list_for_each(proc_pos, &proc_list) {
		/* find and print main thread */
		pcb = list_entry(proc_pos, pcb_t, list);
		LOG_INFO(" [pid %02d] [main_tid %02d : priority %03d : %s]", pcb->pid, pcb->main_thread->tid,
			 pcb->main_thread->priority, pcb->main_thread->name);

		/* find and print other threads */
		if (list_empty(&pcb->threads)) {
			LOG_INFO("\n*********************************************"
				 "*******\n\n");
			return;
		}
		list_for_each(thread_pos, &pcb->threads) {
			tcb = list_entry(thread_pos, tcb_t, list);
			LOG_INFO(" [other_tid %02d : priority %03d : %s]\n", tcb->tid, tcb->priority, tcb->name);
		}
	}
	LOG_INFO("\n****************************************************\n\n");
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
		LOG_ERROR("Number of local fd reached\n");
		return -ENFILE;
	}

	pcb->fd_array[fd] = gfd;

	return fd;
}

void dump_proc(void)
{
	pcb_t *pcb = NULL;

	LOG_INFO("********* List of processes **********\n\n");

	list_for_each_entry(pcb, &proc_list, list) {
		/* Based on process main thread. */

		LOG_INFO(" [pid %d state: %s] [main_tid: %d name: %s]\n", pcb->pid, proc_state_str(pcb->state),
			 ((pcb->main_thread != NULL) ? pcb->main_thread->tid : -1), pcb->main_thread->name);
	}
}
