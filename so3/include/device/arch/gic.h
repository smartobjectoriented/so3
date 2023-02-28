/*
 * Copyright (C) 2014-2023 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

/* irq.h
 * References:
 * [1] Allwinner A20 User Manual: Section 1.11 GIC
 * [2] Migrating a software application from ARMv5 to ARMv7-A/R Application Note 425
 *     http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dai0425/ch03s06s03.html
 * [3] GIC-400 Generic Interrupt Controller Technical Reference Manual
 *     http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0471b/index.html
 * [4] ARM Generic Interrupt Controller Architecture Specification v2.0
 *     http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ihi0048b/index.html
 */

#ifndef GIC_H
#define GIC_H

/* Total number of IRQ sources handled by the controller
 *   0- 15 are SGI interrupts: SGI = Software Generated Interrupts
 *  16- 31 are PPI interrupts: PPI = Private Peripheral Interrupts
 *  32-122 are SPI interrupts: SPI = Shared Peripheral Interrupts
 *  122-127 are unused but declared to fit in GIC multiple of 32 bits structures
 */

#define NR_IRQS	160

#define ICC_SRE_EL2_SRE			(1 << 0)
#define ICC_SRE_EL2_ENABLE		(1 << 3)

#define GICD_ENABLE			0x1
#define GICD_DISABLE			0x0
#define GICD_INT_ACTLOW_LVLTRIG	0x0
#define GICD_INT_EN_CLR_X32		0xffffffff
#define GICD_INT_EN_SET_SGI		0x0000ffff
#define GICD_INT_EN_CLR_PPI		0xffff0000
#define GICD_INT_DEF_PRI		0xa0
#define GICD_INT_DEF_PRI_X4		((GICD_INT_DEF_PRI << 24) |\
					 (GICD_INT_DEF_PRI << 16) |\
					 (GICD_INT_DEF_PRI << 8) |\
					 GICD_INT_DEF_PRI)

#define GIC_CPU_CTRL			0x00
#define GIC_CPU_PRIMASK			0x04
#define GIC_CPU_BINPOINT		0x08
#define GIC_CPU_INTACK			0x0c
#define GIC_CPU_EOI			0x10
#define GIC_CPU_RUNNINGPRI		0x14
#define GIC_CPU_HIGHPRI			0x18
#define GIC_CPU_ALIAS_BINPOINT		0x1c
#define GIC_CPU_ACTIVEPRIO		0xd0
#define GIC_CPU_IDENT			0xfc

#define GICC_ENABLE			0x1
#define GICC_INT_PRI_THRESHOLD		0xf0
#define GICC_IAR_INT_ID_MASK		0x3ff
#define GICC_INT_SPURIOUS		1023
#define GICC_DIS_BYPASS_MASK		0x1e0

#define INTC_CPU_CTRL_REG0		0x28
#define INTC_DISABLE			(1<<4)

#ifndef __ASSEMBLY__

#include <types.h>

typedef enum {
	GIC_IRQ_TYPE_SPI	= 0,
	GIC_IRQ_TYPE_PPI	= 1,
	GIC_IRQ_TYPE_SGI	= 2
} gic_irq_type_t;

/* Bits and regs definitions */

struct gicd_regs {

    /* Distributor Registers: [3] Table 3.2. Distributor register summary */
    volatile uint32_t ctlr;           /* 0x0000 */
    volatile uint32_t typer;          /* 0x0004 */
    volatile uint32_t iidr;           /* 0x0008 */
    volatile uint32_t _res0[29];      /* 0x000c-0x001c: reserved 5 */
                                      /* 0x0020-0x003c: implementation defined 8 */
                                      /* 0x0040-0x007c: reserved 16 */
    volatile uint32_t igroupr[32];    /* 0x0080 1024 interrupts with 1 bit/interrupt */
    volatile uint32_t isenabler[32];  /* 0x0100 */
    volatile uint32_t icenabler[32];  /* 0x0180 */
    volatile uint32_t ispendr[32];    /* 0x0200 */
    volatile uint32_t icpendr[32];    /* 0x0280 */
    volatile uint32_t isactiver[32];  /* 0x0300 */
    volatile uint32_t icactiver[32];  /* 0x0380 */

