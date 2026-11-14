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

#include <common.h>

#include <asm/processor.h>
#include <asm/backtrace.h>

void show_registers(cpu_regs_t *regs);

/* Walk the frame pointer chain, as far as it is present */

void __backtrace(void)
{
	u64 *rbp;
	int depth;

	asm volatile("mov %%rbp, %0" : "=r"(rbp));

	for (depth = 0; depth < 16 && rbp && ((addr_t) rbp >= CONFIG_KERNEL_VADDR); depth++) {
		lprintk("  [<%016lx>]\n", rbp[1]);
		rbp = (u64 *) rbp[0];
	}
}

void dump_stack(void)
{
	lprintk("Call trace:\n");
	__backtrace();
}

void dump_execution_state(void)
{
	dump_stack();
}

void dump_all_execution_state(void)
{
	dump_stack();
}

void kernel_panic(void)
{
	lprintk("%s: entering infinite loop... CPU: %d\n", __func__, smp_processor_id());

	/* Stop all activities. */

	local_irq_disable();
	while (1)
		wfi();
}

void _bug(char *file, int line)
{
	lprintk("BUG in %s at line: %d\n", file, line);
	dump_stack();
	kernel_panic();
}
