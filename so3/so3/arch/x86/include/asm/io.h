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

#ifndef ASM_IO_H
#define ASM_IO_H

#include <types.h>

#define ioread8(p) (*((volatile uint8_t *) (p)))
#define ioread16(p) (*((volatile uint16_t *) (p)))
#define ioread32(p) (*((volatile uint32_t *) (p)))
#define ioread64(p) (*((volatile uint64_t *) (p)))

#define iowrite8(p, v) (*((volatile uint8_t *) (p)) = v)
#define iowrite16(p, v) (*((volatile uint16_t *) (p)) = v)
#define iowrite32(p, v) (*((volatile uint32_t *) (p)) = v)
#define iowrite64(p, v) (*((volatile uint64_t *) (p)) = v)

/* Legacy PC devices (PIC, PIT, COM ports) sit in the I/O port space. */

static inline uint8_t inb(uint16_t port)
{
	uint8_t val;

	asm volatile("inb %1, %0" : "=a"(val) : "Nd"(port) : "memory");

	return val;
}

static inline void outb(uint16_t port, uint8_t val)
{
	asm volatile("outb %0, %1" : : "a"(val), "Nd"(port) : "memory");
}

static inline uint16_t inw(uint16_t port)
{
	uint16_t val;

	asm volatile("inw %1, %0" : "=a"(val) : "Nd"(port) : "memory");

	return val;
}

static inline void outw(uint16_t port, uint16_t val)
{
	asm volatile("outw %0, %1" : : "a"(val), "Nd"(port) : "memory");
}

static inline uint32_t inl(uint16_t port)
{
	uint32_t val;

	asm volatile("inl %1, %0" : "=a"(val) : "Nd"(port) : "memory");

	return val;
}

static inline void outl(uint16_t port, uint32_t val)
{
	asm volatile("outl %0, %1" : : "a"(val), "Nd"(port) : "memory");
}

/* A write to an unused port gives slow legacy chips time to settle. */

static inline void io_delay(void)
{
	outb(0x80, 0);
}

#endif /* ASM_IO_H */
