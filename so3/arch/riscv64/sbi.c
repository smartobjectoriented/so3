/*
 * Copyright (C) 2021 	   Nicolas Müller <nicolas.muller1@heig-vd.ch>
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

#include <device/arch/riscv_timer.h>

#include <asm/processor.h>
#include <asm/io.h>

#include <mach/timer.h>

/* define ecall numbers here. defines can stay private..
 * user will call sbi function and not the ecall directly */
#define ECALL_TIMER_RELOAD		1

/* Called from supervisor privilege */
void sbi_timer_next_event(u64 next) {

	/* set arguments for ecall instruction */
	register u64 arg_next asm("a0") = next;
	register int no_call asm("a7") = ECALL_TIMER_RELOAD;

	/* make the call, will switch to machine mode */
	asm volatile ("ecall\n"
			  :
		      : "r"(arg_next), "r"(no_call)
		      : "memory");
}

/* Called in machine privilege. */
static void __timer_next_event(u64 next) {

	u64 *mtimecmp_addr = (u64*) TIMER_MTIMECMP_REG;

	/* Renable machine irqs and reset irq */
	csr_set(CSR_MIE, IE_MTIE);
	csr_clear(CSR_MIP, IE_STIE);

	/* Set new CMP register value. This clears interrupt as well. Interrupt is only the
	 * result of the comparator between mtime and mtimecmp. If correct value is written,
	 * interrupt is cleared */
	iowrite64(mtimecmp_addr, arch_get_time() + next);
}

void sbi_ecall_handler(cpu_regs_t *regs)
{
	u64 ecall_number = regs->a7;

	/* From specifications, args can be from a0 to a5. Only ecall implemented in SO3
	 * currently uses only one. It's fine to do it like this for now.. Should use pointer
	 * to first arg and counter. */
	u64 ecall_arg = regs->a0;

	switch(ecall_number) {

	case ECALL_TIMER_RELOAD:
		__timer_next_event(ecall_arg);
		break;

	default:
		printk("Unknown ecall.. Ignoring call.\n");
		break;
	}
}
