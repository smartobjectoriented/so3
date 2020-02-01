/*
 * Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#include <asm/processor.h>

void __stack_alignment_fault(void) {
	lprintk("### wrong stack alignment (8-bytes not respected) !! ###");
	kernel_panic();
}

void dump_backtrace_entry(unsigned long where, unsigned long from)
{
        lprintk("Function entered at [<%08lx>] from [<%08lx>]\n", where, from);
}

void __prefetch_abort(uint32_t ifar, uint32_t ifsr) {
	lprintk("### prefetch abort exception ifar: %x ifsr: %x ###\n", ifar, ifsr);

	__backtrace();

	kernel_panic();
}

void __data_abort(uint32_t far, uint32_t fsr) {
	lprintk("### abort exception far: %x fsr: %x ###\n", far, fsr);
	kernel_panic();
}

void __div0(void) {
	lprintk("### division by 0\n");
	kernel_panic();
}

void kernel_panic(void)
{
	lprintk("%s: entering infinite loop... CPU: %d\n", __func__, smp_processor_id());

	__backtrace();

#ifdef CONFIG_VEXPRESS
	{
		extern void send_qemu_halt(void);
		send_qemu_halt();
	}
#endif

	/* Stop all activities. */
	local_irq_disable();

	while (1);
}

void _bug(char *file, int line)
{
	lprintk("BUG in %s at line: %d\n", file, line); \
	kernel_panic();
}