    volatile uint32_t ipriorityr[256]; /* 0x0400 1024 interrupts with 8 bits/interrupt */
    volatile uint32_t itargetsr[256];  /* 0x0800 1024 interrupts with 8 bits/interrupt */
    volatile uint32_t icfgr[64];       /* 0x0c00 1024 interrupts with 2 bits/interrupt */
    volatile uint32_t ppisr;           /* 0x0d00 */
    volatile uint32_t spisr[63];       /* 0x0d04-0x0dfc */
    volatile uint32_t nsacrn[64];      /* 0x0e00 (optional) */
    volatile uint32_t sgir;            /* 0x0f00 */
    volatile uint32_t _res1[3];        /* 0x0f04-0x0f0c */
    volatile uint32_t cpendsgirn[4];   /* 0x0f10-0x0f1c */
    volatile uint32_t spendsgirn[4];   /* 0x0f20-0x0f2c */
    volatile uint32_t _res2[40];       /* 0x0f30-0x0fcc */
    volatile uint32_t pidr4;           /* 0x0fd0 */
    volatile uint32_t pidr5;           /* 0x0fd4 */
    volatile uint32_t pidr6;           /* 0x0fd8 */
    volatile uint32_t pidr7;           /* 0x0fdc */
    volatile uint32_t pidr0;           /* 0x0fe0 */
    volatile uint32_t pidr1;           /* 0x0fe4 */
    volatile uint32_t pidr2;           /* 0x0fe8 */
    volatile uint32_t pidr3;           /* 0x0fec */
    volatile uint32_t cidr0;           /* 0x0ff0 */
    volatile uint32_t cidr1;           /* 0x0ff4 */
    volatile uint32_t cidr2;           /* 0x0ff8 */
    volatile uint32_t cidr3;           /* 0x0ffc */
};

struct gicc_regs {

    /* CPU Interface Registers: [3] Table 3.6. CPU interface register summary */
    volatile uint32_t ctlr;       /* 0x0000 */
    volatile uint32_t pmr;        /* 0x0004 */
    volatile uint32_t bpr;        /* 0x0008 */
    volatile uint32_t iar;        /* 0x000c */
    volatile uint32_t eoir;       /* 0x0010 */
    volatile uint32_t rpr;        /* 0x0014 */
    volatile uint32_t hppir;      /* 0x0018 */
    volatile uint32_t abpr;       /* 0x001c */
    volatile uint32_t aiar;       /* 0x0020 */
    volatile uint32_t aeoir;      /* 0x0024 */
    volatile uint32_t ahppir;     /* 0x0028 */

    volatile uint32_t _res3[41];  /* 0x002c-0x00cc */

    volatile uint32_t apr0;       /* 0x00d0 */
    volatile uint32_t _res4[3];   /* 0x00d4-0x00dc */
    volatile uint32_t nsapr0;     /* 0x00e0 */
    volatile uint32_t _res5[6];   /* 0x00e4-0x00f8 */
    volatile uint32_t iidr;       /* 0x00fc */
    volatile uint32_t _res6[960]; /* 0x0100 - 0x0ffc */
    volatile uint32_t dir;	  /* 0x1000 */

};

struct gich_regs {
	volatile uint32_t hcr;		/* 0x000 */
	volatile uint32_t vtr;  	/* 0x004 */
	volatile uint32_t vmcr; 	/* 0x008 */
	volatile uint32_t _res0;	/* 0x00c */
	volatile uint32_t misr;		/* 0x010 */
	volatile uint32_t _res1[3];	/* 0x14-0x1c */
	volatile uint32_t eisr0;	/* 0x020 */
	volatile uint32_t eisr1;	/* 0x024 */
	volatile uint32_t _res2[2];	/* 0x28-0x2c */
	volatile uint32_t elsr0;	/* 0x030 */
	volatile uint32_t elsr1;	/* 0x034 */
	volatile uint32_t _res3[45];	/* 0x38-0xec */
	volatile uint32_t apr;		/* 0x0f0 */
	volatile uint32_t _res4[3];	/* 0xf4-0xfc */
	volatile uint32_t lrbase[63];	/* 0x100 (lr0) - ox1f8 */
	volatile uint32_t lr63;		/* 0x1fc */
};

