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

#if 0
#define DEBUG
#endif

#include <device/i2c.h>

i2c_ops_t i2c_ops;

void i2c_write(uint32_t slave_addr, uint8_t *buf, size_t size) {
	i2c_ops.i2c_write_op(slave_addr, buf, size);
}

void i2c_read(uint32_t slave_addr, uint8_t *buf, size_t size) {
	i2c_ops.i2c_read_op(slave_addr, buf, size);
}

/* Emulated SMBUS protocol implementing only DATA_BYTE */
void i2c_read_smbus_data_byte(uint32_t slave_addr, uint8_t *buf, uint8_t command) {
	i2c_ops.i2c_write_op(slave_addr, &command, 1);
	i2c_ops.i2c_read_op(slave_addr, buf, 1);	
}
