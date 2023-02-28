/*
 * Copyright (c) 2012 Linaro Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef VIRT_H
#define VIRT_H

#include <asm/processor.h>

/*
 * The arm64 hcall implementation uses x0 to specify the hcall
 * number. A value less than HVC_STUB_HCALL_NR indicates a special
 * hcall, such as set vector. Any other value is handled in a
 * hypervisor specific way.
 *
 * The hypercall is allowed to clobber any of the caller-saved
 * registers (x0-x18), so it is advisable to use it through the
 * indirection of a function call (as implemented in hyp-stub.S).
 */

/*
 * HVC_SET_VECTORS - Set the value of the vbar_el2 register.
 *
 * @x1: Physical address of the new vector table.
 */
#define HVC_SET_VECTORS 0

/*
 * HVC_SOFT_RESTART - CPU soft reset, used by the cpu_soft_restart routine.
 */
#define HVC_SOFT_RESTART 1

/*
 * HVC_RESET_VECTORS - Restore the vectors to the original HYP stubs
 */
#define HVC_RESET_VECTORS 2

/* Max number of HYP stub hypercalls */
#define HVC_STUB_HCALL_NR 3

/* Error returned when an invalid stub number is passed into x0 */
#define HVC_STUB_ERR	0xbadca11

#define BOOT_CPU_MODE_EL1	(0xe11)
#define BOOT_CPU_MODE_EL2	(0xe12)

/* Current Exception Level values, as contained in CurrentEL */
#define CurrentEL_EL0           (0)
#define CurrentEL_EL1		(1 << 2)
#define CurrentEL_EL2		(2 << 2)
#define CurrentEL_EL3           (3 << 2)

/* Hyp Debug Configuration Register bits */
#define MDCR_EL2_TPMS		(1 << 14)
#define MDCR_EL2_E2PB_MASK	(UL(0x3))
#define MDCR_EL2_E2PB_SHIFT	(UL(12))
#define MDCR_EL2_TDRA		(1 << 11)
#define MDCR_EL2_TDOSA		(1 << 10)
#define MDCR_EL2_TDA		(1 << 9)
#define MDCR_EL2_TDE		(1 << 8)
#define MDCR_EL2_HPME		(1 << 7)
#define MDCR_EL2_TPM		(1 << 6)
#define MDCR_EL2_TPMCR		(1 << 5)
#define MDCR_EL2_HPMN_MASK	(0x1F)

/* Hyp Coprocessor Trap Register */
#define CPTR_EL2_TCPAC	(1 << 31)
#define CPTR_EL2_TTA	(1 << 20)
#define CPTR_EL2_TFP	(1 << CPTR_EL2_TFP_SHIFT)
#define CPTR_EL2_TZ		(1 << 8)
#define CPTR_EL2_RES1	0x000032ff /* known RES1 bits in CPTR_EL2 */
#define CPTR_EL2_DEFAULT	CPTR_EL2_RES1

/* Hyp Configuration Register (HCR) bits */
#define HCR_FWB		(UL(1) << 46)
#define HCR_API		(UL(1) << 41)
#define HCR_APK		(UL(1) << 40)
#define HCR_TEA		(UL(1) << 37)
#define HCR_TERR	(UL(1) << 36)
#define HCR_TLOR	(UL(1) << 35)
#define HCR_E2H		(UL(1) << 34)
#define HCR_ID		(UL(1) << 33)
#define HCR_CD		(UL(1) << 32)
#define HCR_RW_SHIFT	31
#define HCR_RW		(UL(1) << HCR_RW_SHIFT)
#define HCR_TRVM	(UL(1) << 30)
#define HCR_HCD		(UL(1) << 29)
#define HCR_TDZ		(UL(1) << 28)
#define HCR_TGE		(UL(1) << 27)
#define HCR_TVM		(UL(1) << 26)
#define HCR_TTLB	(UL(1) << 25)
#define HCR_TPU		(UL(1) << 24)
#define HCR_TPC		(UL(1) << 23)
#define HCR_TSW		(UL(1) << 22)
#define HCR_TAC		(UL(1) << 21)
#define HCR_TIDCP	(UL(1) << 20)
#define HCR_TSC		(UL(1) << 19)
#define HCR_TID3	(UL(1) << 18)
#define HCR_TID2	(UL(1) << 17)
#define HCR_TID1	(UL(1) << 16)
#define HCR_TID0	(UL(1) << 15)
#define HCR_TWE		(UL(1) << 14)
#define HCR_TWI		(UL(1) << 13)
#define HCR_DC		(UL(1) << 12)
#define HCR_BSU		(3 << 10)
#define HCR_BSU_IS	(UL(1) << 10)
#define HCR_FB		(UL(1) << 9)
#define HCR_VSE		(UL(1) << 8)
#define HCR_VI		(UL(1) << 7)
#define HCR_VF		(UL(1) << 6)
#define HCR_AMO		(UL(1) << 5)
#define HCR_IMO		(UL(1) << 4)
#define HCR_FMO		(UL(1) << 3)
#define HCR_PTW		(UL(1) << 2)
#define HCR_SWIO	(UL(1) << 1)
#define HCR_VM		(UL(1) << 0)

