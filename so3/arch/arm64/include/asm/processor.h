/*
 *  linux/include/asm-arm/processor.h
 *
 *  Copyright (C) 1995-2002 Russell King
 *  Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <stringify.h>
#include <linkage.h>

#include <asm/mmu.h>

#define BIT(nr)         (1UL << (nr))

#define sev()           asm volatile("sev" : : : "memory")
#define wfe()           asm volatile("wfe" : : : "memory")
#define wfi()           asm volatile("wfi" : : : "memory")

#define isb()           asm volatile("isb sy" : : : "memory")
#define dsb(scope)      asm volatile("dsb " #scope : : : "memory")
#define dmb(scope)      asm volatile("dmb " #scope : : : "memory")

#define mb()            dsb(sy)
#define rmb()           dsb(ld)

#define wmb()           dsb(st)

#define smp_mb()        dmb(ish)
#define smp_rmb()       dmb(ishld)

#define smp_wmb()       dmb(ishst)

/*
 * SCTLR_EL1/SCTLR_EL2/SCTLR_EL3 bits definitions
 */
#define CR_M		(1 << 0)	/* MMU enable			*/
#define CR_A		(1 << 1)	/* Alignment abort enable	*/
#define CR_C		(1 << 2)	/* Dcache enable		*/
#define CR_SA		(1 << 3)	/* Stack Alignment Check Enable	*/
#define CR_I		(1 << 12)	/* Icache enable		*/
#define CR_WXN		(1 << 19)	/* Write Permision Imply XN	*/
#define CR_EE		(1 << 25)	/* Exception (Big) Endian	*/

/*
 * ARMv8 ARM reserves the following encoding for system registers:
 * (Ref: ARMv8 ARM, Section: "System instruction class encoding overview",
 *  C5.2, version:ARM DDI 0487A.f)
 *	[20-19] : Op0
 *	[18-16] : Op1
 *	[15-12] : CRn
 *	[11-8]  : CRm
 *	[7-5]   : Op2
 */
#define Op0_shift	19
#define Op0_mask	0x3
#define Op1_shift	16
#define Op1_mask	0x7
#define CRn_shift	12
#define CRn_mask	0xf
#define CRm_shift	8
#define CRm_mask	0xf
#define Op2_shift	5
#define Op2_mask	0x7

#define sys_reg(op0, op1, crn, crm, op2) \
	(((op0) << Op0_shift) | ((op1) << Op1_shift) | \
	 ((crn) << CRn_shift) | ((crm) << CRm_shift) | \
	 ((op2) << Op2_shift))

#define sys_insn	sys_reg

#define sys_reg_Op0(id)	(((id) >> Op0_shift) & Op0_mask)
#define sys_reg_Op1(id)	(((id) >> Op1_shift) & Op1_mask)
#define sys_reg_CRn(id)	(((id) >> CRn_shift) & CRn_mask)
#define sys_reg_CRm(id)	(((id) >> CRm_shift) & CRm_mask)
#define sys_reg_Op2(id)	(((id) >> Op2_shift) & Op2_mask)

#ifdef __ASSEMBLY__
// The space separator is omitted so that __emit_inst(x) can be parsed as
// either an assembler directive or an assembler macro argument.
#define __emit_inst(x)			.inst(x)
#else
#define __emit_inst(x)			".inst " __stringify((x)) "\n\t"
#endif

#define ESR_ELx_EC_UNKNOWN	(0x00)
#define ESR_ELx_EC_WFx		(0x01)
/* Unallocated EC: 0x02 */
#define ESR_ELx_EC_CP15_32	(0x03)
#define ESR_ELx_EC_CP15_64	(0x04)
#define ESR_ELx_EC_CP14_MR	(0x05)
#define ESR_ELx_EC_CP14_LS	(0x06)
#define ESR_ELx_EC_FP_ASIMD	(0x07)
#define ESR_ELx_EC_CP10_ID	(0x08)	/* EL2 only */
#define ESR_ELx_EC_PAC		(0x09)	/* EL2 and above */
/* Unallocated EC: 0x0A - 0x0B */
#define ESR_ELx_EC_CP14_64	(0x0C)
#define ESR_ELx_EC_BTI		(0x0D)
#define ESR_ELx_EC_ILL		(0x0E)
/* Unallocated EC: 0x0F - 0x10 */
#define ESR_ELx_EC_SVC32	(0x11)
#define ESR_ELx_EC_HVC32	(0x12)	/* EL2 only */
#define ESR_ELx_EC_SMC32	(0x13)	/* EL2 and above */
/* Unallocated EC: 0x14 */
#define ESR_ELx_EC_SVC64	(0x15)
#define ESR_ELx_EC_HVC64	(0x16)	/* EL2 and above */
#define ESR_ELx_EC_SMC64	(0x17)	/* EL2 and above */
#define ESR_ELx_EC_SYS64	(0x18)
#define ESR_ELx_EC_SVE		(0x19)
#define ESR_ELx_EC_ERET		(0x1a)	/* EL2 only */
/* Unallocated EC: 0x1B */
#define ESR_ELx_EC_FPAC		(0x1C)	/* EL1 and above */
/* Unallocated EC: 0x1D - 0x1E */
#define ESR_ELx_EC_IMP_DEF	(0x1f)	/* EL3 only */
#define ESR_ELx_EC_IABT_LOW	(0x20)
#define ESR_ELx_EC_IABT_CUR	(0x21)
#define ESR_ELx_EC_PC_ALIGN	(0x22)
/* Unallocated EC: 0x23 */
#define ESR_ELx_EC_DABT_LOW	(0x24)
#define ESR_ELx_EC_DABT_CUR	(0x25)
#define ESR_ELx_EC_SP_ALIGN	(0x26)
/* Unallocated EC: 0x27 */
#define ESR_ELx_EC_FP_EXC32	(0x28)
/* Unallocated EC: 0x29 - 0x2B */
#define ESR_ELx_EC_FP_EXC64	(0x2C)
/* Unallocated EC: 0x2D - 0x2E */
#define ESR_ELx_EC_SERROR	(0x2F)
#define ESR_ELx_EC_BREAKPT_LOW	(0x30)
#define ESR_ELx_EC_BREAKPT_CUR	(0x31)
#define ESR_ELx_EC_SOFTSTP_LOW	(0x32)
#define ESR_ELx_EC_SOFTSTP_CUR	(0x33)
#define ESR_ELx_EC_WATCHPT_LOW	(0x34)
#define ESR_ELx_EC_WATCHPT_CUR	(0x35)
/* Unallocated EC: 0x36 - 0x37 */
#define ESR_ELx_EC_BKPT32	(0x38)
/* Unallocated EC: 0x39 */
#define ESR_ELx_EC_VECTOR32	(0x3A)	/* EL2 only */
/* Unallocated EC: 0x3B */
#define ESR_ELx_EC_BRK64	(0x3C)
/* Unallocated EC: 0x3D - 0x3F */
#define ESR_ELx_EC_MAX		(0x3F)

#define ESR_ELx_EC_SHIFT	(26)
#define ESR_ELx_EC_MASK		(UL(0x3F) << ESR_ELx_EC_SHIFT)
#define ESR_ELx_EC(esr)		(((esr) & ESR_ELx_EC_MASK) >> ESR_ELx_EC_SHIFT)
/*
 * PSR bits
 */
#define PSR_MODE_EL0t	0x00000000
#define PSR_MODE_EL1t	0x00000004
#define PSR_MODE_EL1h	0x00000005
#define PSR_MODE_EL2t	0x00000008
#define PSR_MODE_EL2h	0x00000009
#define PSR_MODE_EL3t	0x0000000c
#define PSR_MODE_EL3h	0x0000000d
#define PSR_MODE_MASK	0x0000000f

/* AArch64 SPSR bits */
#define PSR_F_BIT	0x00000040
#define PSR_I_BIT	0x00000080
#define PSR_A_BIT	0x00000100
#define PSR_D_BIT	0x00000200
#define PSR_SSBS_BIT	0x00001000
#define PSR_PAN_BIT	0x00400000
#define PSR_UAO_BIT	0x00800000
#define PSR_DIT_BIT	0x01000000
#define PSR_V_BIT	0x10000000
#define PSR_C_BIT	0x20000000
#define PSR_Z_BIT	0x40000000
#define PSR_N_BIT	0x80000000

