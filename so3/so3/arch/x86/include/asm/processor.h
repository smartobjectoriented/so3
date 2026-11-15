/*
 * Copyright (C) 2026 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#define BIT(nr) (1UL << (nr))

/* Segment selectors, in the GDT order set up by setup.c */
#define KERNEL_CS 0x08
#define KERNEL_DS 0x10
#define USER_DS (0x18 | 3)
#define USER_CS (0x20 | 3)
#define TSS_SEL 0x28

#define X86_EFLAGS_IF BIT(9)
#define X86_EFLAGS_DF BIT(10)
#define X86_EFLAGS_TF BIT(8)
#define X86_EFLAGS_FIXED BIT(1)

#define X86_CR0_PE BIT(0)
#define X86_CR0_MP BIT(1)
#define X86_CR0_EM BIT(2)
#define X86_CR0_TS BIT(3)
#define X86_CR0_NE BIT(5)
#define X86_CR0_WP BIT(16)
#define X86_CR0_PG BIT(31)

#define X86_CR4_PAE BIT(5)
#define X86_CR4_PGE BIT(7)
#define X86_CR4_OSFXSR BIT(9)
#define X86_CR4_OSXMMEXCPT BIT(10)

#define MSR_EFER 0xc0000080
#define MSR_STAR 0xc0000081
#define MSR_LSTAR 0xc0000082
#define MSR_SFMASK 0xc0000084
#define MSR_FS_BASE 0xc0000100
#define MSR_GS_BASE 0xc0000101
#define MSR_KERNEL_GS_BASE 0xc0000102

#define EFER_SCE BIT(0)
#define EFER_LME BIT(8)
#define EFER_NXE BIT(11)

/* Vector pushed in the frame by the syscall entry, outside the IDT range */
#define X86_SYSCALL_VECTOR 0x100

/* Base vector of the 8259 PIC interrupts */
#define X86_IRQ_BASE 32

/* Size of the FXSAVE image kept on the kernel stack across __switch_to */
#define X86_FXSAVE_SIZE 512

#ifndef __ASSEMBLY__

#include <types.h>

#ifndef barrier
#define barrier() asm volatile("" : : : "memory")
#endif

#define mb() asm volatile("mfence" : : : "memory")
#define rmb() asm volatile("lfence" : : : "memory")
#define wmb() asm volatile("sfence" : : : "memory")

#define smp_mb() mb()
#define smp_rmb() barrier()
#define smp_wmb() barrier()

/* ARM barrier names used by the generic code; x86 memory is coherent. */
#define isb() barrier()
#define dsb(scope) mb()
#define dmb(scope) mb()

#define sev() barrier()
#define wfe() asm volatile("pause" : : : "memory")
#define wfi() asm volatile("hlt" : : : "memory")

/*
 * Register frame built on the kernel stack by every entry (exception, IRQ,
 * syscall). It ends with the frame pushed by the CPU, so that a frame taken
 * from user mode ends exactly at TSS.rsp0.
 *
 * The same type holds the thread context in the TCB, where only ksp and
 * fs_base are meaningful: callee-saved registers and the FPU image live on
 * the kernel stack of a switched-out thread.
 */

typedef struct cpu_regs {
	u64 ksp;
	u64 fs_base;

	u64 r15;
	u64 r14;
	u64 r13;
	u64 r12;
	u64 rbp;
	u64 rbx;
	u64 r11;
	u64 r10;
	u64 r9;
	u64 r8;
	u64 rax;
	u64 rcx;
	u64 rdx;
	u64 rsi;
	u64 rdi;

	u64 vector;
	u64 error_code;

	u64 rip;
	u64 cs;
	u64 rflags;
	u64 rsp;
	u64 ss;
} cpu_regs_t;

static inline int smp_processor_id(void)
{
	return 0;
}

static inline int irqs_disabled_flags(cpu_regs_t *regs)
{
	return !(regs->rflags & X86_EFLAGS_IF);
}

static inline void local_irq_enable(void)
{
	asm volatile("sti" : : : "memory");
}

static inline void local_irq_disable(void)
{
	asm volatile("cli" : : : "memory");
}

static inline unsigned long local_save_flags(void)
{
	unsigned long flags;

	asm volatile("pushfq; popq %0" : "=r"(flags) : : "memory");

	return flags;
}

static inline unsigned long local_irq_save(void)
{
	unsigned long flags;

	asm volatile("pushfq; popq %0; cli" : "=r"(flags) : : "memory");

	return flags;
}

static inline void local_irq_restore(unsigned long flags)
{
	asm volatile("pushq %0; popfq" : : "r"(flags) : "memory", "cc");
}

#define local_irq_is_enabled() (!!(local_save_flags() & X86_EFLAGS_IF))
#define local_irq_is_disabled() (!local_irq_is_enabled())

/* The kernel always runs in ring 0. */
#define user_mode() (0)

static inline void cpu_relax(void)
{
	asm volatile("pause" : : : "memory");
}

/* Called with IRQs on from the idle thread: hlt wakes up on the next IRQ. */

static inline void cpu_standby(void)
{
	wfi();
}

static inline u64 rdmsr(u32 msr)
{
	u32 lo, hi;

	asm volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));

	return ((u64) hi << 32) | lo;
}

static inline void wrmsr(u32 msr, u64 val)
{
	asm volatile("wrmsr" : : "c"(msr), "a"((u32) val), "d"((u32) (val >> 32)) : "memory");
}

static inline u64 read_cr2(void)
{
	u64 val;

	asm volatile("mov %%cr2, %0" : "=r"(val));

	return val;
}

static inline u64 read_cr3(void)
{
	u64 val;

	asm volatile("mov %%cr3, %0" : "=r"(val));

	return val;
}

static inline void write_cr3(u64 val)
{
	asm volatile("mov %0, %%cr3" : : "r"(val) : "memory");
}

static inline void invlpg(addr_t va)
{
	asm volatile("invlpg (%0)" : : "r"(va) : "memory");
}

static inline u64 rdtsc(void)
{
	u32 lo, hi;

	asm volatile("rdtsc" : "=a"(lo), "=d"(hi));

	return ((u64) hi << 32) | lo;
}

struct tcb;
void __switch_to(struct tcb *prev, struct tcb *next);

void cpu_do_idle(void);

/* Kernel stack top that the next entry from user mode will use */

void x86_set_kernel_stack(addr_t sp);

void x86_cpu_init(void);
void x86_fpu_init(void);
void x86_fpu_reset(void);

void trap_handle(cpu_regs_t *regs);
long syscall_entry_handle(cpu_regs_t *regs);

#endif /* __ASSEMBLY__ */

#endif /* PROCESSOR_H */