#endif /* __ASSEMBLY__ */

#endif /* GIC_H */

/* ---------------------------------------------------- */


#define ICC_SRE_EL2_SRE			(1 << 0)
#define ICC_SRE_EL2_ENABLE		(1 << 3)

#define GICC_ENABLE			0x1
#define GICC_INT_PRI_THRESHOLD		0xf0
#define GICC_IAR_INT_ID_MASK		0x3ff
#define GICC_DIS_BYPASS_MASK		0x1e0

#define GICD_ENABLE			0x1
#define GICD_DISABLE			0x0
#define GICD_INT_ACTLOW_LVLTRIG		0x0
#define GICD_INT_EN_CLR_X32		0xffffffff
#define GICD_INT_EN_SET_SGI		0x0000ffff
#define GICD_INT_EN_CLR_PPI		0xffff0000
#define GICD_INT_DEF_PRI		0xa0
#define GICD_INT_DEF_PRI_X4		((GICD_INT_DEF_PRI << 24) |\
					(GICD_INT_DEF_PRI << 16) |\
					(GICD_INT_DEF_PRI << 8) |\
					GICD_INT_DEF_PRI)

#define GICC_SIZE		0x2000
#define GICH_SIZE		0x2000

#define GICDv2_CIDR0		0xff0
#define GICDv2_PIDR0		0xfe0
#define GICDv2_PIDR2		0xfe8
#define GICDv2_PIDR4		0xfd0

#define GICC_CTLR_GRPEN1	(1 << 0)
#define GICC_CTLR_EOImode	(1 << 9)

#define GICC_PMR_DEFAULT	0xf0

#define GICV_PMR_SHIFT		3
#define GICH_VMCR_PMR_SHIFT	27
#define GICH_VMCR_EN0		(1 << 0)
#define GICH_VMCR_EN1		(1 << 1)
#define GICH_VMCR_ACKCtl	(1 << 2)
#define GICH_VMCR_EOImode	(1 << 9)

#define GICH_HCR_EN		(1 << 0)
#define GICH_HCR_UIE		(1 << 1)
#define GICH_HCR_LRENPIE	(1 << 2)
#define GICH_HCR_NPIE		(1 << 3)
#define GICH_HCR_VGRP0EIE	(1 << 4)
#define GICH_HCR_VGRP0DIE	(1 << 5)
#define GICH_HCR_VGRP1EIE	(1 << 6)
#define GICH_HCR_VGRP1DIE	(1 << 7)
#define GICH_HCR_EOICOUNT_SHIFT	27

#define GICH_LR_HW_BIT		(1 << 31)
#define GICH_LR_GRP1_BIT	(1 << 30)
#define GICH_LR_ACTIVE_BIT	(1 << 29)
#define GICH_LR_PENDING_BIT	(1 << 28)
#define GICH_LR_PRIORITY_SHIFT	23
#define GICH_LR_SGI_EOI_BIT	(1 << 19)
#define GICH_LR_CPUID_SHIFT	10
#define GICH_LR_PHYS_ID_SHIFT	10
#define GICH_LR_VIRT_ID_MASK	0x3ff

#define GIC_SGI_UNKNOWN			0
#define GIC_SGI_EVENT			1

/* Maintenance IRQ */
#define IRQ_ARCH_ARM_MAINT	25

#ifndef __ASSEMBLY__

void gicc_init(void);
void gic_raise_softirq(int cpu, unsigned int irq);

#endif