/*
 * Instructions for modifying PSTATE fields.
 * As per Arm ARM for v8-A, Section "C.5.1.3 op0 == 0b00, architectural hints,
 * barriers and CLREX, and PSTATE access", ARM DDI 0487 C.a, system instructions
 * for accessing PSTATE fields have the following encoding:
 *	Op0 = 0, CRn = 4
 *	Op1, Op2 encodes the PSTATE field modified and defines the constraints.
 *	CRm = Imm4 for the instruction.
 *	Rt = 0x1f
 */
#define pstate_field(op1, op2)		((op1) << Op1_shift | (op2) << Op2_shift)
#define PSTATE_Imm_shift		CRm_shift

#define PSTATE_PAN			pstate_field(0, 4)
#define PSTATE_UAO			pstate_field(0, 3)
#define PSTATE_SSBS			pstate_field(3, 1)

#define SET_PSTATE_PAN(x)		__emit_inst(0xd500401f | PSTATE_PAN | ((!!x) << PSTATE_Imm_shift))
#define SET_PSTATE_UAO(x)		__emit_inst(0xd500401f | PSTATE_UAO | ((!!x) << PSTATE_Imm_shift))
#define SET_PSTATE_SSBS(x)		__emit_inst(0xd500401f | PSTATE_SSBS | ((!!x) << PSTATE_Imm_shift))

#define __SYS_BARRIER_INSN(CRm, op2, Rt) \
	__emit_inst(0xd5000000 | sys_insn(0, 3, 3, (CRm), (op2)) | ((Rt) & 0x1f))

#define SB_BARRIER_INSN			__SYS_BARRIER_INSN(0, 7, 31)

#define SYS_DC_ISW			sys_insn(1, 0, 7, 6, 2)
#define SYS_DC_CSW			sys_insn(1, 0, 7, 10, 2)
#define SYS_DC_CISW			sys_insn(1, 0, 7, 14, 2)

#define SYS_OSDTRRX_EL1			sys_reg(2, 0, 0, 0, 2)
#define SYS_MDCCINT_EL1			sys_reg(2, 0, 0, 2, 0)
#define SYS_MDSCR_EL1			sys_reg(2, 0, 0, 2, 2)
#define SYS_OSDTRTX_EL1			sys_reg(2, 0, 0, 3, 2)
#define SYS_OSECCR_EL1			sys_reg(2, 0, 0, 6, 2)
#define SYS_DBGBVRn_EL1(n)		sys_reg(2, 0, 0, n, 4)
#define SYS_DBGBCRn_EL1(n)		sys_reg(2, 0, 0, n, 5)
#define SYS_DBGWVRn_EL1(n)		sys_reg(2, 0, 0, n, 6)
#define SYS_DBGWCRn_EL1(n)		sys_reg(2, 0, 0, n, 7)
#define SYS_MDRAR_EL1			sys_reg(2, 0, 1, 0, 0)
#define SYS_OSLAR_EL1			sys_reg(2, 0, 1, 0, 4)
#define SYS_OSLSR_EL1			sys_reg(2, 0, 1, 1, 4)
#define SYS_OSDLR_EL1			sys_reg(2, 0, 1, 3, 4)
#define SYS_DBGPRCR_EL1			sys_reg(2, 0, 1, 4, 4)
#define SYS_DBGCLAIMSET_EL1		sys_reg(2, 0, 7, 8, 6)
#define SYS_DBGCLAIMCLR_EL1		sys_reg(2, 0, 7, 9, 6)
#define SYS_DBGAUTHSTATUS_EL1		sys_reg(2, 0, 7, 14, 6)
#define SYS_MDCCSR_EL0			sys_reg(2, 3, 0, 1, 0)
#define SYS_DBGDTR_EL0			sys_reg(2, 3, 0, 4, 0)
#define SYS_DBGDTRRX_EL0		sys_reg(2, 3, 0, 5, 0)
#define SYS_DBGDTRTX_EL0		sys_reg(2, 3, 0, 5, 0)
#define SYS_DBGVCR32_EL2		sys_reg(2, 4, 0, 7, 0)

#define SYS_MIDR_EL1			sys_reg(3, 0, 0, 0, 0)
#define SYS_MPIDR_EL1			sys_reg(3, 0, 0, 0, 5)
#define SYS_REVIDR_EL1			sys_reg(3, 0, 0, 0, 6)

#define SYS_ID_PFR0_EL1			sys_reg(3, 0, 0, 1, 0)
#define SYS_ID_PFR1_EL1			sys_reg(3, 0, 0, 1, 1)
#define SYS_ID_DFR0_EL1			sys_reg(3, 0, 0, 1, 2)
#define SYS_ID_AFR0_EL1			sys_reg(3, 0, 0, 1, 3)
#define SYS_ID_MMFR0_EL1		sys_reg(3, 0, 0, 1, 4)
#define SYS_ID_MMFR1_EL1		sys_reg(3, 0, 0, 1, 5)
#define SYS_ID_MMFR2_EL1		sys_reg(3, 0, 0, 1, 6)
#define SYS_ID_MMFR3_EL1		sys_reg(3, 0, 0, 1, 7)

#define SYS_ID_ISAR0_EL1		sys_reg(3, 0, 0, 2, 0)
#define SYS_ID_ISAR1_EL1		sys_reg(3, 0, 0, 2, 1)
#define SYS_ID_ISAR2_EL1		sys_reg(3, 0, 0, 2, 2)
#define SYS_ID_ISAR3_EL1		sys_reg(3, 0, 0, 2, 3)
#define SYS_ID_ISAR4_EL1		sys_reg(3, 0, 0, 2, 4)
#define SYS_ID_ISAR5_EL1		sys_reg(3, 0, 0, 2, 5)
#define SYS_ID_MMFR4_EL1		sys_reg(3, 0, 0, 2, 6)

#define SYS_MVFR0_EL1			sys_reg(3, 0, 0, 3, 0)
#define SYS_MVFR1_EL1			sys_reg(3, 0, 0, 3, 1)
#define SYS_MVFR2_EL1			sys_reg(3, 0, 0, 3, 2)

#define SYS_ID_AA64PFR0_EL1		sys_reg(3, 0, 0, 4, 0)
#define SYS_ID_AA64PFR1_EL1		sys_reg(3, 0, 0, 4, 1)
#define SYS_ID_AA64ZFR0_EL1		sys_reg(3, 0, 0, 4, 4)

#define SYS_ID_AA64DFR0_EL1		sys_reg(3, 0, 0, 5, 0)
#define SYS_ID_AA64DFR1_EL1		sys_reg(3, 0, 0, 5, 1)

#define SYS_ID_AA64AFR0_EL1		sys_reg(3, 0, 0, 5, 4)
#define SYS_ID_AA64AFR1_EL1		sys_reg(3, 0, 0, 5, 5)

#define SYS_ID_AA64ISAR0_EL1		sys_reg(3, 0, 0, 6, 0)
#define SYS_ID_AA64ISAR1_EL1		sys_reg(3, 0, 0, 6, 1)

#define SYS_ID_AA64MMFR0_EL1		sys_reg(3, 0, 0, 7, 0)
#define SYS_ID_AA64MMFR1_EL1		sys_reg(3, 0, 0, 7, 1)
#define SYS_ID_AA64MMFR2_EL1		sys_reg(3, 0, 0, 7, 2)

#define SYS_SCTLR_EL1			sys_reg(3, 0, 1, 0, 0)
#define SYS_ACTLR_EL1			sys_reg(3, 0, 1, 0, 1)
#define SYS_CPACR_EL1			sys_reg(3, 0, 1, 0, 2)

#define SYS_ZCR_EL1			sys_reg(3, 0, 1, 2, 0)

