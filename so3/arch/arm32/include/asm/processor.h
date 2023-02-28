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

#include <stringify.h>
#include <linkage.h>

#include <asm/cpregs.h>

#ifndef __ASM_ARM_PROCESSOR_H
#define __ASM_ARM_PROCESSOR_H

#define VECTOR_VADDR		0xffff0000

#define CPU_ARCH_UNKNOWN	0
#define CPU_ARCH_ARMv3		1
#define CPU_ARCH_ARMv4		2
#define CPU_ARCH_ARMv4T		3
#define CPU_ARCH_ARMv5		4
#define CPU_ARCH_ARMv5T		5
#define CPU_ARCH_ARMv5TE	6
#define CPU_ARCH_ARMv5TEJ	7
#define CPU_ARCH_ARMv6		8
#define CPU_ARCH_ARMv7		9

/*
 * CR1 bits (CP#15 CR1)
 */
#define CR_M	(1 << 0)	/* MMU enable				*/
#define CR_A	(1 << 1)	/* Alignment abort enable		*/
#define CR_C	(1 << 2)	/* Dcache enable			*/
#define CR_W	(1 << 3)	/* Write buffer enable			*/
#define CR_P	(1 << 4)	/* 32-bit exception handler		*/
#define CR_D	(1 << 5)	/* 32-bit data address range		*/
#define CR_L	(1 << 6)	/* Implementation defined		*/
#define CR_B	(1 << 7)	/* Big endian				*/
#define CR_S	(1 << 8)	/* System MMU protection		*/
#define CR_R	(1 << 9)	/* ROM MMU protection			*/
#define CR_F	(1 << 10)	/* Implementation defined		*/
#define CR_Z	(1 << 11)	/* Implementation defined		*/
#define CR_I	(1 << 12)	/* Icache enable			*/
#define CR_V	(1 << 13)	/* Vectors relocated to 0xffff0000	*/
#define CR_RR	(1 << 14)	/* Round Robin cache replacement	*/
#define CR_L4	(1 << 15)	/* LDR pc can set T bit			*/
#define CR_DT	(1 << 16)
#define CR_IT	(1 << 18)
#define CR_ST	(1 << 19)
#define CR_FI	(1 << 21)	/* Fast interrupt (lower latency mode)	*/
#define CR_U	(1 << 22)	/* Unaligned access operation		*/
#define CR_XP	(1 << 23)	/* Extended page tables			*/
#define CR_VE	(1 << 24)	/* Vectored interrupts			*/
#define CR_EE	(1 << 25)	/* Exception (Big) Endian		*/
#define CR_TRE	(1 << 28)	/* TEX remap enable			*/
#define CR_AFE	(1 << 29)	/* Access flag enable			*/
#define CR_TE	(1 << 30)	/* Thumb exception enable		*/

#define CPACC_FULL(n)           (3 << (n * 2))
#define CPACC_SVC(n)            (1 << (n * 2))
#define CPACC_DISABLE(n)        (0 << (n * 2))

/* CCSIDR */
#define CCSIDR_LINE_SIZE_OFFSET		0
#define CCSIDR_LINE_SIZE_MASK		0x7
#define CCSIDR_ASSOCIATIVITY_OFFSET	3
#define CCSIDR_ASSOCIATIVITY_MASK	(0x3FF << 3)
#define CCSIDR_NUM_SETS_OFFSET		13
#define CCSIDR_NUM_SETS_MASK		(0x7FFF << 13)

/*
 * The stack frame which is built at the entry in the kernel current.
 */
#define	SVC_STACK_FRAME_SIZE	(20 * 4)

/*
 * This is used to ensure the compiler did actually allocate the register we
 * asked it for some inline assembly sequences.  Apparently we can't trust
 * the compiler from one version to another so a bit of paranoia won't hurt.
 * This string is meant to be concatenated with the inline asm string and
 * will cause compilation to stop on mismatch.
 * (for details, see gcc PR 15089)
 */
#define __asmeq(x, y)  ".ifnc " x "," y " ; .err ; .endif\n\t"

/*
 * PSR bits
 */
