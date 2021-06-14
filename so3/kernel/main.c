/*
 * Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
 * Copyright (C) 2014 Romain Bornet <romain.bornet@heig-vd.ch>
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

#include <common.h>
#include <calibrate.h>
#include <schedule.h>
#include <memory.h>
#include <mmc.h>
#include <vfs.h>
#include <process.h>
#include <timer.h>
#include <version.h>

#include <asm/atomic.h>
#include <asm/setup.h>
#include <asm/mmu.h>

#include <device/driver.h>

boot_stage_t boot_stage = BOOT_STAGE_INIT;

/**
 * Initialization of initcalls which have to be done right before IRQs are enabled.
 */
void pre_irq_init(void) {

	pre_irq_init_t *pre_irq_init;
	int i;

	pre_irq_init = ll_entry_start(pre_irq_init_t, core);

	for (i = 0; i < ll_entry_count(pre_irq_init_t, core); i++)
		pre_irq_init[i]();

}

/**
 * Remaining initialization which can be performed with IRQs on and full scheduling.
 */
void post_init(void) {

	postinit_t *postinit;
	int i;

	postinit = ll_entry_start(postinit_t, core);

	for (i = 0; i < ll_entry_count(postinit_t, core); i++)
		postinit[i]();
}

/*
 * Initial (root) process which will start the first process running in SO3.
 * The process is running in user mode.
 */
int root_proc(void *args)
{
	printk("SO3: starting the initial process (shell) ...\n\n\n");

	/* Start the first process */
	__exec("sh.elf");

	/* We normally never runs here, if the exec() succeeds... */
	printk("so3: No init proc (shell) found ...\n");

	kernel_panic();

	return 0; /* Make gcc happy ;-) */
}

int rest_init(void *dummy) {

	post_init();

	/* Start a first SO3 thread (main app thread) */
#if defined(CONFIG_THREAD_ENV)

	kernel_thread(app_thread_main, "main_kernel", NULL, 0);

	thread_exit(NULL);

#elif defined(CONFIG_PROC_ENV)

	/* Launch the root process (should be the shell...) */
	create_process(root_proc, "root_proc");

	/* We should never reach this ... */
	BUG();
#else
#error "Can not start initial SO3 environment"
#endif

	return 0;
}

void kernel_start(void) {

	/* Basic low-level initialization */
	setup_arch();

	lprintk("\n\n********** Smart Object Oriented SO3 Operating System **********\n");
	lprintk("Copyright (c) 2014-2020 REDS Institute, HEIG-VD, Yverdon\n");
	lprintk("Version %s\n", SO3_KERNEL_VERSION);

	lprintk("\n\nNow bootstraping the kernel ...\n");

	/* Memory manager subsystem initialization */
	memory_init();

	devices_init();

	/* At this point of time, we are able to use the standard printk() */

	timer_init();

	vfs_init();

	/* Scheduler init */
	scheduler_init();

	pre_irq_init();

	boot_stage = BOOT_STAGE_IRQ_ENABLE;

	local_irq_enable();

	calibrate_delay();

	/*
	 * Perform the rest of bootstrap sequence in a separate thread, so that
	 * we can rely on the scheduler for subsequent threads.
	 * The priority is max (99) over other possible threads (normally there is no such thread at this time).
	 */
	kernel_thread(rest_init, "so3_boot", NULL, 99);

	boot_stage = BOOT_STAGE_COMPLETED;

	/*
	 * We loop forever, just the time the scheduler gives the hand to a ready thread.
	 * After that, this code will never be executed anymore ...
	 */

	schedule();

}
