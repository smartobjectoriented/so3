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
#include <smp.h>
#include <mmio.h>
#include <memory.h>

#ifdef CONFIG_AVZ

#include <avz/sched.h>
#include <avz/domain.h>

#include <asm/cacheflush.h>
#include <asm/mmu.h>
#include <asm/setup.h>

#ifdef CONFIG_SOO
#include <soo/uapi/soo.h>
#endif /* CONFIG_SOO */

#else /* CONFIG_AVZ */
#include <syscall.h>
#endif /* !CONFIG_AVZ */

#include <asm/processor.h>

#ifdef CONFIG_AVZ

const char entry_error_messages[19][32] = { "SYNC_INVALID_EL2t",     "IRQ_INVALID_EL2t",    "FIQ_INVALID_EL2t",
					    "SERROR_INVALID_EL2t",   "SYNC_INVALID_EL2h",   "IRQ_INVALID_EL2h",
					    "FIQ_INVALID_EL2h",	     "SERROR_INVALID_EL2h", "SYNC_INVALID_EL1_64",
					    "IRQ_INVALID_EL1_64",    "FIQ_INVALID_EL1_64",  "SERROR_INVALID_EL1_64",
					    "SYNC_INVALID_EL1_32",   "IRQ_INVALID_EL1_32",  "FIQ_INVALID_EL1_32",
					    "SERROR_INVALID_EL1_32", "SYNC_ERROR",	    "SYSCALL_ERROR",
					    "DATA_ABORT_ERROR" };

#else

const char entry_error_messages[19][32] = { "SYNC_INVALID_EL1t",     "IRQ_INVALID_EL1t",    "FIQ_INVALID_EL1t",
					    "SERROR_INVALID_EL1t",   "SYNC_INVALID_EL1h",   "IRQ_INVALID_EL1h",
					    "FIQ_INVALID_EL1h",	     "SERROR_INVALID_EL1h", "SYNC_INVALID_EL0_64",
					    "IRQ_INVALID_EL0_64",    "FIQ_INVALID_EL0_64",  "SERROR_INVALID_EL0_64",
					    "SYNC_INVALID_EL0_32",   "IRQ_INVALID_EL0_32",  "FIQ_INVALID_EL0_32",
					    "SERROR_INVALID_EL0_32", "SYNC_ERROR",	    "SYSCALL_ERROR",
					    "DATA_ABORT_ERROR" };

#endif

void show_invalid_entry_message(u32 type, u64 esr, u64 address)
{
	printk("CPU%d: ERROR CAUGHT: ", smp_processor_id());

	/* trap_handle_error() passes the ESR exception class here, which can
	 * exceed the message table (e.g. EC 0x25, data abort without EL
	 * change) — print the raw value instead of walking off the array.
	 */

	if (type < ARRAY_SIZE(entry_error_messages))
		printk(entry_error_messages[type]);
	else
		printk("type %d", type);
	printk(", ESR: ");
	printk("%lx", esr);
	printk(", Address: ");
	printk("%lx\n", address);

	while (1)
		;
}

void trap_handle_error(addr_t lr)
{
#ifdef CONFIG_AVZ
	unsigned long esr = read_sysreg(esr_el2);
	unsigned long elr = read_sysreg(elr_el2);
	unsigned long far = read_sysreg(far_el2);
	unsigned long hpfar = read_sysreg(hpfar_el2);
	printk("CPU%d: ELR: %lx, FAR: %lx, HPFAR: %lx (IPA=0x%lx), LR(x30): %lx\n", smp_processor_id(), elr, far, hpfar,
	       hpfar << 8, lr);
#else
	unsigned long esr = read_sysreg(esr_el1);
	unsigned long elr = read_sysreg(elr_el1);
	unsigned long far = read_sysreg(far_el1);
	printk("CPU%d: ELR: %lx, FAR: %lx, LR(x30): %lx\n", smp_processor_id(), elr, far, lr);
#endif

	show_invalid_entry_message(ESR_ELx_EC(esr), esr, lr);
}