#define SYS_TTBR0_EL1			sys_reg(3, 0, 2, 0, 0)
#define SYS_TTBR1_EL1			sys_reg(3, 0, 2, 0, 1)
#define SYS_TCR_EL1			sys_reg(3, 0, 2, 0, 2)

#define SYS_APIAKEYLO_EL1		sys_reg(3, 0, 2, 1, 0)
#define SYS_APIAKEYHI_EL1		sys_reg(3, 0, 2, 1, 1)
#define SYS_APIBKEYLO_EL1		sys_reg(3, 0, 2, 1, 2)
#define SYS_APIBKEYHI_EL1		sys_reg(3, 0, 2, 1, 3)

#define SYS_APDAKEYLO_EL1		sys_reg(3, 0, 2, 2, 0)
#define SYS_APDAKEYHI_EL1		sys_reg(3, 0, 2, 2, 1)
#define SYS_APDBKEYLO_EL1		sys_reg(3, 0, 2, 2, 2)
#define SYS_APDBKEYHI_EL1		sys_reg(3, 0, 2, 2, 3)

#define SYS_APGAKEYLO_EL1		sys_reg(3, 0, 2, 3, 0)
#define SYS_APGAKEYHI_EL1		sys_reg(3, 0, 2, 3, 1)

#define SYS_SPSR_EL1			sys_reg(3, 0, 4, 0, 0)
#define SYS_ELR_EL1			sys_reg(3, 0, 4, 0, 1)

#define SYS_ICC_PMR_EL1			sys_reg(3, 0, 4, 6, 0)

#define SYS_AFSR0_EL1			sys_reg(3, 0, 5, 1, 0)
#define SYS_AFSR1_EL1			sys_reg(3, 0, 5, 1, 1)
#define SYS_ESR_EL1			sys_reg(3, 0, 5, 2, 0)

#define SYS_ERRIDR_EL1			sys_reg(3, 0, 5, 3, 0)
#define SYS_ERRSELR_EL1			sys_reg(3, 0, 5, 3, 1)
#define SYS_ERXFR_EL1			sys_reg(3, 0, 5, 4, 0)
#define SYS_ERXCTLR_EL1			sys_reg(3, 0, 5, 4, 1)
#define SYS_ERXSTATUS_EL1		sys_reg(3, 0, 5, 4, 2)
#define SYS_ERXADDR_EL1			sys_reg(3, 0, 5, 4, 3)
#define SYS_ERXMISC0_EL1		sys_reg(3, 0, 5, 5, 0)
#define SYS_ERXMISC1_EL1		sys_reg(3, 0, 5, 5, 1)

#define SYS_FAR_EL1			sys_reg(3, 0, 6, 0, 0)
#define SYS_PAR_EL1			sys_reg(3, 0, 7, 4, 0)

#define SYS_PAR_EL1_F			BIT(0)
#define SYS_PAR_EL1_FST			GENMASK(6, 1)

/*** Statistical Profiling Extension ***/
/* ID registers */
#define SYS_PMSIDR_EL1			sys_reg(3, 0, 9, 9, 7)
#define SYS_PMSIDR_EL1_FE_SHIFT		0
#define SYS_PMSIDR_EL1_FT_SHIFT		1
#define SYS_PMSIDR_EL1_FL_SHIFT		2
#define SYS_PMSIDR_EL1_ARCHINST_SHIFT	3
#define SYS_PMSIDR_EL1_LDS_SHIFT	4
#define SYS_PMSIDR_EL1_ERND_SHIFT	5
#define SYS_PMSIDR_EL1_INTERVAL_SHIFT	8
#define SYS_PMSIDR_EL1_INTERVAL_MASK	0xfUL
#define SYS_PMSIDR_EL1_MAXSIZE_SHIFT	12
#define SYS_PMSIDR_EL1_MAXSIZE_MASK	0xfUL
#define SYS_PMSIDR_EL1_COUNTSIZE_SHIFT	16
#define SYS_PMSIDR_EL1_COUNTSIZE_MASK	0xfUL

#define SYS_PMBIDR_EL1			sys_reg(3, 0, 9, 10, 7)
#define SYS_PMBIDR_EL1_ALIGN_SHIFT	0
#define SYS_PMBIDR_EL1_ALIGN_MASK	0xfU
#define SYS_PMBIDR_EL1_P_SHIFT		4
#define SYS_PMBIDR_EL1_F_SHIFT		5

/* Sampling controls */
#define SYS_PMSCR_EL1			sys_reg(3, 0, 9, 9, 0)
#define SYS_PMSCR_EL1_E0SPE_SHIFT	0
#define SYS_PMSCR_EL1_E1SPE_SHIFT	1
#define SYS_PMSCR_EL1_CX_SHIFT		3
#define SYS_PMSCR_EL1_PA_SHIFT		4
#define SYS_PMSCR_EL1_TS_SHIFT		5
#define SYS_PMSCR_EL1_PCT_SHIFT		6

#define SYS_PMSCR_EL2			sys_reg(3, 4, 9, 9, 0)
#define SYS_PMSCR_EL2_E0HSPE_SHIFT	0
#define SYS_PMSCR_EL2_E2SPE_SHIFT	1
#define SYS_PMSCR_EL2_CX_SHIFT		3
#define SYS_PMSCR_EL2_PA_SHIFT		4
#define SYS_PMSCR_EL2_TS_SHIFT		5
#define SYS_PMSCR_EL2_PCT_SHIFT		6

#define SYS_PMSICR_EL1			sys_reg(3, 0, 9, 9, 2)

#define SYS_PMSIRR_EL1			sys_reg(3, 0, 9, 9, 3)
#define SYS_PMSIRR_EL1_RND_SHIFT	0
#define SYS_PMSIRR_EL1_INTERVAL_SHIFT	8
#define SYS_PMSIRR_EL1_INTERVAL_MASK	0xffffffUL

/* Filtering controls */
#define SYS_PMSFCR_EL1			sys_reg(3, 0, 9, 9, 4)
#define SYS_PMSFCR_EL1_FE_SHIFT		0
#define SYS_PMSFCR_EL1_FT_SHIFT		1
#define SYS_PMSFCR_EL1_FL_SHIFT		2
#define SYS_PMSFCR_EL1_B_SHIFT		16
#define SYS_PMSFCR_EL1_LD_SHIFT		17
#define SYS_PMSFCR_EL1_ST_SHIFT		18

#define SYS_PMSEVFR_EL1			sys_reg(3, 0, 9, 9, 5)
#define SYS_PMSEVFR_EL1_RES0		0x0000ffff00ff0f55UL

#define SYS_PMSLATFR_EL1		sys_reg(3, 0, 9, 9, 6)
#define SYS_PMSLATFR_EL1_MINLAT_SHIFT	0

/* Buffer controls */
#define SYS_PMBLIMITR_EL1		sys_reg(3, 0, 9, 10, 0)
#define SYS_PMBLIMITR_EL1_E_SHIFT	0
#define SYS_PMBLIMITR_EL1_FM_SHIFT	1
#define SYS_PMBLIMITR_EL1_FM_MASK	0x3UL
#define SYS_PMBLIMITR_EL1_FM_STOP_IRQ	(0 << SYS_PMBLIMITR_EL1_FM_SHIFT)

#define SYS_PMBPTR_EL1			sys_reg(3, 0, 9, 10, 1)

/* Buffer error reporting */
#define SYS_PMBSR_EL1			sys_reg(3, 0, 9, 10, 3)
#define SYS_PMBSR_EL1_COLL_SHIFT	16
#define SYS_PMBSR_EL1_S_SHIFT		17
#define SYS_PMBSR_EL1_EA_SHIFT		18
#define SYS_PMBSR_EL1_DL_SHIFT		19
#define SYS_PMBSR_EL1_EC_SHIFT		26
#define SYS_PMBSR_EL1_EC_MASK		0x3fUL

#define SYS_PMBSR_EL1_EC_BUF		(0x0UL << SYS_PMBSR_EL1_EC_SHIFT)
#define SYS_PMBSR_EL1_EC_FAULT_S1	(0x24UL << SYS_PMBSR_EL1_EC_SHIFT)
#define SYS_PMBSR_EL1_EC_FAULT_S2	(0x25UL << SYS_PMBSR_EL1_EC_SHIFT)

