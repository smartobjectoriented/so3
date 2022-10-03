/*
 * Copyright (C) 2022 Daniel Rossier <daniel.rossier//heig-vd.ch>
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
#include <syscall.h>

#include <asm/processor.h>

const char entry_error_messages[19][32] =
{
    "SYNC_INVALID_EL1t",
    "IRQ_INVALID_EL1t",
    "FIQ_INVALID_EL1t",
    "SERROR_INVALID_EL1t",
    "SYNC_INVALID_EL1h",
    "IRQ_INVALID_EL1h",
    "FIQ_INVALID_EL1h",
    "SERROR_INVALID_EL1h",
    "SYNC_INVALID_EL0_64",
    "IRQ_INVALID_EL0_64",
    "FIQ_INVALID_EL0_64",
    "SERROR_INVALID_EL0_64",
    "SYNC_INVALID_EL0_32",
    "IRQ_INVALID_EL0_32",
    "FIQ_INVALID_EL0_32",
    "SERROR_INVALID_EL0_32",
    "SYNC_ERROR",
    "SYSCALL_ERROR",
    "DATA_ABORT_ERROR"
};

void show_invalid_entry_message(u32 type, u64 esr, u64 address)
{
    printk("ERROR CAUGHT: ");
    printk(entry_error_messages[type]);
    printk(", ESR: ");
    printk("%lx", esr);
    printk(", Address: ");
    printk("%lx\n", address);

}

void trap_handle_error(addr_t lr) {
	unsigned long esr = read_sysreg(esr_el1);

	show_invalid_entry_message(ESR_ELx_EC(esr), esr, lr);
}

/**
 * This is the entry point for all exceptions currently managed by SO3.
 *
 * @param regs	Pointer to the stack frame
 */
void trap_handle(cpu_regs_t *regs) {
	unsigned long esr = read_sysreg(esr_el1);

	switch (ESR_ELx_EC(esr)) {

	/* SVC used for syscalls */
	case ESR_ELx_EC_SVC64:
		local_irq_enable();
		regs->x0 = syscall_handle(regs->x0, regs->x1, regs->x2, regs->x3);
		local_irq_disable();
		break;

#if 0
	case ESR_ELx_EC_DABT_LOW:
		break;
	case ESR_ELx_EC_IABT_LOW:;
		break;
	case ESR_ELx_EC_FP_ASIMD:
		break;
	case ESR_ELx_EC_SVE:
		el0_sve_acc(regs, esr);
		break;
	case ESR_ELx_EC_FP_EXC64:
		el0_fpsimd_exc(regs, esr);
		break;
	case ESR_ELx_EC_SYS64:
	case ESR_ELx_EC_WFx:
		el0_sys(regs, esr);
		break;
	case ESR_ELx_EC_SP_ALIGN:
		el0_sp(regs, esr);
		break;
	case ESR_ELx_EC_PC_ALIGN:
		el0_pc(regs, esr);
		break;
	case ESR_ELx_EC_UNKNOWN:
		el0_undef(regs);
		break;
	case ESR_ELx_EC_BTI:
		el0_bti(regs);
		break;
	case ESR_ELx_EC_BREAKPT_LOW:
	case ESR_ELx_EC_SOFTSTP_LOW:
	case ESR_ELx_EC_WATCHPT_LOW:
	case ESR_ELx_EC_BRK64:
		el0_dbg(regs, esr);
		break;
	case ESR_ELx_EC_FPAC:
		el0_fpac(regs, esr);
		break;
#endif

	default:
		lprintk("### ESR_Elx_EC(esr): %lx\n", ESR_ELx_EC(esr));
		trap_handle_error(regs->lr);
		kernel_panic();
	}

}