/*
 * The bits we set in HCR:
 * TLOR:	Trap LORegion register accesses
 * RW:		64bit by default, can be overridden for 32bit VMs
 * TAC:		Trap ACTLR
 * TSC:		Trap SMC
 * TVM:		Trap VM ops (until M+C set in SCTLR_EL1)
 * TSW:		Trap cache operations by set/way
 * TWE:		Trap WFE
 * TWI:		Trap WFI
 * TIDCP:	Trap L2CTLR/L2ECTLR
 * BSU_IS:	Upgrade barriers to the inner shareable domain
 * FB:		Force broadcast of all maintainance operations
 * AMO:		Override CPSR.A and enable signaling with VA
 * IMO:		Override CPSR.I and enable signaling with VI
 * FMO:		Override CPSR.F and enable signaling with VF
 * SWIO:	Turn set/way invalidates into set/way clean+invalidate
 */
#define HCR_AGENCY_FLAGS (HCR_VM | HCR_API | HCR_APK | HCR_RW)

#define HCR_ME_FLAGS (HCR_VM | HCR_API | HCR_APK \
		| HCR_AMO | HCR_RW \
		| HCR_FMO | HCR_IMO)

#define HCR_GUEST_FLAGS (HCR_TSC | HCR_TSW | HCR_TWE | HCR_TWI | HCR_VM | \
			 HCR_TVM | HCR_BSU_IS | HCR_FB | HCR_TAC | \
			 HCR_AMO | HCR_SWIO | HCR_TIDCP | HCR_RW | HCR_TLOR | \
			 HCR_FMO | HCR_IMO)

#define HCR_VIRT_EXCP_MASK (HCR_VSE | HCR_VI | HCR_VF)
#define HCR_HOST_NVHE_FLAGS (HCR_RW | HCR_API | HCR_APK)
#define HCR_HOST_VHE_FLAGS (HCR_RW | HCR_TGE | HCR_E2H)


#ifndef __ASSEMBLY__

#include <asm/ptrace.h>
#include <asm/sections.h>
#include <asm/sysreg.h>
#include <asm/cpufeature.h>

/*
 * __boot_cpu_mode records what mode CPUs were booted in.
 * A correctly-implemented bootloader must start all CPUs in the same mode:
 * In this case, both 32bit halves of __boot_cpu_mode will contain the
 * same value (either 0 if booted in EL1, BOOT_CPU_MODE_EL2 if booted in EL2).
 *
 * Should the bootloader fail to do this, the two values will be different.
 * This allows the kernel to flag an error when the secondaries have come up.
 */
extern u32 __boot_cpu_mode[2];

void __hyp_set_vectors(phys_addr_t phys_vector_base);
void __hyp_reset_vectors(void);

/* Reports the availability of HYP mode */
static inline bool is_hyp_mode_available(void)
{
	return (__boot_cpu_mode[0] == BOOT_CPU_MODE_EL2 &&
		__boot_cpu_mode[1] == BOOT_CPU_MODE_EL2);
}

/* Check if the bootloader has booted CPUs in different modes */
static inline bool is_hyp_mode_mismatched(void)
{
	return __boot_cpu_mode[0] != __boot_cpu_mode[1];
}

static inline bool is_kernel_in_hyp_mode(void)
{
	return read_sysreg(CurrentEL) == CurrentEL_EL2;
}

static inline bool has_vhe(void)
{
	if (cpus_have_const_cap(ARM64_HAS_VIRT_HOST_EXTN))
		return true;

	return false;
}

#endif /* __ASSEMBLY__ */

#endif /* ! VIRT_H */