#define SYS_PMBSR_EL1_FAULT_FSC_SHIFT	0
#define SYS_PMBSR_EL1_FAULT_FSC_MASK	0x3fUL

#define SYS_PMBSR_EL1_BUF_BSC_SHIFT	0
#define SYS_PMBSR_EL1_BUF_BSC_MASK	0x3fUL

#define SYS_PMBSR_EL1_BUF_BSC_FULL	(0x1UL << SYS_PMBSR_EL1_BUF_BSC_SHIFT)

/*** End of Statistical Profiling Extension ***/

#define SYS_PMINTENSET_EL1		sys_reg(3, 0, 9, 14, 1)
#define SYS_PMINTENCLR_EL1		sys_reg(3, 0, 9, 14, 2)

#define SYS_MAIR_EL1			sys_reg(3, 0, 10, 2, 0)
#define SYS_AMAIR_EL1			sys_reg(3, 0, 10, 3, 0)

#define SYS_LORSA_EL1			sys_reg(3, 0, 10, 4, 0)
#define SYS_LOREA_EL1			sys_reg(3, 0, 10, 4, 1)
#define SYS_LORN_EL1			sys_reg(3, 0, 10, 4, 2)
#define SYS_LORC_EL1			sys_reg(3, 0, 10, 4, 3)
#define SYS_LORID_EL1			sys_reg(3, 0, 10, 4, 7)

#define SYS_VBAR_EL1			sys_reg(3, 0, 12, 0, 0)
#define SYS_DISR_EL1			sys_reg(3, 0, 12, 1, 1)

#define SYS_ICC_IAR0_EL1		sys_reg(3, 0, 12, 8, 0)
#define SYS_ICC_EOIR0_EL1		sys_reg(3, 0, 12, 8, 1)
#define SYS_ICC_HPPIR0_EL1		sys_reg(3, 0, 12, 8, 2)
#define SYS_ICC_BPR0_EL1		sys_reg(3, 0, 12, 8, 3)
#define SYS_ICC_AP0Rn_EL1(n)		sys_reg(3, 0, 12, 8, 4 | n)
#define SYS_ICC_AP0R0_EL1		SYS_ICC_AP0Rn_EL1(0)
#define SYS_ICC_AP0R1_EL1		SYS_ICC_AP0Rn_EL1(1)
#define SYS_ICC_AP0R2_EL1		SYS_ICC_AP0Rn_EL1(2)
#define SYS_ICC_AP0R3_EL1		SYS_ICC_AP0Rn_EL1(3)
#define SYS_ICC_AP1Rn_EL1(n)		sys_reg(3, 0, 12, 9, n)
#define SYS_ICC_AP1R0_EL1		SYS_ICC_AP1Rn_EL1(0)
#define SYS_ICC_AP1R1_EL1		SYS_ICC_AP1Rn_EL1(1)
#define SYS_ICC_AP1R2_EL1		SYS_ICC_AP1Rn_EL1(2)
#define SYS_ICC_AP1R3_EL1		SYS_ICC_AP1Rn_EL1(3)
#define SYS_ICC_DIR_EL1			sys_reg(3, 0, 12, 11, 1)
#define SYS_ICC_RPR_EL1			sys_reg(3, 0, 12, 11, 3)
#define SYS_ICC_SGI1R_EL1		sys_reg(3, 0, 12, 11, 5)
#define SYS_ICC_ASGI1R_EL1		sys_reg(3, 0, 12, 11, 6)
#define SYS_ICC_SGI0R_EL1		sys_reg(3, 0, 12, 11, 7)
#define SYS_ICC_IAR1_EL1		sys_reg(3, 0, 12, 12, 0)
#define SYS_ICC_EOIR1_EL1		sys_reg(3, 0, 12, 12, 1)
#define SYS_ICC_HPPIR1_EL1		sys_reg(3, 0, 12, 12, 2)
#define SYS_ICC_BPR1_EL1		sys_reg(3, 0, 12, 12, 3)
#define SYS_ICC_CTLR_EL1		sys_reg(3, 0, 12, 12, 4)
#define SYS_ICC_SRE_EL1			sys_reg(3, 0, 12, 12, 5)
#define SYS_ICC_IGRPEN0_EL1		sys_reg(3, 0, 12, 12, 6)
#define SYS_ICC_IGRPEN1_EL1		sys_reg(3, 0, 12, 12, 7)

#define SYS_CONTEXTIDR_EL1		sys_reg(3, 0, 13, 0, 1)
#define SYS_TPIDR_EL1			sys_reg(3, 0, 13, 0, 4)

#define SYS_CNTKCTL_EL1			sys_reg(3, 0, 14, 1, 0)

#define SYS_CCSIDR_EL1			sys_reg(3, 1, 0, 0, 0)
#define SYS_CLIDR_EL1			sys_reg(3, 1, 0, 0, 1)
#define SYS_AIDR_EL1			sys_reg(3, 1, 0, 0, 7)

#define SYS_CSSELR_EL1			sys_reg(3, 2, 0, 0, 0)

#define SYS_CTR_EL0			sys_reg(3, 3, 0, 0, 1)
#define SYS_DCZID_EL0			sys_reg(3, 3, 0, 0, 7)

#define SYS_PMCR_EL0			sys_reg(3, 3, 9, 12, 0)
#define SYS_PMCNTENSET_EL0		sys_reg(3, 3, 9, 12, 1)
#define SYS_PMCNTENCLR_EL0		sys_reg(3, 3, 9, 12, 2)
#define SYS_PMOVSCLR_EL0		sys_reg(3, 3, 9, 12, 3)
#define SYS_PMSWINC_EL0			sys_reg(3, 3, 9, 12, 4)
#define SYS_PMSELR_EL0			sys_reg(3, 3, 9, 12, 5)
#define SYS_PMCEID0_EL0			sys_reg(3, 3, 9, 12, 6)
#define SYS_PMCEID1_EL0			sys_reg(3, 3, 9, 12, 7)
#define SYS_PMCCNTR_EL0			sys_reg(3, 3, 9, 13, 0)
#define SYS_PMXEVTYPER_EL0		sys_reg(3, 3, 9, 13, 1)
#define SYS_PMXEVCNTR_EL0		sys_reg(3, 3, 9, 13, 2)
#define SYS_PMUSERENR_EL0		sys_reg(3, 3, 9, 14, 0)
#define SYS_PMOVSSET_EL0		sys_reg(3, 3, 9, 14, 3)

#define SYS_TPIDR_EL0			sys_reg(3, 3, 13, 0, 2)
#define SYS_TPIDRRO_EL0			sys_reg(3, 3, 13, 0, 3)

#define SYS_CNTFRQ_EL0			sys_reg(3, 3, 14, 0, 0)

#define SYS_CNTP_TVAL_EL0		sys_reg(3, 3, 14, 2, 0)
#define SYS_CNTP_CTL_EL0		sys_reg(3, 3, 14, 2, 1)
#define SYS_CNTP_CVAL_EL0		sys_reg(3, 3, 14, 2, 2)

#define SYS_CNTV_CTL_EL0		sys_reg(3, 3, 14, 3, 1)
#define SYS_CNTV_CVAL_EL0		sys_reg(3, 3, 14, 3, 2)

#define SYS_AARCH32_CNTP_TVAL		sys_reg(0, 0, 14, 2, 0)
#define SYS_AARCH32_CNTP_CTL		sys_reg(0, 0, 14, 2, 1)
#define SYS_AARCH32_CNTP_CVAL		sys_reg(0, 2, 0, 14, 0)

#define __PMEV_op2(n)			((n) & 0x7)
#define __CNTR_CRm(n)			(0x8 | (((n) >> 3) & 0x3))
#define SYS_PMEVCNTRn_EL0(n)		sys_reg(3, 3, 14, __CNTR_CRm(n), __PMEV_op2(n))
#define __TYPER_CRm(n)			(0xc | (((n) >> 3) & 0x3))
#define SYS_PMEVTYPERn_EL0(n)		sys_reg(3, 3, 14, __TYPER_CRm(n), __PMEV_op2(n))

