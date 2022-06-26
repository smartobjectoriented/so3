/*
 * Copyright (C) 2014-2022 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#include <ptrace.h>
#include <user.h>
#include <process.h>

/**
 * Update the CPU registers of the TCB belonging
 * to the current thread.
 */
void update_cpu_regs(void) {
	register uint32_t __r0 asm("r0");
	register uint32_t __r1 asm("r1");
	register uint32_t __r2 asm("r2");
	register uint32_t __r3 asm("r3");
	register uint32_t __r4 asm("r4");
	register uint32_t __r5 asm("r5");
	register uint32_t __r6 asm("r6");
	register uint32_t __r7 asm("r7");
	register uint32_t __r8 asm("r8");
	register uint32_t __r9 asm("r9");
	register uint32_t __r10 asm("r10");
	register uint32_t __r11 asm("r11");
	register uint32_t __r12 asm("r12");
	register uint32_t __r13 asm("r13");
	register uint32_t __r14 asm("r14");

	/* Keep the values permanent */
	uint32_t r0 = __r0;
	uint32_t r1 = __r1;
	uint32_t r2 = __r2;
	uint32_t r3 = __r3;
	uint32_t r4 = __r4;
	uint32_t r5 = __r5;
	uint32_t r6 = __r6;
	uint32_t r7 = __r7;
	uint32_t r8 = __r8;
	uint32_t r9 = __r9;
	uint32_t r10 = __r10;
	uint32_t r11 = __r11;
	uint32_t r12 = __r12;
	uint32_t r13 = __r13;
	uint32_t r14 = __r14;


	/* Finally update the tcb structure */
	tcb_t *tcb = current();

	tcb->cpu_regs.r0 = r0;
	tcb->cpu_regs.r1 = r1;
	tcb->cpu_regs.r2 = r2;
	tcb->cpu_regs.r3 = r3;
	tcb->cpu_regs.r4 = r4;
	tcb->cpu_regs.r5 = r5;
	tcb->cpu_regs.r6 = r6;
	tcb->cpu_regs.r7 = r7;
	tcb->cpu_regs.r8 = r8;
	tcb->cpu_regs.r9 = r9;
	tcb->cpu_regs.r10 = r10;
	tcb->cpu_regs.fp = r11;
	tcb->cpu_regs.ip = r12;
	tcb->cpu_regs.sp = r13;
	tcb->cpu_regs.lr = r14;
}

void retrieve_cpu_regs(struct user *uregs, pcb_t *pcb) {

	uregs->regs.uregs[0] = pcb->main_thread->cpu_regs.r0;
	uregs->regs.uregs[1] = pcb->main_thread->cpu_regs.r1;
	uregs->regs.uregs[2] = pcb->main_thread->cpu_regs.r2;
	uregs->regs.uregs[3] = pcb->main_thread->cpu_regs.r3;
	uregs->regs.uregs[4] = pcb->main_thread->cpu_regs.r4;
	uregs->regs.uregs[5] = pcb->main_thread->cpu_regs.r5;
	uregs->regs.uregs[6] = pcb->main_thread->cpu_regs.r6;
	uregs->regs.uregs[7] = pcb->main_thread->cpu_regs.r7;
	uregs->regs.uregs[8] = pcb->main_thread->cpu_regs.r8;
	uregs->regs.uregs[9] = pcb->main_thread->cpu_regs.r9;
	uregs->regs.uregs[10] = pcb->main_thread->cpu_regs.r10;
	uregs->regs.uregs[11] = pcb->main_thread->cpu_regs.fp;
	uregs->regs.uregs[12] = pcb->main_thread->cpu_regs.ip;
	uregs->regs.uregs[13] = pcb->main_thread->cpu_regs.sp;
	uregs->regs.uregs[14] = pcb->main_thread->cpu_regs.lr;

}
