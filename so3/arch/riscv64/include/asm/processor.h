/*
 *  linux/include/asm-arm/processor.h
 *
 *  Copyright (C) 1995-2002 Russell King
 *  Copyright (C) 2012 Regents of the University of California
 *  Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
 *  Copyright (C) 2021 Nicolas Müller <nicolas.muller1@heig-vd.ch>
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

#ifndef __ASM_ARM_PROCESSOR_H
#define __ASM_ARM_PROCESSOR_H

#include <asm/csr.h>
#include <asm/types.h>

#define NR_CPUS 		1

#ifndef __ASSEMBLY__

#include <types.h>
#include <compiler.h>
#include <common.h>

/*
 * CPU regs matches with the stack frame layout.
 * It has to be 8 bytes aligned.
 */
typedef struct cpu_regs {
	__u64   ra;
	__u64   sp;
	__u64   gp;
	__u64   tp;
	__u64   t0;
	__u64   t1;
	__u64   t2;
	__u64   fp; // Also called s0
	__u64   s1;
	__u64   a0;
	__u64   a1;
	__u64   a2;
	__u64   a3;
	__u64   a4;
	__u64   a5;
	__u64   a6;
	__u64	a7;
	__u64	s2;
	__u64	s3;
	__u64	s4;
	__u64	s5;
	__u64	s6;
	__u64	s7;
	__u64	s8;
	__u64	s9;
	__u64	s10;
	__u64	s11;
	__u64	t3;
	__u64	t4;
	__u64	t5;
	__u64	t6;
	__u64	status;
	__u64 	epc;
} cpu_regs_t;


#define cpu_relax()	barrier()

static inline int __irqs_disabled_flags(unsigned long flags)
{
	return !(flags & SR_IE);
}

/* _MNR_ TODO regs not used yet */
static inline int irqs_disabled_flags(cpu_regs_t *regs)
{
	return !(csr_read(CSR_STATUS) & SR_IE);
}

/*
 * Enable IRQs
 */
static inline void local_irq_enable(void)
{
	csr_set(CSR_STATUS, SR_IE);
}

/*
 * Disable IRQs
 */
static inline void local_irq_disable(void)
{
	csr_clear(CSR_STATUS, SR_IE);
}

/*
 * Save the current interrupt enable state.
 */
static inline __u64 local_save_flags(void)
{
	return csr_read(CSR_STATUS);
}

static inline __u64 local_irq_save(void)
{
	return csr_read_clear(CSR_STATUS, SR_IE);
}

/*
 * restore saved IRQ & FIQ state
 */
static inline void local_irq_restore(__u64 flags)
{
	csr_set(CSR_STATUS, flags & SR_IE);
}

#define local_irq_is_enabled() \
	({!__irqs_disabled_flags(local_save_flags());\
})

#define local_irq_is_disabled() \
	(!local_irq_is_enabled())

#define nop()		__asm__ __volatile__ ("nop")

#define RISCV_FENCE(p, s) \
	__asm__ __volatile__ ("fence " #p "," #s : : : "memory")

/* These barriers need to enforce ordering on both devices or memory. */
#define mb()		RISCV_FENCE(iorw,iorw)
#define rmb()		RISCV_FENCE(ir,ir)
#define wmb()		RISCV_FENCE(ow,ow)

/* These barriers do not need to enforce ordering on devices, just memory. */
#define smp_mb()	RISCV_FENCE(rw,rw)
#define smp_rmb()	RISCV_FENCE(r,r)
#define smp_wmb()	RISCV_FENCE(w,w)

/* Current configuration on qemu is only with 1 cpu. Will for the moment always be 0 */
static inline int smp_processor_id(void) {
	return 0;
}

static inline void cpu_standby(void) {
	__asm("wfi");
}

static inline void isb(void) {
	__asm("fence.i; fence r,r");
}

#endif /* __ASSEMBLY__ */

#endif /* __ASM_ARM_PROCESSOR_H */