#define SYS_PMCCFILTR_EL0		sys_reg(3, 3, 14, 15, 7)

#define SYS_ZCR_EL2			sys_reg(3, 4, 1, 2, 0)
#define SYS_DACR32_EL2			sys_reg(3, 4, 3, 0, 0)
#define SYS_SPSR_EL2			sys_reg(3, 4, 4, 0, 0)
#define SYS_ELR_EL2			sys_reg(3, 4, 4, 0, 1)
#define SYS_IFSR32_EL2			sys_reg(3, 4, 5, 0, 1)
#define SYS_ESR_EL2			sys_reg(3, 4, 5, 2, 0)
#define SYS_VSESR_EL2			sys_reg(3, 4, 5, 2, 3)
#define SYS_FPEXC32_EL2			sys_reg(3, 4, 5, 3, 0)
#define SYS_FAR_EL2			sys_reg(3, 4, 6, 0, 0)

#define SYS_VDISR_EL2			sys_reg(3, 4, 12, 1,  1)
#define __SYS__AP0Rx_EL2(x)		sys_reg(3, 4, 12, 8, x)
#define SYS_ICH_AP0R0_EL2		__SYS__AP0Rx_EL2(0)
#define SYS_ICH_AP0R1_EL2		__SYS__AP0Rx_EL2(1)
#define SYS_ICH_AP0R2_EL2		__SYS__AP0Rx_EL2(2)
#define SYS_ICH_AP0R3_EL2		__SYS__AP0Rx_EL2(3)

#define __SYS__AP1Rx_EL2(x)		sys_reg(3, 4, 12, 9, x)
#define SYS_ICH_AP1R0_EL2		__SYS__AP1Rx_EL2(0)
#define SYS_ICH_AP1R1_EL2		__SYS__AP1Rx_EL2(1)
#define SYS_ICH_AP1R2_EL2		__SYS__AP1Rx_EL2(2)
#define SYS_ICH_AP1R3_EL2		__SYS__AP1Rx_EL2(3)

#define SYS_ICH_VSEIR_EL2		sys_reg(3, 4, 12, 9, 4)
#define SYS_ICC_SRE_EL2			sys_reg(3, 4, 12, 9, 5)
#define SYS_ICH_HCR_EL2			sys_reg(3, 4, 12, 11, 0)
#define SYS_ICH_VTR_EL2			sys_reg(3, 4, 12, 11, 1)
#define SYS_ICH_MISR_EL2		sys_reg(3, 4, 12, 11, 2)
#define SYS_ICH_EISR_EL2		sys_reg(3, 4, 12, 11, 3)
#define SYS_ICH_ELRSR_EL2		sys_reg(3, 4, 12, 11, 5)
#define SYS_ICH_VMCR_EL2		sys_reg(3, 4, 12, 11, 7)

#define __SYS__LR0_EL2(x)		sys_reg(3, 4, 12, 12, x)
#define SYS_ICH_LR0_EL2			__SYS__LR0_EL2(0)
#define SYS_ICH_LR1_EL2			__SYS__LR0_EL2(1)
#define SYS_ICH_LR2_EL2			__SYS__LR0_EL2(2)
#define SYS_ICH_LR3_EL2			__SYS__LR0_EL2(3)
#define SYS_ICH_LR4_EL2			__SYS__LR0_EL2(4)
#define SYS_ICH_LR5_EL2			__SYS__LR0_EL2(5)
#define SYS_ICH_LR6_EL2			__SYS__LR0_EL2(6)
#define SYS_ICH_LR7_EL2			__SYS__LR0_EL2(7)

#define __SYS__LR8_EL2(x)		sys_reg(3, 4, 12, 13, x)
#define SYS_ICH_LR8_EL2			__SYS__LR8_EL2(0)
#define SYS_ICH_LR9_EL2			__SYS__LR8_EL2(1)
#define SYS_ICH_LR10_EL2		__SYS__LR8_EL2(2)
#define SYS_ICH_LR11_EL2		__SYS__LR8_EL2(3)
#define SYS_ICH_LR12_EL2		__SYS__LR8_EL2(4)
#define SYS_ICH_LR13_EL2		__SYS__LR8_EL2(5)
#define SYS_ICH_LR14_EL2		__SYS__LR8_EL2(6)
#define SYS_ICH_LR15_EL2		__SYS__LR8_EL2(7)

/* VHE encodings for architectural EL0/1 system registers */
#define SYS_SCTLR_EL12			sys_reg(3, 5, 1, 0, 0)
#define SYS_CPACR_EL12			sys_reg(3, 5, 1, 0, 2)
#define SYS_ZCR_EL12			sys_reg(3, 5, 1, 2, 0)
#define SYS_TTBR0_EL12			sys_reg(3, 5, 2, 0, 0)
#define SYS_TTBR1_EL12			sys_reg(3, 5, 2, 0, 1)
#define SYS_TCR_EL12			sys_reg(3, 5, 2, 0, 2)
#define SYS_SPSR_EL12			sys_reg(3, 5, 4, 0, 0)
#define SYS_ELR_EL12			sys_reg(3, 5, 4, 0, 1)
#define SYS_AFSR0_EL12			sys_reg(3, 5, 5, 1, 0)
#define SYS_AFSR1_EL12			sys_reg(3, 5, 5, 1, 1)
#define SYS_ESR_EL12			sys_reg(3, 5, 5, 2, 0)
#define SYS_FAR_EL12			sys_reg(3, 5, 6, 0, 0)
#define SYS_MAIR_EL12			sys_reg(3, 5, 10, 2, 0)
#define SYS_AMAIR_EL12			sys_reg(3, 5, 10, 3, 0)
#define SYS_VBAR_EL12			sys_reg(3, 5, 12, 0, 0)
#define SYS_CONTEXTIDR_EL12		sys_reg(3, 5, 13, 0, 1)
#define SYS_CNTKCTL_EL12		sys_reg(3, 5, 14, 1, 0)
#define SYS_CNTP_TVAL_EL02		sys_reg(3, 5, 14, 2, 0)
#define SYS_CNTP_CTL_EL02		sys_reg(3, 5, 14, 2, 1)
#define SYS_CNTP_CVAL_EL02		sys_reg(3, 5, 14, 2, 2)
#define SYS_CNTV_TVAL_EL02		sys_reg(3, 5, 14, 3, 0)
#define SYS_CNTV_CTL_EL02		sys_reg(3, 5, 14, 3, 1)
#define SYS_CNTV_CVAL_EL02		sys_reg(3, 5, 14, 3, 2)

#define SCTLR_M_BIT	(1 << 0)
#define SCTLR_A_BIT	(1 << 1)
#define SCTLR_C_BIT	(1 << 2)
#define SCTLR_SA_BIT	(1 << 3)
#define SCTLR_SA0_BIT	(1 << 4)
#define SCTLR_CP15B_BIT (1 << 5)
#define SCTLR_ITD_BIT	(1 << 7)
#define SCTLR_SED_BIT	(1 << 8)
#define SCTLR_UMA_BIT	(1 << 9)

#define SCTLR_EOS_BIT   (1 << 11)
#define SCTLR_I_BIT	(1 << 12)
#define SCTLR_DZE_BIT	(1 << 14)
#define SCTLR_UCT_BIT	(1 << 15)
#define SCTLR_nTWI	(1 << 16)
#define SCTLR_nTWE	(1 << 18)
#define SCTLR_WXN_BIT	(1 << 19)
#define SCTLR_TSCXT_BIT	(1 << 20)
#define SCTLR_EIS_BIT	(1 << 22)
#define SCTLR_E0E_BIT	(1 << 24)
#define SCTLR_EE_BIT	(1 << 25)
#define SCTLR_UCI_BIT	(1 << 26)
#define SCTLR_nTLSMD_BIT (1 << 28)
#define SCLTR_LSMAOE_BIT (1 << 29)

