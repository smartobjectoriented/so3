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
 
#ifndef I2C_H
#define I2C_H

#include <types.h>

/* I2C controller */
typedef struct  {
	void (*i2c_write_op)(uint32_t slave_addr, uint8_t* buf, size_t size);
	void (*i2c_read_op)(uint32_t slave_addr, uint8_t* buf, size_t size);
} i2c_ops_t;

extern i2c_ops_t i2c_ops;

void i2c_write(uint32_t slave_addr, uint8_t* buf, size_t size);
void i2c_read(uint32_t slave_addr, uint8_t* buf, size_t size);
void i2c_read_smbus_data_byte(uint32_t slave_addr, uint8_t* buf, uint8_t command);

#endif /* I2C_H */
