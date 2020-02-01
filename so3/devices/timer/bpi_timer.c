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

#include <types.h>
#include <heap.h>

#include <device/irq.h>

#include <device/device.h>
#include <device/driver.h>

#include <device/serial.h>
#include <device/arch/bpi_timer.h>

#include <asm/io.h>                 /* ioread/iowrite macros */
#include <timer.h>

#define TMR_IRQ_EN_REG		0x0
#define TMR_STATUS_REG		0x4
#define TMR_CTRL_REG 		0x10
#define TMR_INT_VALUE_REG	0X14
#define TMR_VALUE_REG 		0x18
#define TIMER_CTRL_IE 		0x1
#define TIMER_INTCLR		0x1
#define CONTINUOUS_MODE 	(0x1 << 6)
#define TIMER_CTRL_DIV16 	(0b100 << 5)
#define TIMER_CTRL_ENABLE 	0x1
#define TIMER_CTRL_RELOAD   0x2
#define TMR_IRQ_STATUS_REG 	0x4

static irq_number;

static dev_t bananapi_dev =
{

};

uint32_t period = 0xB71B00;//1 seconde lorsque div16 est actif

void start(void){
	int i;
	*(unsigned int *)(bananapi_dev.base + TMR_INT_VALUE_REG) = period;
	*(unsigned int *)(bananapi_dev.base + TMR_CTRL_REG) |= TIMER_CTRL_RELOAD;
	while(*(unsigned int *)(bananapi_dev.base + TMR_CTRL_REG) & TIMER_CTRL_RELOAD); //ok
	*(unsigned int *)(bananapi_dev.base + TMR_CTRL_REG) |= TIMER_CTRL_ENABLE;

	printk("start@ %x \n",bananapi_dev.base);
	irq_enable(50);
	local_irq_enable();
	while (1)
		printk("## T: %x\n", *(unsigned int *)(bananapi_dev.base + TMR_VALUE_REG));
}

void irq_handler(void){
	printk("Interrupt catch \n");
	*(unsigned int *)(bananapi_dev.base + TMR_IRQ_STATUS_REG) |= TIMER_INTCLR;	  //clear l'interruption
	*(unsigned int *)(bananapi_dev.base + TMR_CTRL_REG) |= TIMER_CTRL_ENABLE;

}

static int bananapiTimer_init(dev_t *dev) {

	/* Init bananapi timer */

	memcpy(&bananapi_dev, dev, sizeof(dev_t));

	*(unsigned int *)(bananapi_dev.base + TMR_INT_VALUE_REG) |= period; //Set interval value
	*(unsigned int *)(bananapi_dev.base + TMR_CTRL_REG) &= 0;
	*(unsigned int *)(bananapi_dev.base + TMR_CTRL_REG) |= 0x94; //Select Single mode,24MHz clock source,2 pre-scale
	//*(unsigned int *)(bananapi_dev.base + TMR_CTRL_REG) |= CONTINUOUS_MODE;  //ok
	*(unsigned int *)(bananapi_dev.base + TMR_IRQ_EN_REG) |= TIMER_CTRL_IE;  //Activation des interruptions
	*(unsigned int *)(bananapi_dev.base + TMR_IRQ_STATUS_REG) |= TIMER_INTCLR;	  //ok


	periodic_timer.dev = &bananapi_dev;
	periodic_timer.period = period;
	periodic_timer.start = &start;

	irq_bind(bananapi_dev.irq, irq_handler, NULL, NULL);

	printk("Bananapi Timer Initialized @ %x \n",bananapi_dev.base);

	return 0;
}

REGISTER_DRIVER(SP804_timer, "allwinnertimer", bananapiTimer_init);