/* Common SCTLR_ELx flags. */
#define SCTLR_ELx_DSSBS	(BIT(44))
#define SCTLR_ELx_ENIA	(BIT(31))
#define SCTLR_ELx_ENIB	(BIT(30))
#define SCTLR_ELx_ENDA	(BIT(27))
#define SCTLR_ELx_EE    (BIT(25))
#define SCTLR_ELx_IESB	(BIT(21))
#define SCTLR_ELx_WXN	(BIT(19))
#define SCTLR_ELx_ENDB	(BIT(13))
#define SCTLR_ELx_I	(BIT(12))
#define SCTLR_ELx_SA	(BIT(3))
#define SCTLR_ELx_C	(BIT(2))
#define SCTLR_ELx_A	(BIT(1))
#define SCTLR_ELx_M	(BIT(0))

#define SCTLR_ELx_FLAGS	(SCTLR_ELx_M  | SCTLR_ELx_A | SCTLR_ELx_C | \
			 SCTLR_ELx_SA | SCTLR_ELx_I | SCTLR_ELx_IESB)

#ifdef CONFIG_ARM64VT
#define SCTLR_EL2_RES1  (SCTLR_UCI_BIT | SCTLR_nTWE | SCTLR_nTWI \
		        | SCTLR_UCT_BIT | SCTLR_DZE_BIT)
#else
/* SCTLR_EL2 specific flags. */

#define SCTLR_EL2_RES1	((BIT(4))  | (BIT(5))  | (BIT(11)) | (BIT(16)) | \
			 (BIT(18)) | (BIT(22)) | (BIT(23)) | (BIT(28)) | \
			 (BIT(29)))
#endif

#define SCTLR_EL1_RES1	((BIT(11)) | (BIT(20)) | (BIT(22)) | (BIT(28)) | (BIT(29)))

/* SCTLR_EL1 specific flags. */
#define SCTLR_EL1_UCI		(BIT(26))
#define SCTLR_EL1_E0E		(BIT(24))
#define SCTLR_EL1_SPAN		(BIT(23))
#define SCTLR_EL1_NTWE		(BIT(18))
#define SCTLR_EL1_NTWI		(BIT(16))
#define SCTLR_EL1_UCT		(BIT(15))
#define SCTLR_EL1_DZE		(BIT(14))
#define SCTLR_EL1_UMA		(BIT(9))
#define SCTLR_EL1_SED		(BIT(8))
#define SCTLR_EL1_ITD		(BIT(7))
#define SCTLR_EL1_CP15BEN	(BIT(5))
#define SCTLR_EL1_SA0		(BIT(4))

#define ENDIAN_SET_EL2		0
#define ENDIAN_SET_EL1		0

#define SCTLR_EL1_SET	(SCTLR_ELx_M    | SCTLR_ELx_C    | SCTLR_ELx_SA   |\
			 SCTLR_EL1_SA0  | SCTLR_EL1_SED  | SCTLR_ELx_I    |\
			 SCTLR_EL1_DZE  | SCTLR_EL1_UCT                   |\
			 SCTLR_EL1_NTWE | SCTLR_ELx_IESB | SCTLR_EL1_SPAN |\
			 ENDIAN_SET_EL1 | SCTLR_EL1_UCI  | SCTLR_EL1_RES1)

/* id_aa64isar0 */
#define ID_AA64ISAR0_TS_SHIFT		52
#define ID_AA64ISAR0_FHM_SHIFT		48
#define ID_AA64ISAR0_DP_SHIFT		44
#define ID_AA64ISAR0_SM4_SHIFT		40
#define ID_AA64ISAR0_SM3_SHIFT		36
#define ID_AA64ISAR0_SHA3_SHIFT		32
#define ID_AA64ISAR0_RDM_SHIFT		28
#define ID_AA64ISAR0_ATOMICS_SHIFT	20
#define ID_AA64ISAR0_CRC32_SHIFT	16
#define ID_AA64ISAR0_SHA2_SHIFT		12
#define ID_AA64ISAR0_SHA1_SHIFT		8
#define ID_AA64ISAR0_AES_SHIFT		4

/* id_aa64isar1 */
#define ID_AA64ISAR1_SB_SHIFT		36
#define ID_AA64ISAR1_FRINTTS_SHIFT	32
#define ID_AA64ISAR1_GPI_SHIFT		28
#define ID_AA64ISAR1_GPA_SHIFT		24
#define ID_AA64ISAR1_LRCPC_SHIFT	20
#define ID_AA64ISAR1_FCMA_SHIFT		16
#define ID_AA64ISAR1_JSCVT_SHIFT	12
#define ID_AA64ISAR1_API_SHIFT		8
#define ID_AA64ISAR1_APA_SHIFT		4
#define ID_AA64ISAR1_DPB_SHIFT		0

#define ID_AA64ISAR1_APA_NI		0x0
#define ID_AA64ISAR1_APA_ARCHITECTED	0x1
#define ID_AA64ISAR1_API_NI		0x0
#define ID_AA64ISAR1_API_IMP_DEF	0x1
#define ID_AA64ISAR1_GPA_NI		0x0
#define ID_AA64ISAR1_GPA_ARCHITECTED	0x1
#define ID_AA64ISAR1_GPI_NI		0x0
#define ID_AA64ISAR1_GPI_IMP_DEF	0x1

/* id_aa64pfr0 */
#define ID_AA64PFR0_CSV3_SHIFT		60
#define ID_AA64PFR0_CSV2_SHIFT		56
#define ID_AA64PFR0_DIT_SHIFT		48
#define ID_AA64PFR0_SVE_SHIFT		32
#define ID_AA64PFR0_RAS_SHIFT		28
#define ID_AA64PFR0_GIC_SHIFT		24
#define ID_AA64PFR0_ASIMD_SHIFT		20
#define ID_AA64PFR0_FP_SHIFT		16
#define ID_AA64PFR0_EL3_SHIFT		12
#define ID_AA64PFR0_EL2_SHIFT		8
#define ID_AA64PFR0_EL1_SHIFT		4
#define ID_AA64PFR0_EL0_SHIFT		0

#define ID_AA64PFR0_SVE			0x1
#define ID_AA64PFR0_RAS_V1		0x1
#define ID_AA64PFR0_FP_NI		0xf
#define ID_AA64PFR0_FP_SUPPORTED	0x0
#define ID_AA64PFR0_ASIMD_NI		0xf
#define ID_AA64PFR0_ASIMD_SUPPORTED	0x0
#define ID_AA64PFR0_EL1_64BIT_ONLY	0x1
#define ID_AA64PFR0_EL0_64BIT_ONLY	0x1
#define ID_AA64PFR0_EL0_32BIT_64BIT	0x2

/* id_aa64pfr1 */
#define ID_AA64PFR1_SSBS_SHIFT		4

#define ID_AA64PFR1_SSBS_PSTATE_NI	0
#define ID_AA64PFR1_SSBS_PSTATE_ONLY	1
#define ID_AA64PFR1_SSBS_PSTATE_INSNS	2

/* id_aa64zfr0 */
#define ID_AA64ZFR0_SM4_SHIFT		40
#define ID_AA64ZFR0_SHA3_SHIFT		32
#define ID_AA64ZFR0_BITPERM_SHIFT	16
#define ID_AA64ZFR0_AES_SHIFT		4
#define ID_AA64ZFR0_SVEVER_SHIFT	0

#define ID_AA64ZFR0_SM4			0x1
#define ID_AA64ZFR0_SHA3		0x1
#define ID_AA64ZFR0_BITPERM		0x1
#define ID_AA64ZFR0_AES			0x1
#define ID_AA64ZFR0_AES_PMULL		0x2
#define ID_AA64ZFR0_SVEVER_SVE2		0x1

/* id_aa64mmfr0 */
#define ID_AA64MMFR0_TGRAN4_SHIFT	28
#define ID_AA64MMFR0_TGRAN64_SHIFT	24
#define ID_AA64MMFR0_TGRAN16_SHIFT	20
#define ID_AA64MMFR0_BIGENDEL0_SHIFT	16
#define ID_AA64MMFR0_SNSMEM_SHIFT	12
#define ID_AA64MMFR0_BIGENDEL_SHIFT	8
#define ID_AA64MMFR0_ASID_SHIFT		4
#define ID_AA64MMFR0_PARANGE_SHIFT	0