#define PSR_USR26_MODE	0x00000000
#define PSR_FIQ26_MODE	0x00000001
#define PSR_IRQ26_MODE	0x00000002
#define PSR_SVC26_MODE	0x00000003
#define PSR_USR_MODE	0x00000010
#define PSR_FIQ_MODE	0x00000011
#define PSR_IRQ_MODE	0x00000012
#define PSR_SVC_MODE	0x00000013
#define PSR_ABT_MODE	0x00000017
#define PSR_UND_MODE	0x0000001b
#define PSR_HYP_MODE 	0x0000001a
#define PSR_SYSTEM_MODE	0x0000001f
#define PSR_MODE32_BIT	0x00000010
#define PSR_MODE_MASK	0x0000001f
#define PSR_T_BIT	0x00000020
#define PSR_F_BIT	0x00000040
#define PSR_I_BIT	0x00000080
#define PSR_A_BIT 	0x00000100
#define PSR_J_BIT	0x01000000
#define PSR_Q_BIT	0x08000000
#define PSR_V_BIT	0x10000000
#define PSR_C_BIT	0x20000000
#define PSR_Z_BIT	0x40000000
#define PSR_N_BIT	0x80000000

/*
 *
 * Groups of PSR bits
 */
#define PSR_f		0xff000000	/* Flags		*/
#define PSR_s		0x00ff0000	/* Status		*/
#define PSR_x		0x0000ff00	/* Extension		*/
#define PSR_c		0x000000ff	/* Control		*/

#define IRQMASK_REG_NAME_R "cpsr"
#define IRQMASK_REG_NAME_W "cpsr_c"

#define wfe()           asm volatile("wfe" : : : "memory")
#define wfi()           asm volatile("wfi" : : : "memory")
#define sev()           asm volatile("sev" : : : "memory")

#define ___asm_opcode_identity32(x) ((x) & 0xFFFFFFFF)

#define __inst_arm(x) ___inst_arm((___asm_opcode_identity32(x)))

#define __inst_arm32(arm_opcode, thumb_opcode) __inst_arm(arm_opcode)

#define ___inst_arm(x) .long x

#define __HVC(imm16) __inst_arm32(				\
	0xE1400070 | (((imm16) & 0xFFF0) << 4) | ((imm16) & 0x000F),	\
	0xF7E08000 | (((imm16) & 0xF000) << 4) | ((imm16) & 0x0FFF)	\
)

#define __ERET	__inst_arm32(					\
	0xE160006E,							\
	0xF3DE8F00							\
)

#define __MSR_ELR_HYP(regnum)	__inst_arm32(			\
	0xE12EF300 | regnum,						\
	0xF3808E30 | (regnum << 16)					\
)

#define __SMC(imm4) __inst_arm32(					\
	0xE1600070 | (((imm4) & 0xF) << 0),				\
	0xF7F08000 | (((imm4) & 0xF) << 16)				\
)

#define S_FRAME_SIZE	(4 * 35)

/*
 * Domain numbers
 *
 *  DOMAIN_IO     - domain 2 includes all IO only
 *  DOMAIN_KERNEL - domain 1 includes all kernel memory only
 *  DOMAIN_USER   - domain 0 includes all user memory only
 */
#define DOMAIN_USER	0
#define DOMAIN_KERNEL	1
#define DOMAIN_IO	2

/*
 * Domain types
 */
#define DOMAIN_MASK	(0x3 << 0)

#define DOMAIN_NOACCESS	0
#define DOMAIN_CLIENT	1
#define DOMAIN_MANAGER	3

#define domain_val(dom,type)	((type) << 2*(dom))

/* Layout as used in assembly, with src/dest registers mixed in */
#define __CP32(r, coproc, opc1, crn, crm, opc2) coproc, opc1, r, crn, crm, opc2
#define __CP64(r1, r2, coproc, opc, crm) coproc, opc, r1, r2, crm
#define CP32(r, name...) __CP32(r, name)
#define CP64(r, name...) __CP64(r, name)

/* Stringified for inline assembly */
#define LOAD_CP32(r, name...)  "mrc " __stringify(CP32(%r, name)) ";"
#define STORE_CP32(r, name...) "mcr " __stringify(CP32(%r, name)) ";"
#define LOAD_CP64(r, name...)  "mrrc " __stringify(CP64(%r, %H##r, name)) ";"
#define STORE_CP64(r, name...) "mcrr " __stringify(CP64(%r, %H##r, name)) ";"

/*
 * This is used to ensure the compiler did actually allocate the register we
 * asked it for some inline assembly sequences.  Apparently we can't trust
 * the compiler from one version to another so a bit of paranoia won't hurt.
 * This string is meant to be concatenated with the inline asm string and
 * will cause compilation to stop on mismatch.
 * (for details, see gcc PR 15089)
 */
