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

#ifndef __ASSEMBLY__
#include <types.h>
#endif

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
#define HVC_STUB_ERR 0xbadca11

#define BOOT_CPU_MODE_EL1 (0xe11)
#define BOOT_CPU_MODE_EL2 (0xe12)

/* Current Exception Level values, as contained in CurrentEL */
#define CurrentEL_EL0 (0)
#define CurrentEL_EL1 (1 << 2)
#define CurrentEL_EL2 (2 << 2)
#define CurrentEL_EL3 (3 << 2)

/* Hyp Debug Configuration Register bits */
#define MDCR_EL2_TPMS (1 << 14)
#define MDCR_EL2_E2PB_MASK (UL(0x3))
#define MDCR_EL2_E2PB_SHIFT (UL(12))
#define MDCR_EL2_TDRA (1 << 11)
#define MDCR_EL2_TDOSA (1 << 10)
#define MDCR_EL2_TDA (1 << 9)
#define MDCR_EL2_TDE (1 << 8)
#define MDCR_EL2_HPME (1 << 7)
#define MDCR_EL2_TPM (1 << 6)
#define MDCR_EL2_TPMCR (1 << 5)
#define MDCR_EL2_HPMN_MASK (0x1F)

/* Hyp Coprocessor Trap Register */
#define CPTR_EL2_TCPAC (1 << 31)
#define CPTR_EL2_TTA (1 << 20)
#define CPTR_EL2_TFP (1 << CPTR_EL2_TFP_SHIFT)
#define CPTR_EL2_TZ (1 << 8)
#define CPTR_EL2_RES1 0x000032ff /* known RES1 bits in CPTR_EL2 */
#define CPTR_EL2_DEFAULT CPTR_EL2_RES1

//* HCR_EL2 */
#define HCR_INITVAL 0x000000000
#define HCR_FWB_MASK _AC(0x400000000000, UL)
#define HCR_FWB_SHIFT 46
#define HCR_API_MASK _AC(0x20000000000, UL)
#define HCR_API_SHIFT 41
#define HCR_APK_MASK _AC(0x10000000000, UL)
#define HCR_APK_SHIFT 40
#define HCR_TEA_MASK _AC(0x2000000000, UL)
#define HCR_TEA_SHIFT 37
#define HCR_TERR_MASK _AC(0x1000000000, UL)
#define HCR_TERR_SHIFT 36
#define HCR_TLOR_MASK _AC(0x800000000, UL)
#define HCR_TLOR_SHIFT 35
#define HCR_E2H_MASK _AC(0x400000000, UL)
#define HCR_E2H_SHIFT 34
#define HCR_ID_MASK _AC(0x200000000, UL)
#define HCR_ID_SHIFT 33
#define HCR_CD_MASK _AC(0x100000000, UL)
#define HCR_CD_SHIFT 32
#define HCR_RW_MASK 0x080000000
#define HCR_RW_SHIFT 31
#define HCR_TRVM_MASK 0x040000000
#define HCR_TRVM_SHIFT 30
#define HCR_HCD_MASK 0x020000000
#define HCR_HCD_SHIFT 29
#define HCR_TDZ_MASK 0x010000000
#define HCR_TDZ_SHIFT 28
#define HCR_TGE_MASK 0x008000000
#define HCR_TGE_SHIFT 27
#define HCR_TVM_MASK 0x004000000
#define HCR_TVM_SHIFT 26
#define HCR_TTLB_MASK 0x002000000
#define HCR_TTLB_SHIFT 25
#define HCR_TPU_MASK 0x001000000
#define HCR_TPU_SHIFT 24
#define HCR_TPC_MASK 0x000800000
#define HCR_TPC_SHIFT 23
#define HCR_TSW_MASK 0x000400000
#define HCR_TSW_SHIFT 22
#define HCR_TACR_MASK 0x000200000
#define HCR_TACR_SHIFT 21
#define HCR_TIDCP_MASK 0x000100000
#define HCR_TIDCP_SHIFT 20
#define HCR_TSC_MASK 0x000080000
#define HCR_TSC_SHIFT 19
#define HCR_TID3_MASK 0x000040000
#define HCR_TID3_SHIFT 18
#define HCR_TID2_MASK 0x000020000
#define HCR_TID2_SHIFT 17
#define HCR_TID1_MASK 0x000010000
#define HCR_TID1_SHIFT 16
#define HCR_TID0_MASK 0x000008000
#define HCR_TID0_SHIFT 15
#define HCR_TWE_MASK 0x000004000
#define HCR_TWE_SHIFT 14
#define HCR_TWI_MASK 0x000002000
#define HCR_TWI_SHIFT 13
#define HCR_DC_MASK 0x000001000
#define HCR_DC_SHIFT 12
#define HCR_BSU_MASK 0x000000C00
#define HCR_BSU_SHIFT 10
#define HCR_FB_MASK 0x000000200
#define HCR_FB_SHIFT 9
#define HCR_VSE_MASK 0x000000100
#define HCR_VSE_SHIFT 8
#define HCR_VI_MASK 0x000000080
#define HCR_VI_SHIFT 7
#define HCR_VF_MASK 0x000000040
#define HCR_VF_SHIFT 6
#define HCR_AMO_MASK 0x000000020
#define HCR_AMO_SHIFT 5
#define HCR_IMO_MASK 0x000000010
#define HCR_IMO_SHIFT 4
#define HCR_FMO_MASK 0x000000008
#define HCR_FMO_SHIFT 3
#define HCR_PTW_MASK 0x000000004
#define HCR_PTW_SHIFT 2
#define HCR_SWIO_MASK 0x000000002
#define HCR_SWIO_SHIFT 1
#define HCR_VM_MASK 0x000000001
#define HCR_VM_SHIFT 0
#define HCR_DEFAULT_BITS \
	(HCR_AMO_MASK | HCR_IMO_MASK | HCR_FMO_MASK | HCR_VM_MASK)

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
#define HCR_AGENCY_FLAGS \
	(HCR_VM_MASK | HCR_API_MASK | HCR_APK_MASK | HCR_RW_MASK)

#define HCR_ME_FLAGS                                                \
	(HCR_VM_MASK | HCR_API_MASK | HCR_APK_MASK | HCR_AMO_MASK | \
	 HCR_RW_MASK | HCR_FMO_MASK | HCR_IMO_MASK)

#define HCR_GUEST_FLAGS                                                   \
	(HCR_TSC | HCR_TSW | HCR_TWE | HCR_TWI | HCR_VM | HCR_TVM |       \
	 HCR_BSU_IS | HCR_FB | HCR_TAC | HCR_AMO | HCR_SWIO | HCR_TIDCP | \
	 HCR_RW | HCR_TLOR | HCR_FMO | HCR_IMO)

#define HCR_VIRT_EXCP_MASK (HCR_VSE_MASK | HCR_VI_MASK | HCR_VF_MASK)
#define HCR_HOST_NVHE_FLAGS (HCR_RW_MASK | HCR_API_MASK | HCR_APK_MASK)
#define HCR_HOST_VHE_FLAGS (HCR_RW_MASK | HCR_TGE_MASK | HCR_E2H_MASK)

#endif /* ! VIRT_H */