#define ID_AA64MMFR0_TGRAN4_NI		0xf
#define ID_AA64MMFR0_TGRAN4_SUPPORTED	0x0
#define ID_AA64MMFR0_TGRAN64_NI		0xf
#define ID_AA64MMFR0_TGRAN64_SUPPORTED	0x0
#define ID_AA64MMFR0_TGRAN16_NI		0x0
#define ID_AA64MMFR0_TGRAN16_SUPPORTED	0x1
#define ID_AA64MMFR0_PARANGE_48		0x5
#define ID_AA64MMFR0_PARANGE_52		0x6

#define ID_AA64MMFR0_PARANGE_MAX	ID_AA64MMFR0_PARANGE_48

/* id_aa64mmfr1 */
#define ID_AA64MMFR1_PAN_SHIFT		20
#define ID_AA64MMFR1_LOR_SHIFT		16
#define ID_AA64MMFR1_HPD_SHIFT		12
#define ID_AA64MMFR1_VHE_SHIFT		8
#define ID_AA64MMFR1_VMIDBITS_SHIFT	4
#define ID_AA64MMFR1_HADBS_SHIFT	0

#define ID_AA64MMFR1_VMIDBITS_8		0
#define ID_AA64MMFR1_VMIDBITS_16	2

/* id_aa64mmfr2 */
#define ID_AA64MMFR2_FWB_SHIFT		40
#define ID_AA64MMFR2_AT_SHIFT		32
#define ID_AA64MMFR2_LVA_SHIFT		16
#define ID_AA64MMFR2_IESB_SHIFT		12
#define ID_AA64MMFR2_LSM_SHIFT		8
#define ID_AA64MMFR2_UAO_SHIFT		4
#define ID_AA64MMFR2_CNP_SHIFT		0

/* id_aa64dfr0 */
#define ID_AA64DFR0_PMSVER_SHIFT	32
#define ID_AA64DFR0_CTX_CMPS_SHIFT	28
#define ID_AA64DFR0_WRPS_SHIFT		20
#define ID_AA64DFR0_BRPS_SHIFT		12
#define ID_AA64DFR0_PMUVER_SHIFT	8
#define ID_AA64DFR0_TRACEVER_SHIFT	4
#define ID_AA64DFR0_DEBUGVER_SHIFT	0

#define ID_ISAR5_RDM_SHIFT		24
#define ID_ISAR5_CRC32_SHIFT		16
#define ID_ISAR5_SHA2_SHIFT		12
#define ID_ISAR5_SHA1_SHIFT		8
#define ID_ISAR5_AES_SHIFT		4
#define ID_ISAR5_SEVL_SHIFT		0

#define MVFR0_FPROUND_SHIFT		28
#define MVFR0_FPSHVEC_SHIFT		24
#define MVFR0_FPSQRT_SHIFT		20
#define MVFR0_FPDIVIDE_SHIFT		16
#define MVFR0_FPTRAP_SHIFT		12
#define MVFR0_FPDP_SHIFT		8
#define MVFR0_FPSP_SHIFT		4
#define MVFR0_SIMD_SHIFT		0

#define MVFR1_SIMDFMAC_SHIFT		28
#define MVFR1_FPHP_SHIFT		24
#define MVFR1_SIMDHP_SHIFT		20
#define MVFR1_SIMDSP_SHIFT		16
#define MVFR1_SIMDINT_SHIFT		12
#define MVFR1_SIMDLS_SHIFT		8
#define MVFR1_FPDNAN_SHIFT		4
#define MVFR1_FPFTZ_SHIFT		0


#define ID_AA64MMFR0_TGRAN4_SHIFT	28
#define ID_AA64MMFR0_TGRAN64_SHIFT	24
#define ID_AA64MMFR0_TGRAN16_SHIFT	20

#define ID_AA64MMFR0_TGRAN4_NI		0xf
#define ID_AA64MMFR0_TGRAN4_SUPPORTED	0x0
#define ID_AA64MMFR0_TGRAN64_NI		0xf
#define ID_AA64MMFR0_TGRAN64_SUPPORTED	0x0
#define ID_AA64MMFR0_TGRAN16_NI		0x0
#define ID_AA64MMFR0_TGRAN16_SUPPORTED	0x1

#define ID_AA64MMFR0_TGRAN_SHIFT	ID_AA64MMFR0_TGRAN4_SHIFT
#define ID_AA64MMFR0_TGRAN_SUPPORTED	ID_AA64MMFR0_TGRAN4_SUPPORTED

/*
 * The ZCR_ELx_LEN_* definitions intentionally include bits [8:4] which
 * are reserved by the SVE architecture for future expansion of the LEN
 * field, with compatible semantics.
 */
#define ZCR_ELx_LEN_SHIFT	0
#define ZCR_ELx_LEN_SIZE	9
#define ZCR_ELx_LEN_MASK	0x1ff

#define CPACR_EL1_ZEN_EL1EN	(BIT(16)) /* enable EL1 access */
#define CPACR_EL1_ZEN_EL0EN	(BIT(17)) /* enable EL0 access, if EL1EN set */
#define CPACR_EL1_ZEN		(CPACR_EL1_ZEN_EL1EN | CPACR_EL1_ZEN_EL0EN)


/* Safe value for MPIDR_EL1: Bit31:RES1, Bit30:U:0, Bit24:MT:0 */
#define SYS_MPIDR_SAFE_VAL	(BIT(31))

/* The stack must be 16-byte aligned */

#define S_FRAME_SIZE	(8 * 34)

#ifdef __ASSEMBLY__

.macro current_cpu reg
	mrs \reg, mpidr_el1 	// read Multiprocessor ID register reg
	and \reg, \reg, #0x3 	// mask on CPU ID bits
.endm

.macro disable_irq
	msr	daifset, #2		// arch_local_irq_disable
	nop
.endm

.macro enable_irq
	msr	daifclr, #2		// arch_local_irq_enable
	nop
.endm


	.irp	num,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30
	.equ	.L__reg_num_x\num, \num
	.endr
	.equ	.L__reg_num_xzr, 31

.macro	mrs_s, rt, sreg
	 __emit_inst(0xd5200000|(\sreg)|(.L__reg_num_\rt))
.endm

.macro	msr_s, sreg, rt
	__emit_inst(0xd5000000|(\sreg)|(.L__reg_num_\rt))
.endm

