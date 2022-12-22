/*
 * Copyright (C) 2020 David Truan <david.truan@heig-vd.ch>
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

#ifndef I2C_BSC_H
#define I2C_BSC_H

#include <types.h>

/* I2C Registers (manual p.28) */
struct i2c_regs {
	volatile uint32_t i2c_ctrl;		/* 0x0 Control Register */
	volatile uint32_t i2c_stat;		/* 0x4 Status Register */
	volatile uint32_t i2c_dlen;		/* 0x8 Data Length Register */
	volatile uint32_t i2c_a;		/* 0xc Slave Address Register */
	volatile uint32_t i2c_fifo;		/* 0x10 Data FIFO Register */
	volatile uint32_t i2c_div;		/* 0x14 Clock Divider Register */
	volatile uint32_t i2c_del;		/* 0x18 Data Delay Register */
	volatile uint32_t i2c_clkt;		/* 0x1c Clock Stretch Timeout Register */
};

/* I2C Control Register */
#define CTRL_I2CEN		(1 << 15)
#define CTRL_START_TR		(1 << 7)
#define CTRL_CLEAR_FIFO		(1 << 4)
#define CTRL_ST		    (1 << 7)
#define CTRL_INTD		(1 << 8)
#define CTRL_INTT	    (1 << 9)
#define CTRL_INTR	    (1 << 10)
#define CTRL_READ       (1 << 0)


/* I2C Status Register */
#define S_CLKT  (1 << 9)
#define S_ERR   (1 << 8)
#define S_RXF   (1 << 7)
#define S_TXE   (1 << 6)
#define S_RXD   (1 << 5)
#define S_TXD   (1 << 4)
#define S_RXR   (1 << 3)
#define S_TXW   (1 << 2)
#define S_DONE  (1 << 1)
#define S_TA    (1 << 0)

#define I2C_TOUT		0x40


#define NO_STRETCH  0x0

#define I2C_WRITE   false
#define I2C_READ    true

#warning to be moved in DT...

#define GPIO_REGS_ADDR 			0xFE200000
#define GPIO_FSEL0_OFF			0x0
#define GPIO23_ALT0			0x00000900
#define GPIO23_CLEAR			0x00000FC0


void i2c_bsc_write_data(uint32_t slave_addr, uint8_t* buf, size_t size);
void i2c_bsc_read_data(uint32_t slave_addr, uint8_t* buf, size_t size);

#endif /* I2C_BSC_H */
 
