/*
 * Copyright (C) 2021 Nicolas Müller <nicolas.muller1@heig-vd.ch>
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

#if 1
#define DEBUG
#endif

#include <common.h>
#include <asm/csr.h>
#include <device/irq.h>
#include <softirq.h>

#include <device/arch/riscv_timer.h>
#include <asm/trap.h>
#include <asm/fault.h>
#include <asm/sbi.h>

#define EXCEPTION_COUNT 	16

extern void irq_handle(cpu_regs_t *regs);
extern irq_return_t timer_isr(int irq, void *dummy);

/* Store exceptions.. 16 is the number of standard exceptions */
static exception_handler_t exception_handlers[EXCEPTION_COUNT];
static bool exception_handler_registred[EXCEPTION_COUNT];

void trap_dump_regs(cpu_regs_t *regs);

/* Once in a trap, registers are stored in the trap frame. Only machine mode has a trap frame.
 * Supervisor traps are stored on the stack directly */
cpu_regs_t mtrap_frame;

u64 handle_mtrap(u64 epc, u64 tval, u64 cause, u64 status, cpu_regs_t *regs) {

	/* Interrupt source is on the other bits of mcause reg.
	 * max trap source is 15 yet.. Mask 0xfff is more than enough */
	u64 trap_source = cause & 0xfff;

	/* If reg MSB is 1, it is an interrupt */
	if (cause & CAUSE_IRQ_FLAG) {

		/* Only machine trap that can occur is machine timer irq */
		if (trap_source == IRQ_M_TIMER) {

			/* Clear the machine mode bit to avoid another irq and raise S-mode timer irq */
			csr_clear(CSR_MIE, IE_MTIE);
			csr_set(CSR_MIP, IE_STIE);
		}
		else {
			/* Sould not happen since external irqs are supervisor irqs and last type of irq
			 * are soft irq.. There is no softirqs in SO3 */
			printk("Ignoring unkown IRQ source in MACHINE mode: No is %d\n", trap_source);
		}
	}
	/* Else it is an exception or an ecall */
	else {

		/* If we get an ecall from supervisor mode */
		if (trap_source == MCAUSE_SUPERVISOR_ECALL) {
			sbi_ecall_handler(regs);
			/* instructions are 32 bits => 4 bytes. mepc holds pc of instruction at the time
			 * of the trap. To continue the execution normally, code has to return one instruction
			 * further. This is only a problem for ecalls and not for regular traps where mepc also
			 * holds current instruction but could not be executed because of the trap. */
			epc += 4;
		}

		/* Else it's an exception. Print debug info */
		else if (exception_handler_registred[trap_source]) {
			printk("### Got MACHINE exception at :\n"
				   "### instr addr:   %#16x\n"
				   "### mstatus reg:  %#16x\n"
				   "### mtval:        %#16x\n", epc, status, tval);
#ifdef DEBUG
			/* Print prq-trap registers */
			trap_dump_regs(regs);
#endif
			/* Call exception handler of correct cause */
			exception_handlers[trap_source]();
		}
		else {
			printk("Got Unimplemented Environement call (ECALL) or Breakpoint (EBREAK) "
					": No is %d\n"
					"There is no implementation for this case yet !\n", trap_source);
			kernel_panic();
		}
	}
	return epc;
}

u64 handle_strap(u64 epc, u64 tval, u64 cause, u64 status, cpu_regs_t *regs) {

	/* Interrupt source is on the other bits of mcause reg.
	 * max trap source is 15 yet.. Mask 0xfff is more than enough */
	u64 trap_source = cause & 0xfff;

	/* If reg MSB is 1, it is an interrupt */
	if (cause & CAUSE_IRQ_FLAG) {

		switch(trap_source) {

			/* Goes to timer ISR */
			case RV_IRQ_TIMER:

				__in_interrupt = true;

				timer_isr(trap_source, NULL);

				/* Perform the softirqs if allowed */
				if (!irqs_disabled_before_irq_flags(regs))
					do_softirq();

				break;
			/* Will be handled by the PLIC */
			case RV_IRQ_EXT:

				/* This function is located in irq.c and is the generic way to handle an
				 * irq. */
				irq_handle(regs);

				break;
			default:
				/* Sould not happen since last type possible here is for SOFT_IRQs and SO3
				 * doesn't use softirqs as real hardware irqs. Notify it anyways just ine case. */
				printk("Ignoring unkown IRQ source : No is %d\n", trap_source);
		}

	}
	/* Else it is an exception */
	else {

		/* Only ecalls we get here are user ecalls.. They don't exist yet */

		/* Print debug info */
		if (exception_handler_registred[trap_source]) {
			printk("### Got SUPERVISOR exception at :\n"
				   "### instr addr:   %#16x\n"
				   "### sstatus reg:  %#16x\n"
				   "### stval:        %#16x\n", epc, status, tval);
#ifdef DEBUG
			/* Print prq-trap registers */
			trap_dump_regs(regs);
#endif
			/* Call exception handler of correct cause */
			exception_handlers[trap_source]();
		}
		else {
			printk("Got Environement call (ECALL) or Breakpoint (EBREAK) "
					": No is %d\n"
					"There is no implementation for this case yet !\n", trap_source);
			kernel_panic();
		}
	}
	return epc;
}