#define __asmeq(x, y)  ".ifnc " x "," y " ; .err ; .endif\n\t"

/* C wrappers */
#define READ_CP32(name...) ({                                   \
    register uint32_t _r;                                       \
    asm volatile(LOAD_CP32(0, name) : "=r" (_r));               \
    _r; })

#define WRITE_CP32(v, name...) do {                             \
    register uint32_t _r = (v);                                 \
    asm volatile(STORE_CP32(0, name) : : "r" (_r));             \
} while (0)

#define READ_CP64(name...) ({                                   \
    register uint64_t _r;                                       \
    asm volatile(LOAD_CP64(0, name) : "=r" (_r));               \
    _r; })

#define WRITE_CP64(v, name...) do {                             \
    register uint64_t _r = (v);                                 \
    asm volatile(STORE_CP64(0, name) : : "r" (_r));             \
} while (0)

#ifdef __ASSEMBLY__

.irp	c,,eq,ne,cs,cc,mi,pl,vs,vc,hi,ls,ge,lt,gt,le,hs,lo
.macro	ret\c, reg

.ifeqs	"\reg", "lr"
	bx\c	\reg
.else
	mov\c	pc, \reg
.endif

.endm
.endr

.macro current_cpu reg
	mrc p15, 0, \reg, c0, c0, 5 @ read Multiprocessor ID register reg
	and \reg, \reg, #0x3 @ mask on CPU ID bits
.endm

.macro disable_irq
	cpsid	i
.endm

.macro enable_irq
	cpsie	i
.endm

/*
 * Build a return instruction for this processor type.
 */
#define RETINSTR(instr, regs...)\
        instr   regs

#define LOADREGS(cond, base, reglist...)\
        ldm##cond       base,reglist

scno    .req    r7              @ syscall number
tbl     .req    r8              @ syscall table pointer

#endif /* __ASSEMBLY__ */

#ifndef __ASSEMBLY__

#include <types.h>
#include <compiler.h>
#include <common.h>

#ifdef CONFIG_AVZ

extern char hypercall_entry[];

void cpu_on(unsigned long cpuid, addr_t entry_point);

struct vcpu_guest_context;
struct domain;

void __switch_domain_to(struct domain *prev, struct domain *next);

void ret_to_user(void);
void pre_ret_to_user(void);

#endif /* CONFIG_AVZ */

void arm_init_domains(void);
void __enable_vfp(void);

struct tcb;
void __switch_to(struct tcb *prev, struct tcb *next);

void cpu_do_idle(void);

struct fp_hard_struct {
	unsigned int save[S_FRAME_SIZE/4];		/* as yet undefined */
};

struct fp_soft_struct {
	unsigned int save[S_FRAME_SIZE/4];		/* undefined information */
};

union fp_state {
	struct fp_hard_struct	hard;
	struct fp_soft_struct	soft;
};

#define isb(option) __asm__ __volatile__ ("isb " #option : : : "memory")
#define dsb(option) __asm__ __volatile__ ("dsb " #option : : : "memory")
#define dmb(option) __asm__ __volatile__ ("dmb " #option : : : "memory")

/*
 * CPU regs matches with the stack frame layout.
 * It has to be 8 bytes aligned.
 */
typedef struct cpu_regs {
	__u32   r0;
	__u32   r1;
	__u32   r2;
	__u32   r3;
	__u32   r4;
	__u32   r5;
	__u32   r6;
	__u32   r7;
	__u32   r8;
	__u32   r9;
	__u32   r10;
	__u32   fp;
	__u32   ip;
	__u32   sp;
	__u32   lr;
	__u32   pc;
	__u32   psr;
	__u32	sp_usr;
	__u32   lr_usr;
	__u32   padding;  /* padding to keep 8-bytes alignment */
} cpu_regs_t;

void trap_handle(cpu_regs_t *regs);

#define cpu_relax()	wfe()

#define interrupts_enabled(regs) \
        (!((regs)->psr & PSR_I_BIT))

#define fast_interrupts_enabled(regs) \
	(!((regs)->psr & PSR_F_BIT))

#define processor_mode(regs) \
	((regs)->psr & PSR_MODE_MASK)

static inline int irqs_disabled_flags(cpu_regs_t *regs)
{
	return (int)((regs->psr) & PSR_I_BIT);
}

