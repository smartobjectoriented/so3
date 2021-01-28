/*
 * Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#include <types.h>

/* Total number of IRQ sources handled by the controller
 *   0- 15 are SGI interrupts: SGI = Software Generated Interrupts
 *  16- 31 are PPI interrupts: PPI = Private Peripheral Interrupts
 *  32-122 are SPI interrupts: SPI = Shared Peripheral Interrupts
 *  122-127 are unused but declared to fit in GIC multiple of 32 bits structures
 */

#define NR_IRQS	160

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

typedef enum {
	GIC_IRQ_TYPE_SPI	= 0,
	GIC_IRQ_TYPE_PPI	= 1,
	GIC_IRQ_TYPE_SGI	= 2
} gic_irq_type_t;

/* Bits and regs definitions */
struct intc_regs {
    /* Distributor Registers: [3] Table 3.2. Distributor register summary */
    volatile uint32_t gicd_ctlr;           /* 0x0000 */
    volatile uint32_t gicd_typer;          /* 0x0004 */
    volatile uint32_t gicd_iidr;           /* 0x0008 */
    volatile uint32_t _res0[29];           /* 0x000c-0x001c: reserved 5 */
                                           /* 0x0020-0x003c: implementation defined 8 */
                                           /* 0x0040-0x007c: reserved 16 */
    volatile uint32_t gicd_igroupr[32];    /* 0x0080 1024 interrupts with 1 bit/interrupt */
    volatile uint32_t gicd_isenabler[32];  /* 0x0100 */
    volatile uint32_t gicd_icenabler[32];  /* 0x0180 */
    volatile uint32_t gicd_ispendr[32];    /* 0x0200 */
    volatile uint32_t gicd_icpendr[32];    /* 0x0280 */
    volatile uint32_t gicd_isactiver[32];  /* 0x0300 */
    volatile uint32_t gicd_icactiver[32];  /* 0x0380 */

    volatile uint32_t gicd_ipriorityr[256]; /* 0x0400 1024 interrupts with 8 bits/interrupt */
    volatile uint32_t gicd_itargetsr[256];  /* 0x0800 1024 interrupts with 8 bits/interrupt */
    volatile uint32_t gicd_icfgr[64];       /* 0x0c00 1024 interrupts with 2 bits/interrupt */
    volatile uint32_t gicd_ppisr;           /* 0x0d00 */
    volatile uint32_t gicd_spisr[63];       /* 0x0d04-0x0dfc */
    volatile uint32_t gicd_nsacrn[64];      /* 0x0e00 (optional) */
    volatile uint32_t gicd_sgir;            /* 0x0f00 */
    volatile uint32_t _res1[3];             /* 0x0f04-0x0f0c */
    volatile uint32_t gicd_cpendsgirn[4];   /* 0x0f10-0x0f1c */
    volatile uint32_t gicd_spendsgirn[4];   /* 0x0f20-0x0f2c */
    volatile uint32_t _res2[40];            /* 0x0f30-0x0fcc */
    volatile uint32_t gicd_pidr4;           /* 0x0fd0 */
    volatile uint32_t gicd_pidr5;           /* 0x0fd4 */
    volatile uint32_t gicd_pidr6;           /* 0x0fd8 */
    volatile uint32_t gicd_pidr7;           /* 0x0fdc */
    volatile uint32_t gicd_pidr0;           /* 0x0fe0 */
    volatile uint32_t gicd_pidr1;           /* 0x0fe4 */
    volatile uint32_t gicd_pidr2;           /* 0x0fe8 */
    volatile uint32_t gicd_pidr3;           /* 0x0fec */
    volatile uint32_t gicd_cidr0;           /* 0x0ff0 */
    volatile uint32_t gicd_cidr1;           /* 0x0ff4 */
    volatile uint32_t gicd_cidr2;           /* 0x0ff8 */
    volatile uint32_t gicd_cidr3;           /* 0x0ffc */

    /* CPU Interface Registers: [3] Table 3.6. CPU interface register summary */
    volatile uint32_t gicc_ctlr;       /* 0x0000 */
    volatile uint32_t gicc_pmr;        /* 0x0004 */
    volatile uint32_t gicc_bpr;        /* 0x0008 */
    volatile uint32_t gicc_iar;        /* 0x000c */
    volatile uint32_t gicc_eoir;       /* 0x0010 */
    volatile uint32_t gicc_rpr;        /* 0x0014 */
    volatile uint32_t gicc_hppir;      /* 0x0018 */
    volatile uint32_t gicc_abpr;       /* 0x001c */
    volatile uint32_t gicc_aiar;       /* 0x0020 */
    volatile uint32_t gicc_aeoir;      /* 0x0024 */
    volatile uint32_t gicc_ahppir;     /* 0x0028 */

    volatile uint32_t _res3[41];       /* 0x002c-0x00cc */

    volatile uint32_t gicc_apr0;       /* 0x00d0 */
    volatile uint32_t _res4[3];        /* 0x00d4-0x00dc */
    volatile uint32_t gicc_nsapr0;     /* 0x00e0 */
    volatile uint32_t _res5[6];        /* 0x00e4-0x00f8 */
    volatile uint32_t gicc_iidr;       /* 0x00fc */

};

#endif /* GIC_H */
