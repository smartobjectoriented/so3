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
#include <psci.h>
#include <errno.h>
#include <smp.h>

#ifdef CONFIG_AVZ
#include <avz/sched.h>
#include <avz/hypercall.h>
#include <avz/event.h>

#include <avz/uapi/domctl.h>

#include <asm/cacheflush.h>
#include <asm/setup.h>

#else
#include <syscall.h>
#endif

#include <asm/processor.h>

#ifdef CONFIG_AVZ

const char entry_error_messages[19][32] =
{
    "SYNC_INVALID_EL2t",
    "IRQ_INVALID_EL2t",
    "FIQ_INVALID_EL2t",
    "SERROR_INVALID_EL2t",
    "SYNC_INVALID_EL2h",
    "IRQ_INVALID_EL2h",
    "FIQ_INVALID_EL2h",
    "SERROR_INVALID_EL2h",
    "SYNC_INVALID_EL1_64",
    "IRQ_INVALID_EL1_64",
    "FIQ_INVALID_EL1_64",
    "SERROR_INVALID_EL1_64",
    "SYNC_INVALID_EL1_32",
    "IRQ_INVALID_EL1_32",
    "FIQ_INVALID_EL1_32",
    "SERROR_INVALID_EL1_32",
    "SYNC_ERROR",
    "SYSCALL_ERROR",
    "DATA_ABORT_ERROR"
};

#else

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

#endif

void show_invalid_entry_message(u32 type, u64 esr, u64 address)
{
    printk("CPU%d: ERROR CAUGHT: ", smp_processor_id());
    printk(entry_error_messages[type]);
    printk(", ESR: ");
    printk("%lx", esr);
    printk(", Address: ");
    printk("%lx\n", address);

    while (1);

}

void trap_handle_error(addr_t lr) {
#ifdef CONFIG_AVZ
	unsigned long esr = read_sysreg(esr_el2);
#else
	unsigned long esr = read_sysreg(esr_el1);
#endif

	show_invalid_entry_message(ESR_ELx_EC(esr), esr, lr);
}

#ifdef CONFIG_SMP
extern addr_t cpu_entrypoint;
#endif

/**
 * This is the entry point for all exceptions currently managed by SO3.
 * 
 * Regarding the SOO hypercalls, all addresses got from arguments
 * *must* be physical addresses.
 *
 * @param regs	Pointer to the stack frame
 */
typedef void(*vector_fn_t)(cpu_regs_t *);

int trap_handle(cpu_regs_t *regs) {

	unsigned long esr = read_sysreg(esr_el2);
	unsigned long hvc_code;
 
 #if defined(CONFIG_AVZ) && defined (CONFIG_SOO)
        unsigned int memslotID = ((current_domain->avz_shared->domID == DOMID_AGENCY) ? MEMSLOT_AGENCY : current_domain->avz_shared->domID);
#endif

        switch (ESR_ELx_EC(esr)) {

	/* SVC used for syscalls */
	case ESR_ELx_EC_SVC64:

#ifdef CONFIG_AVZ
		/* No syscall can be issued fron the hypervisor. */
		BUG();

#else /* CONFIG_AVZ */

		local_irq_enable();
		regs->x0 = syscall_handle(regs->x0, regs->x1, regs->x2, regs->x3);
		local_irq_disable();

#endif /* !CONFIG_AVZ */

		return 0;

	case ESR_ELx_EC_HVC64:
		hvc_code = regs->x0;

                switch (hvc_code) {

#ifdef CONFIG_SMP
                /* PSCI hypercalls */
		case PSCI_0_2_FN_PSCI_VERSION:
                        return PSCI_VERSION(1, 1);

                case PSCI_0_2_FN64_CPU_ON:
                        printk("Power on CPU #%d...\n", regs->x1 & 3);

			cpu_entrypoint = regs->x2;
			smp_trigger_event(regs->x1 & 3);

			return PSCI_RET_SUCCESS;

		case PSCI_0_2_FN_MIGRATE_INFO_TYPE:
		case PSCI_1_0_FN_PSCI_FEATURES:
                        return PSCI_RET_SUCCESS;
#endif /* CONFIG_SMP */

#ifdef CONFIG_AVZ
                /* AVZ Hypercalls */
		case __HYPERVISOR_console_io:
			printk("%c", regs->x1);
			return ESUCCESS;

#ifdef CONFIG_SOO
		case __HYPERVISOR_domctl:

                        do_domctl((domctl_t *) ipa_to_va(memslotID, regs->x1));
                        flush_dcache_all();

			return ESUCCESS;

		case __HYPERVISOR_event_channel_op:

                        do_event_channel_op(regs->x1, (void *) ipa_to_va(memslotID, regs->x2));
                        flush_dcache_all();

			return ESUCCESS;

                case __HYPERVISOR_soo_hypercall:

			/* Propagate the SOO hypercall to the dedicated routines. */
                        do_soo_hypercall((soo_hyp_t *) ipa_to_va(memslotID, regs->x1));
                        flush_dcache_all();

                        return ESUCCESS;
#endif /* CONFIG_SOO */

#endif /* CONFIG_AVZ */

                default:
			return EFAULT;
                }
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
		lprintk("### On CPU %d: ESR_Elx_EC(esr): 0x%lx\n", smp_processor_id(), ESR_ELx_EC(esr));
		trap_handle_error(regs->lr);
		kernel_panic();
        }

        return -1;
}