#ifdef CONFIG_SMP
extern addr_t cpu_entrypoints[4];

/* Called from pre_ret_to_el1 assembly before entering the WFI poll loop */
void pre_ret_debug_wfi(u32 cpu_id)
{
	(void) cpu_id;
}

/* Called from pre_ret_to_el1 assembly when entrypoint becomes non-zero */
void pre_ret_debug_woke(u32 cpu_id, u64 ep)
{
	(void) cpu_id;
	(void) ep;
}
#endif

#include <device/timer.h>
#include <device/arch/gic.h>
#include <asm/io.h>

#define CNTHP_PPI_INTID 26

/* Set to 0 to bring up Linux with only CPU0 — useful for isolating vGIC
 * issues before enabling full SMP.  Set to 1 for normal 4-CPU operation.
 * Used by the PSCI CPU_ON handler below (which is shared between GICv2
 * and GICv3), so it must be defined regardless of CONFIG_GIC_V3. */
#define AVZ_SMP_BOOT 1

#ifdef CONFIG_GIC_V3
/* EL2 IRQ handler for GICv3 with HCR_EL2.IMO=1.
 * All physical IRQs (Group 1 NS) are taken at EL2.  CNTHP (PPI 26) and the
 * vGIC maintenance IRQ (PPI 25) are handled locally; every other IRQ is
 * forwarded to Linux via a hardware-backed virtual LR.
 *
 * ICH_HCR_EL2.EN is enabled lazily inside gic_inject_irq() the first time a
 * virtual LR is written (mirrors Bao's vgic_add_lr pattern).  This guarantees
 * EN=0 while Linux's gic_cpu_sys_reg_init() runs so it accesses the physical
 * ICC registers directly, avoiding undefined-instruction faults on ICV_AP0Rx. */
void avz_el2_irq_handle(cpu_regs_t *regs)
{
	u64 intid = read_sysreg_s(SYS_ICC_IAR1_EL1);
	u32 id = (u32) (intid & 0x3ff);

	/* Spurious — no EOI required */
	if (id >= 1020)
		return;

	/* Linux's gic_cpu_init writes GICR_ICENABLER0 = 0xffffffff which
	 * disables ALL PPIs at the redistributor on each CPU, including
	 * AVZ's MAINT (PPI 25) and CNTHP (PPI 26).  Re-assert them on
	 * every IRQ entry — the write is idempotent and per-CPU. */
	{
		int cpu_id = smp_processor_id();
		u8 *gicr_sgi = (u8 *) gic->gicc + cpu_id * 0x20000 + 0x10000;
		iowrite32(gicr_sgi + 0x100, (1u << IRQ_ARCH_ARM_MAINT) | (1u << CNTHP_PPI_INTID));
	}

	if (id == CNTHP_PPI_INTID) {
		/* CNTHP (id=26): AVZ's own hypervisor timer — handle locally. */
		avz_el2_timer_tick();
		/* EOImode=1: priority drop, then explicit deactivate. */
		write_sysreg_s(intid, SYS_ICC_EOIR1_EL1);
		write_sysreg_s(intid, SYS_ICC_DIR_EL1);
		isb();
	} else if (id == IRQ_ARCH_ARM_MAINT) {
		/* vGIC maintenance (id=25): drain overflow queue into free LRs. */
		gic_inject_pending();
		write_sysreg_s(intid, SYS_ICC_EOIR1_EL1);
		write_sysreg_s(intid, SYS_ICC_DIR_EL1);
		isb();
	} else if (is_sgi(id)) {
		/* SGIs (0–15): software-backed. Deactivate physically, then inject
		 * as virtual SGI so Linux's IPI handler fires. */
		write_sysreg_s(intid, SYS_ICC_EOIR1_EL1);
		write_sysreg_s(intid, SYS_ICC_DIR_EL1);
		isb();
		gic_set_pending((u16) id);
	} else {
		/* PPIs (16–31, except 25/26) and SPIs (32+) destined for Linux.
		 * Drop priority at EL2 so further physical IRQs can be taken;
		 * the HW=1 LR triggers the physical deactivate when Linux later
		 * writes ICV_EOIR1_EL1 — DO NOT call DIR here, otherwise
		 * level-triggered IRQs storm before Linux clears the device's
		 * level source. */
		gic_set_pending((u16) id);
		write_sysreg_s(intid, SYS_ICC_EOIR1_EL1);
		isb();
	}
}
#endif /* CONFIG_GIC_V3 */

