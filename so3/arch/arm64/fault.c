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

void __check(void) {
	register unsigned long __sp asm("sp");

	lprintk("## check  sp: %lx\n", __sp);
}

void __stack_alignment_fault(void) {
	lprintk("### wrong stack alignment (8-bytes not respected) !! ###");
	kernel_panic();
}

void __sync_serror(addr_t lr) {

	lprintk("### Got a SError lr: 0x%lx ###\n", lr);

	kernel_panic();
}

void __sync_el2_fault(addr_t lr, addr_t sp) {

	lprintk("### Got a sync interrupt in EL2 / lr: 0x%lx sp: 0x%lx ###\n", lr, sp);

	kernel_panic();
}

#if 0

void __prefetch_abort(uint32_t ifar, uint32_t ifsr, uint32_t lr) {
	lprintk("### prefetch abort exception ifar: %x ifsr: %x lr(r14)-8: %x cr: %x ###\n", ifar, ifsr, lr-8, get_cr());

	kernel_panic();
}

void __data_abort(uint32_t far, uint32_t fsr, uint32_t lr) {
	lprintk("### abort exception far: %x fsr: %x lr(r14)-8: %x cr: %x ###\n", far, fsr, lr-8, get_cr());

	kernel_panic();
}

void __undefined_instruction(uint32_t lr) {
	lprintk("### undefined instruction lr(r14)-8: %x ###\n", lr-8);

	kernel_panic();
}
#endif

void __div0(void) {
	lprintk("### division by 0\n");
	kernel_panic();
}

void kernel_panic(void)
{
	if (user_mode())
		printk("%s: entering infinite loop...\n", __func__);
	else {
		lprintk("%s: entering infinite loop... CPU: %d\n", __func__, smp_processor_id());

	}
	/* Stop all activities. */
	local_irq_disable();

	while (1);
}

void _bug(char *file, int line)
{
	lprintk("BUG in %s at line: %d\n", file, line);

	kernel_panic();
}