static inline int cpu_mode(void)
{
	uint32_t cpsr;

	asm volatile(
		"mrs     %0, cpsr"
		: "=r" (cpsr) : : "memory", "cc");

	return cpsr & PSR_MODE_MASK;
}

#define user_mode()     (cpu_mode() == PSR_USR_MODE)

/*
 * Enable IRQs
 */
static inline void local_irq_enable(void)
{
	/* Once we do not have pending IRQs anymore, we can enable IRQ */
	asm volatile(
		"cpsie i			@ enable IRQ"
		:
		:
		: "memory", "cc");

}

/*
 * Disable IRQs
 */
static inline void local_irq_disable(void)
{
	asm volatile(
		"cpsid i			@ disable IRQ"
		:
		:
		: "memory", "cc");
}

/*
 * Save the current interrupt enable state.
 */
static inline uint32_t local_save_flags(void)
{
	uint32_t flags;

	asm volatile(
		"mrs	%0, " IRQMASK_REG_NAME_R "	@ local_save_flags"
		: "=r" (flags) : : "memory", "cc");

	return flags;
}

static inline uint32_t local_irq_save(void)
{
	uint32_t flags;

	asm volatile(
		"mrs     %0, " IRQMASK_REG_NAME_R "      @ arch_local_irq_save\n"
		"cpsid   i"
		: "=r" (flags) : : "memory", "cc");
	return flags;
}

/*
 * restore saved IRQ & FIQ state
 */
static inline void local_irq_restore(uint32_t flags)
{
	asm volatile(
		"msr	" IRQMASK_REG_NAME_W ", %0	@ local_irq_restore"
		:
		: "r" (flags)
		: "memory", "cc");
}

#define local_irq_is_enabled() \
	({ unsigned long flags; \
	flags = local_save_flags(); \
	!(flags & PSR_I_BIT); \
})

#define local_irq_is_disabled() \
	(!local_irq_is_enabled())

#define nop() __asm__ __volatile__("mov\tr0,r0\t@ nop\n\t");

static inline unsigned int get_cr(void)
{
	unsigned int val;
	asm("mrc p15, 0, %0, c1, c0, 0	@ get CR" : "=r" (val) : : "cc");

	return val;
}

static inline void set_cr(unsigned int val)
{
	asm volatile("mcr p15, 0, %0, c1, c0, 0	@ set CR"
	  : : "r" (val) : "cc");
	isb();
}

#define mb() __asm__ __volatile__ ("" : : : "memory")
#define rmb() mb()
#define wmb() mb()
#define smp_wmb()		wmb()

#define smp_mb()		dmb()
#define smp_rmb()		rmb()
#define smp_wmb()		wmb()

#define barrier() __asm__ __volatile__("": : :"memory")

#define cpu_get_l1pgtable()	\
({						\
	unsigned long pg;			\
	__asm__("mrc	p15, 0, %0, c2, c0, 0"	\
		 : "=r" (pg) : : "cc");		\
	pg &= ~0x3fff;				\
})

static inline int smp_processor_id(void) {
	int cpu;

	/* Read Multiprocessor ID register */
	asm volatile ("mrc p15, 0, %0, c0, c0, 5": "=r" (cpu));

	/* Mask out all but CPU ID bits */
	return (cpu & 0x3);
}

static inline unsigned int get_copro_access(void)
{
	unsigned int val;
	asm("mrc p15, 0, %0, c1, c0, 2 @ get copro access"
			: "=r" (val) : : "cc");
	return val;
}

static inline void set_copro_access(unsigned int val)
{
	asm volatile("mcr p15, 0, %0, c1, c0, 2 @ set copro access"
			: : "r" (val) : "cc");
	isb();
}

static inline unsigned int get_dacr(void)
{
	unsigned int val;
	asm("mrc p15, 0, %0, c3, c0, 0	@ get DACR" : "=r" (val) : : "cc");

	return val;
}

static inline void set_dacr(unsigned int val)
{
	asm volatile("mcr p15, 0, %0, c3, c0, 0	@ set DACR"
	  : : "r" (val) : "cc");
	isb();
}

/*
 * Put the CPU in idle/standby until an interrupt is raised up.
 */
static inline void cpu_standby(void) {
	__asm("dsb");
	__asm("wfi");
}

#endif /* __ASSEMBLY__ */

#endif /* __ASM_ARM_PROCESSOR_H */