void init_trap() {
	int i;

	/* Set all exception handler*/
	for (i = 0; i < EXCEPTION_COUNT; i++) {
		switch (i) {
		case MCAUSE_INSTR_ADDR_MISALIGNED	:
			exception_handler_registred[i] = true;
			exception_handlers[i] = __instr_addr_misalignment;
			break;
		case MCAUSE_INSTR_ACCESS_FAULT		:
			exception_handler_registred[i] = true;
			exception_handlers[i] = __instr_access_fault;
			break;
		case MCAUSE_ILLEGAL_INSTR			:
			exception_handler_registred[i] = true;
			exception_handlers[i] = __illegal_instr;
			break;
		case MCAUSE_LOAD_ADDR_MISALIGNED	:
			exception_handler_registred[i] = true;
			exception_handlers[i] = __load_addr_misalignement;
			break;
		case MCAUSE_LOAD_ACCESS_FAULT		:
			exception_handler_registred[i] = true;
			exception_handlers[i] = __load_access_fault;
			break;
		case MCAUSE_STORE_ADDR_MISALIGNED	:
			exception_handler_registred[i] = true;
			exception_handlers[i] = __store_AMO_addr_misaligned;
			break;
		case MCAUSE_STORE_ACCESS_FAULT		:
			exception_handler_registred[i] = true;
			exception_handlers[i] = __store_AMO_access_fault;
			break;
		case MCAUSE_INSTR_PAGE_FAULT		:
			exception_handler_registred[i] = true;
			exception_handlers[i] = __instr_page_fault;
			break;
		case MCAUSE_LOAD_PAGE_FAULT		:
			exception_handler_registred[i] = true;
			exception_handlers[i] = __load_page_fault;
			break;
		case MCAUSE_STORE_PAGE_FAULT		:
			exception_handler_registred[i] = true;
			exception_handlers[i] = __store_AMO_page_fault;
			break;
		default:
			exception_handler_registred[i] = false;
			exception_handlers[i] = NULL;
			break;
		}
	}
}

void trap_dump_regs(cpu_regs_t *regs) {

	printk("ra:  %#016x\n", regs->ra);
	printk("sp:  %#016x\n", regs->sp);
	printk("gp:  %#016x\n", regs->gp);
	printk("tp:  %#016x\n", regs->tp);
	printk("t0:  %#016x\n", regs->t0);
	printk("t1:  %#016x\n", regs->t1);
	printk("t2:  %#016x\n", regs->t2);
	printk("fp:  %#016x\n", regs->fp);
	printk("s1:  %#016x\n", regs->s1);
	printk("a0:  %#016x\n", regs->a0);
	printk("a1:  %#016x\n", regs->a1);
	printk("a2:  %#016x\n", regs->a2);
	printk("a3:  %#016x\n", regs->a3);
	printk("a4:  %#016x\n", regs->a4);
	printk("a5:  %#016x\n", regs->a5);
	printk("a6:  %#016x\n", regs->a6);
	printk("a7:  %#016x\n", regs->a7);
	printk("s2:  %#016x\n", regs->s2);
	printk("s3:  %#016x\n", regs->s3);
	printk("s4:  %#016x\n", regs->s4);
	printk("s5:  %#016x\n", regs->s5);
	printk("s6:  %#016x\n", regs->s6);
	printk("s7:  %#016x\n", regs->s7);
	printk("s8:  %#016x\n", regs->s8);
	printk("s9:  %#016x\n", regs->s9);
	printk("s10: %#016x\n", regs->s10);
	printk("s11: %#016x\n", regs->s11);
	printk("t3:  %#016x\n", regs->t3);
	printk("t4:  %#016x\n", regs->t4);
	printk("t5:  %#016x\n", regs->t5);
	printk("t6:  %#016x\n", regs->t6);
}