.macro kernel_entry
	sub		sp, sp, #S_FRAME_SIZE

	stp		x0, x1, [sp, #OFFSET_X0]
	stp		x2, x3, [sp, #OFFSET_X2]
	stp		x4, x5, [sp, #OFFSET_X4]
	stp		x6, x7, [sp, #OFFSET_X6]
	stp		x8, x9, [sp, #OFFSET_X8]
	stp		x10, x11, [sp, #OFFSET_X10]
	stp		x12, x13, [sp, #OFFSET_X12]
	stp		x14, x15, [sp, #OFFSET_X14]
	stp		x16, x17, [sp, #OFFSET_X16]
	stp		x18, x19, [sp, #OFFSET_X18]
	stp		x20, x21, [sp, #OFFSET_X20]
	stp		x22, x23, [sp, #OFFSET_X22]
	stp		x24, x25, [sp, #OFFSET_X24]
	stp		x26, x27, [sp, #OFFSET_X26]
	stp		x28, x29, [sp, #OFFSET_X28]
	str		lr, [sp, #OFFSET_LR]
.endm

.macro kernel_exit

	ldp		x0, x1, [sp, #OFFSET_X0]
	ldp		x2, x3, [sp, #OFFSET_X2]
	ldp		x4, x5, [sp, #OFFSET_X4]
	ldp		x6, x7, [sp, #OFFSET_X6]
	ldp		x8, x9, [sp, #OFFSET_X8]
	ldp		x10, x11, [sp, #OFFSET_X10]
	ldp		x12, x13, [sp, #OFFSET_X12]
	ldp		x14, x15, [sp, #OFFSET_X14]
	ldp		x16, x17, [sp, #OFFSET_X16]
	ldp		x18, x19, [sp, #OFFSET_X18]
	ldp		x20, x21, [sp, #OFFSET_X20]
	ldp		x22, x23, [sp, #OFFSET_X22]
	ldp		x24, x25, [sp, #OFFSET_X24]
	ldp		x26, x27, [sp, #OFFSET_X26]
	ldp		x28, x29, [sp, #OFFSET_X28]
	ldr		lr, [sp, #OFFSET_LR]

	add		sp, sp, #S_FRAME_SIZE
.endm

#else

static inline int cpu_mode(void)
{
	uint32_t el;

	asm volatile(
		"mrs     %0, CurrentEL"	: "=r" (el) : : "memory", "cc");

	return el;
}

#define user_mode()     (cpu_mode() == 0)

#define __DEFINE_MRS_MSR_S_REGNUM				\
"	.irp	num,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30\n" \
"	.equ	.L__reg_num_x\\num, \\num\n"			\
"	.endr\n"						\
"	.equ	.L__reg_num_xzr, 31\n"

#define DEFINE_MRS_S						\
	__DEFINE_MRS_MSR_S_REGNUM				\
"	.macro	mrs_s, rt, sreg\n"				\
	__emit_inst(0xd5200000|(\\sreg)|(.L__reg_num_\\rt))	\
"	.endm\n"

#define DEFINE_MSR_S						\
	__DEFINE_MRS_MSR_S_REGNUM				\
"	.macro	msr_s, sreg, rt\n"				\
	__emit_inst(0xd5000000|(\\sreg)|(.L__reg_num_\\rt))	\
"	.endm\n"

#define UNDEFINE_MRS_S						\
"	.purgem	mrs_s\n"

#define UNDEFINE_MSR_S						\
"	.purgem	msr_s\n"

#define __mrs_s(v, r)						\
	DEFINE_MRS_S						\
"	mrs_s " v ", " __stringify(r) "\n"			\
	UNDEFINE_MRS_S

#define __msr_s(r, v)						\
	DEFINE_MSR_S						\
"	msr_s " __stringify(r) ", " v "\n"			\
	UNDEFINE_MSR_S

/*
 * Unlike read_cpuid, calls to read_sysreg are never expected to be
 * optimized away or replaced with synthetic values.
 */
#define read_sysreg(r) ({					\
	u64 __val;						\
	asm volatile("mrs %0, " __stringify(r) : "=r" (__val));	\
	__val;							\
})

/*
 * The "Z" constraint normally means a zero immediate, but when combined with
 * the "%x0" template means XZR.
 */
#define write_sysreg(v, r) do {					\
	u64 __val = (u64)(v);					\
	asm volatile("msr " __stringify(r) ", %x0"		\
		     : : "rZ" (__val));				\
} while (0)

/*
 * For registers without architectural names, or simply unsupported by
 * GAS.
 */
#define read_sysreg_s(r) ({						\
	u64 __val;							\
	asm volatile(__mrs_s("%0", r) : "=r" (__val));			\
	__val;								\
})

#define write_sysreg_s(v, r) do {					\
	u64 __val = (u64)(v);						\
	asm volatile(__msr_s(r, "%x0") : : "rZ" (__val));		\
} while (0)

/*
 * Modify bits in a sysreg. Bits in the clear mask are zeroed, then bits in the
 * set mask are set. Other bits are left as-is.
 */
#define sysreg_clear_set(sysreg, clear, set) do {			\
	u64 __scs_val = read_sysreg(sysreg);				\
	u64 __scs_new = (__scs_val & ~(u64)(clear)) | (set);		\
	if (__scs_new != __scs_val)					\
		write_sysreg(__scs_new, sysreg);			\
} while (0)

/*
 * PMR values used to mask/unmask interrupts.
 *
 * GIC priority masking works as follows: if an IRQ's priority is a higher value
 * than the value held in PMR, that IRQ is masked. Lowering the value of PMR
 * means masking more IRQs (or at least that the same IRQs remain masked).
 *
 * To mask interrupts, we clear the most significant bit of PMR.
 *
 * Some code sections either automatically switch back to PSR.I or explicitly
 * require to not use priority masking. If bit GIC_PRIO_PSR_I_SET is included
 * in the  the priority mask, it indicates that PSR.I should be set and
 * interrupt disabling temporarily does not rely on IRQ priorities.
 */
#define GIC_PRIO_IRQON			0xe0
#define GIC_PRIO_IRQOFF			(GIC_PRIO_IRQON & ~0x80)
#define GIC_PRIO_PSR_I_SET		(1 << 4)

typedef struct cpu_regs {
	u64 x0;
	u64 x1;
	u64 x2;
	u64 x3;
	u64 x4;
	u64 x5;
	u64 x6;
	u64 x7;
	u64 x8;
	u64 x9;
	u64 x10;
	u64 x11;
	u64 x12;
	u64 x13;
	u64 x14;
	u64 x15;
	u64 x16;
	u64 x17;
	u64 x18;
	u64 x19;
	u64 x20;
	u64 x21;
	u64 x22;
	u64 x23;
	u64 x24;
	u64 x25;
	u64 x26;
	u64 x27;
	u64 x28;
	u64 fp;
	u64 lr;
	u64 sp;
	u64 pc;
	u64 pstate;
} cpu_regs_t;

static inline int smp_processor_id(void) {
	int cpu;

	/* Read Multiprocessor ID register */
	asm volatile ("mrs %0, mpidr_el1": "=r" (cpu));

	/* Mask out all but CPU ID bits */
	return (cpu & 0x3);
}

static inline int irqs_disabled_flags(cpu_regs_t *regs)
{
	return (int)((regs->pstate) & PSR_I_BIT);
}

static inline void local_irq_enable(void)
{
	asm volatile(
		"msr	daifclr, #2		// arch_local_irq_enable\n"
		"nop"
		::: "memory");

}

static inline void local_irq_disable(void)
{
	asm volatile(
		"msr	daifset, #2		// arch_local_irq_disable\n"
		"nop"
		::: "memory");

}

static inline unsigned long local_irq_save(void)
{
	unsigned long flags;

	asm volatile(
		"mrs	%0, daif \n"     \
		"msr 	daifset, #2"
		: "=r" (flags) : : "memory", "cc");

	return flags;
}


static inline void local_irq_restore(unsigned long flags)
{
	asm volatile(
		"msr	daif, %0"
		:: "r" (flags) : "memory", "cc");
}

/*
 * Save the current interrupt enable state.
 */
static inline unsigned long local_save_flags(void)
{
	unsigned long flags;

	asm volatile(
		"mrs	%0, daif"
		: "=r" (flags) : : "memory", "cc");

	return flags;
}


#define local_irq_is_enabled() \
	({ unsigned long flags; \
	flags = local_save_flags(); \
	!(flags & PSR_I_BIT); \
})

#define local_irq_is_disabled() \
	(!local_irq_is_enabled())

static inline void cpu_relax(void)
{
	asm volatile("yield" ::: "memory");
}

/*
 * Put the CPU in idle/standby until an interrupt is raised up.
 */
static inline void cpu_standby(void) {
	dsb(sy);
	wfi();
}

#ifdef CONFIG_AVZ

extern char hypercall_entry[];

typedef struct cpu_sys_regs {
	u64   vksp;
	u64   vusp;
} cpu_sys_regs_t;

void cpu_on(unsigned long cpuid, addr_t entry_point);

struct vcpu_guest_context;
struct domain;

void __switch_domain_to(struct domain *prev, struct domain *next);

void ret_to_user(void);
void pre_ret_to_user(void);
void pre_ret_to_el1(void);
void pre_ret_to_el1_with_spin(addr_t release_addr);

#endif /* CONFIG_AVZ */

struct tcb;
void __switch_to(struct tcb *prev, struct tcb *next);

int trap_handle(cpu_regs_t *regs);

void cpu_do_idle(void);

#endif /* __ASSEMBLY__ */


#endif /* PROCESSOR_H */
