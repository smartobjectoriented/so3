/*
 * Copyright (C) 2014-2017 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifndef SP804_TIMER_H
#define SP804_TIMER_H

#include <types.h>

#define TIMER_RATE			1000000ull   /* 1 MHz */

#define TIMER_CTRL_ONESHOT      (1 << 0)
#define TIMER_CTRL_32BIT        (1 << 1)
#define TIMER_CTRL_DIV1         (0 << 2)
#define TIMER_CTRL_DIV16        (1 << 2)
#define TIMER_CTRL_DIV256       (2 << 2)
#define TIMER_CTRL_IE           (1 << 5)        /* Interrupt Enable (versatile only) */
#define TIMER_CTRL_PERIODIC     (1 << 6)
#define TIMER_CTRL_ENABLE       (1 << 7)

#define TIMER_INTCLR    				0x0c

/* Bits and regs definitions */
/* System controller (SP810) register definitions */
#define SP810_TIMER0_ENSEL	(1 << 15)
#define SP810_TIMER1_ENSEL	(1 << 17)
#define SP810_TIMER2_ENSEL	(1 << 19)
#define SP810_TIMER3_ENSEL	(1 << 21)

struct sp804_timer {
	u32 timerload;		/* 0x00 */
	u32 timervalue;
	u32 timercontrol;
	u32 timerintclr;
	u32 timerris;
	u32 timermis;
	u32 timerbgload;
};

struct sysctrl {
	u32 scctrl;		/* 0x000 */
	u32 scsysstat;
	u32 scimctrl;
	u32 scimstat;
	u32 scxtalctrl;
	u32 scpllctrl;
	u32 scpllfctrl;
	u32 scperctrl0;
	u32 scperctrl1;
	u32 scperen;
	u32 scperdis;
	u32 scperclken;
	u32 scperstat;
	u32 res1[0x006];
	u32 scflashctrl;	/* 0x04c */
	u32 res2[0x3a4];
	u32 scsysid0;		/* 0xee0 */
	u32 scsysid1;
	u32 scsysid2;
	u32 scsysid3;
	u32 scitcr;
	u32 scitir0;
	u32 scitir1;
	u32 scitor;
	u32 sccntctrl;
	u32 sccntdata;
	u32 sccntstep;
	u32 res3[0x32];
	u32 scperiphid0;	/* 0xfe0 */
	u32 scperiphid1;
	u32 scperiphid2;
	u32 scperiphid3;
	u32 scpcellid0;
	u32 scpcellid1;
	u32 scpcellid2;
	u32 scpcellid3;
};


#endif /* SP804_TIMER_H */
