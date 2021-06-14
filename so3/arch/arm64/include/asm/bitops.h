/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright 1995, Russell King.
 * Various bits and pieces copyrights include:
 *  Linus Torvalds (test_bit).
 * Big endian support: Copyright 2001, Nicolas Pitre
 *  reworked by rmk.
 *
 * bit 0 is the LSB of an "unsigned long" quantity.
 *
 * Please note that the code in this file should never be included
 * from user space.  Many of these are not implemented in assembler
 * since they would be too costly.  Also, they require privileged
 * instructions (which are not available from user mode) to ensure
 * that they are atomic.
 */

#ifndef ASM_BITOPS_H
#define ASM_BITOPS_H

#include <compiler.h>

#include <asm/processor.h>

#define __L2(_x)  (((_x) & 0x00000002) ?   1 : 0)
#define __L4(_x)  (((_x) & 0x0000000c) ? ( 2 + __L2( (_x)>> 2)) : __L2( _x))
#define __L8(_x)  (((_x) & 0x000000f0) ? ( 4 + __L4( (_x)>> 4)) : __L4( _x))
#define __L16(_x) (((_x) & 0x0000ff00) ? ( 8 + __L8( (_x)>> 8)) : __L8( _x))
#define LOG_2(_x) (((_x) & 0xffff0000) ? (16 + __L16((_x)>>16)) : __L16(_x))

#ifndef __ASSEMBLY__

#define smp_mb__before_clear_bit()	mb()
#define smp_mb__after_clear_bit()	mb()

#define BITOP_MASK(nr)		(1UL << ((nr) % BITS_PER_LONG))
#define BITOP_WORD(nr)		((nr) / BITS_PER_LONG)

/**
 * test_bit - Determine whether a bit is set
 * @nr: bit number to test
 * @addr: Address to start counting from
 */
static inline int test_bit(int nr, const volatile unsigned long *addr)
{
        return 1UL & (addr[BITOP_WORD(nr)] >> (nr & (BITS_PER_LONG-1)));
}



/*
 * These functions are the basis of our bit ops.
 *
 * First, the atomic bitops. These use native endian.
 */
static inline void ____atomic_set_bit(unsigned int bit, volatile unsigned long *p)
{
	unsigned long flags;
	unsigned long mask = 1UL << (bit & 31);

	p += bit >> 5;

	local_irq_save(flags);
	*p |= mask;
	local_irq_restore(flags);
}

static inline void ____atomic_clear_bit(unsigned int bit, volatile unsigned long *p)
{
	unsigned long flags;
	unsigned long mask = 1UL << (bit & 31);

	p += bit >> 5;

	local_irq_save(flags);
	*p &= ~mask;
	local_irq_restore(flags);
}

static inline void ____atomic_change_bit(unsigned int bit, volatile unsigned long *p)
{
	unsigned long flags;
	unsigned long mask = 1UL << (bit & 31);

	p += bit >> 5;

	local_irq_save(flags);
	*p ^= mask;
	local_irq_restore(flags);
}

static inline int
____atomic_test_and_set_bit(unsigned int bit, volatile unsigned long *p)
{
	unsigned long flags;
	unsigned int res;
	unsigned long mask = 1UL << (bit & 31);

	p += bit >> 5;

	local_irq_save(flags);
	res = *p;
	*p = res | mask;
	local_irq_restore(flags);

	return res & mask;
}

static inline int
____atomic_test_and_clear_bit(unsigned int bit, volatile unsigned long *p)
{
	unsigned long flags;
	unsigned int res;
	unsigned long mask = 1UL << (bit & 31);

	p += bit >> 5;

	local_irq_save(flags);
	res = *p;
	*p = res & ~mask;
	local_irq_restore(flags);

	return res & mask;
}

static inline int
____atomic_test_and_change_bit(unsigned int bit, volatile unsigned long *p)
{
	unsigned long flags;
	unsigned int res;
	unsigned long mask = 1UL << (bit & 31);

	p += bit >> 5;

	local_irq_save(flags);
	res = *p;
	*p = res ^ mask;
	local_irq_restore(flags);

	return res & mask;
}



/*
 *  A note about Endian-ness.
 *  -------------------------
 *
 * When the ARM is put into big endian mode via CR15, the processor
 * merely swaps the order of bytes within words, thus:
 *
 *          ------------ physical data bus bits -----------
 *          D31 ... D24  D23 ... D16  D15 ... D8  D7 ... D0
 * little     byte 3       byte 2       byte 1      byte 0
 * big        byte 0       byte 1       byte 2      byte 3
 *
 * This means that reading a 32-bit word at address 0 returns the same
 * value irrespective of the endian mode bit.
 *
 * Peripheral devices should be connected with the data bus reversed in
 * "Big Endian" mode.  ARM Application Note 61 is applicable, and is
 * available from http://www.arm.com/.
 *
 * The following assumes that the data bus connectivity for big endian
 * mode has been followed.
 *
 * Note that bit 0 is defined to be 32-bit word bit 0, not byte 0 bit 0.
 */

/*
 * Little endian assembly bitops.  nr = 0 -> byte 0 bit 0.
 */
extern void _set_bit_le(int nr, volatile unsigned long * p);
extern void _clear_bit_le(int nr, volatile unsigned long * p);
extern void _change_bit_le(int nr, volatile unsigned long * p);
extern int _test_and_set_bit_le(int nr, volatile unsigned long * p);
extern int _test_and_clear_bit_le(int nr, volatile unsigned long * p);
extern int _test_and_change_bit_le(int nr, volatile unsigned long * p);


/*
 * The __* form of bitops are non-atomic and may be reordered.
 */

#define	ATOMIC_BITOP_LE(name,nr,p)	 (____atomic_##name(nr, p) )
#define	ATOMIC_BITOP_BE(name,nr,p)	(____atomic_##name(nr, p) )


#define NONATOMIC_BITOP(name,nr,p)		\
	(____nonatomic_##name(nr, p))


/*
 * These are the little endian, atomic definitions.
 */
#define set_bit(nr,p)			ATOMIC_BITOP_LE(set_bit,nr,p)
#define clear_bit(nr,p)			ATOMIC_BITOP_LE(clear_bit,nr,p)
#define change_bit(nr,p)		ATOMIC_BITOP_LE(change_bit,nr,p)
#define test_and_set_bit(nr,p)		ATOMIC_BITOP_LE(test_and_set_bit,nr,p)
#define test_and_clear_bit(nr,p)	ATOMIC_BITOP_LE(test_and_clear_bit,nr,p)
#define test_and_change_bit(nr,p)	ATOMIC_BITOP_LE(test_and_change_bit,nr,p)

#endif /*__ASSEMBLY__ */

#endif /* ASM_BITOPS_H */
