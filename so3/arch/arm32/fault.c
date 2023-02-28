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
#include <schedule.h>

#include <generated/asm-offsets.h>

#include <asm/mmu.h>
#include <asm/processor.h>

void __stack_alignment_fault(void) {
	lprintk("### wrong stack alignment (8-bytes not respected) !! ###");
	kernel_panic();
}

void __prefetch_abort(uint32_t ifar, uint32_t ifsr, uint32_t lr) {
	lprintk("### On CPU %d prefetch abort exception ifar: %x ifsr: %x lr(r14)-8: %x cr: %x current thread: %s ###\n", smp_processor_id(),
		ifar, ifsr, lr-8, get_cr(), current()->name);

	kernel_panic();
}

void __data_abort(uint32_t far, uint32_t fsr, uint32_t lr) {
	lprintk("### On CPU %d abort exception far: %x fsr: %x lr(r14)-8: %x cr: %x current thread: %s ###\n", smp_processor_id(),
		far, fsr, lr-8, get_cr(), current()->name);

	kernel_panic();
}

void __undefined_instruction(uint32_t lr) {
	lprintk("### On CPU %d undefined instruction lr(r14)-8: %x current thread: %s ###\n", smp_processor_id(),
		lr-8, current()->name);

	kernel_panic();
}

void __div0(void) {
	lprintk("### division by 0\n");
	kernel_panic();
}

void kernel_panic(void)
{
	if (cpu_mode() == PSR_USR_MODE)
		printk("%s: entering infinite loop...\n", __func__);
	else {
		lprintk("%s: entering infinite loop... CPU: %d\n", __func__, smp_processor_id());

#ifdef CONFIG_VEXPRESS
		{
			extern void send_qemu_halt(void);
			send_qemu_halt();
		}
#endif
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

/*
 * Mostly used for debugging purposes
 */
void dumpregisters(void) {
	register uint32_t *sp __asm__("sp");

	lprintk("## fp: %x\n", *(sp + OFFSET_FP/4));
	lprintk("## sp: %x\n", *(sp + OFFSET_SP/4));
	lprintk("## lr: %x\n", *(sp + OFFSET_LR/4));
	lprintk("## pc: %x\n", *(sp + OFFSET_PC/4));

	lprintk("## sp_usr: %x\n", *(sp + OFFSET_SP_USR/4));
	lprintk("## lr_usr: %x\n", *(sp + OFFSET_LR_USR/4));

}

void trap_handle(cpu_regs_t *regs) {

	/* To be implemented */

}