/**
 * @brief Handling the dabt condition
 * 
 * @param regs 
 * @param esr 
 * @return int 
 */
int dabt_handle(cpu_regs_t *regs, unsigned long esr)
{
#ifdef CONFIG_AVZ
	return mmio_dabt_decode(regs, esr);
#else
	return -1;
#endif
}

/**
 * This is the entry point for all exceptions currently managed by SO3.
 * 
 * Regarding the SOO hypercalls, all addresses got from arguments
 * *must* be physical addresses.
 *
 * @param regs	Pointer to the stack frame
 */
typedef void (*vector_fn_t)(cpu_regs_t *);

void trap_handle(cpu_regs_t *regs)
{
	int ret = 0;

#ifndef CONFIG_AVZ
	syscall_args_t sys_args;
#endif

#ifdef CONFIG_AVZ

	unsigned long esr = read_sysreg(esr_el2);
	unsigned long hvc_code;

#ifdef CONFIG_SOO
	unsigned int memslotID =
		((current_domain->avz_shared->domID == DOMID_AGENCY) ? MEMSLOT_AGENCY : current_domain->avz_shared->domID);
#else
	unsigned int memslotID = MEMSLOT_AGENCY;
#endif /* CONFIG_SOO */

#else
	unsigned long esr = read_sysreg(esr_el1);
#endif /* CONFIG_AVZ */

	switch (ESR_ELx_EC(esr)) {
	case ESR_ELx_EC_DABT_LOW:

		ret = dabt_handle(regs, esr);
		if (ret == -1)
			goto __err;
		break;

	/* SVC used for syscalls */
	case ESR_ELx_EC_SVC64:

#ifdef CONFIG_AVZ
		/* No syscall can be issued fron the hypervisor. */
		BUG();

#else /* CONFIG_AVZ */

		sys_args.args[0] = regs->x0;
		sys_args.args[1] = regs->x1;
		sys_args.args[2] = regs->x2;
		sys_args.args[3] = regs->x3;
		sys_args.args[4] = regs->x4;
		sys_args.args[5] = regs->x5;

		local_irq_enable();
		regs->x0 = syscall_handle(&sys_args);
		local_irq_disable();

#endif /* !CONFIG_AVZ */

		break;

#ifdef CONFIG_AVZ
	case ESR_ELx_EC_HVC64:
		hvc_code = regs->x0;

		switch (hvc_code) {
#ifdef CONFIG_SMP
		/* PSCI hypercalls */
		case PSCI_0_2_FN_PSCI_VERSION:
			regs->x0 = PSCI_VERSION(1, 1);
			break;

		case PSCI_0_2_FN64_CPU_ON: {
			int target_cpu = regs->x1 & 3;

#if AVZ_SMP_BOOT == 0
			regs->x0 = PSCI_RET_NOT_SUPPORTED;
			break;
#endif
			cpu_entrypoints[target_cpu] = regs->x2;
			dsb(ish);

#ifdef CONFIG_GIC_V3
			/* GICv3: targeted SGI via ICC_SGI1R_EL1 system register.
			 * ICC_SGI1R_EL1: [3:0]=SGI_ID, [23:16]=Aff1 target list (bit per Aff0). */
			{
				u64 sgi1r = ((u64) (1U << target_cpu) << 16) | IPI_EVENT_CHECK;
				write_sysreg_s(sgi1r, SYS_ICC_SGI1R_EL1);
				isb();
			}
#else
			/* GICv2: targeted SGI via GICD_SGIR MMIO.
			 * Format: [25:24]=00b (use TargetList), [23:16]=CPUTargetList
			 * (one bit per CPU), [3:0]=SGIINTID.
			 * Earlier code used (1u << 24) which is mode 01b
			 * (all-but-self) and ignores the TargetList — broadcasting
			 * IPI_EVENT_CHECK to every other CPU during PSCI_CPU_ON. */
			iowrite32(&gic->gicd->sgir, ((1u << target_cpu) << 16) | IPI_EVENT_CHECK);
			dsb(ish);
#endif

			regs->x0 = PSCI_RET_SUCCESS;
			break;
		}

		case PSCI_0_2_FN_MIGRATE_INFO_TYPE:
			/* No Trusted OS requiring migration — multiprocessor system */
			regs->x0 = PSCI_0_2_TOS_MP;
			break;

		case PSCI_1_0_FN_PSCI_FEATURES:
			/* No PSCI 1.x features implemented beyond the base set */
			regs->x0 = PSCI_RET_NOT_SUPPORTED;
			break;

		case PSCI_0_2_FN_CPU_SUSPEND:
		case PSCI_0_2_FN64_CPU_SUSPEND:
			/* Not implemented: Linux cpuidle falls back to WFI */
			regs->x0 = PSCI_RET_NOT_SUPPORTED;
			break;

		case PSCI_0_2_FN_CPU_OFF:
			regs->x0 = PSCI_RET_NOT_SUPPORTED;
			break;
#endif /* CONFIG_SMP */

		case AVZ_HYPERCALL_TRAP:
			do_avz_hypercall((avz_hyp_t *) ipa_to_va(memslotID, regs->x1));
			break;
#ifdef CONFIG_SOO
		case AVZ_HYPERCALL_SIGRETURN:
			__sigreturn();
			break;
#endif /* CONFIG_SOO */
		default:
			lprintk("[AVZ] HVC caught from EL1: x0=0x%lx ELR_EL2=0x%lx\n", hvc_code, read_sysreg(elr_el2));
			regs->x0 = PSCI_RET_NOT_SUPPORTED;
			break;
		}
		break;
#endif /* CONFIG_AVZ */

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

	case ESR_ELx_EC_SYS64: {
		/* Architectural traps that survive HSTR_EL2=0 — primarily
		 * ICC_SGI1R_EL1 writes, which the GICv3 spec mandates trap to EL2
		 * when HCR_EL2.IMO=1 so the hypervisor can route the IPI. */
		u64 elr = read_sysreg(elr_el2);
		u32 iss = (u32) (esr & 0x1ffffff);
		u32 dir = iss & 1;
		u32 crm = (iss >> 1) & 0xf;
		u32 rt = (iss >> 5) & 0x1f;
		u32 crn = (iss >> 10) & 0xf;
		u32 op1 = (iss >> 14) & 0x7;
		u32 op2 = (iss >> 17) & 0x7;
		u32 op0 = (iss >> 20) & 0x3;

		if (!dir && op0 == 3 && op1 == 0 && crn == 12 && crm == 11 && op2 == 5) {
			u64 sgi_val = (rt != 31) ? ((u64 *) regs)[rt] : 0;
			write_sysreg_s(sgi_val, SYS_ICC_SGI1R_EL1);
			isb();
		} else if (dir && rt != 31) {
			((u64 *) regs)[rt] = 0;
		}

		regs->pc = elr + 4;
		break;
	}

	case ESR_ELx_EC_IABT_LOW:
		goto __err;

	default:
__err:
		lprintk("### On CPU %d: ESR_Elx_EC(esr): 0x%lx\n", smp_processor_id(), ESR_ELx_EC(esr));
		trap_handle_error(regs->lr);
		kernel_panic();
	}
}
