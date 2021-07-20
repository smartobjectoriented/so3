/*
 * Copyright (C) 2021 Nicolas Müller <nicolas.muller1@heig-vd.ch>
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
#include <asm/processor.h>

int _find_first_zero_bit(void *addr, unsigned int maxbit) {

    /* function is not used yet */
    BUG();
    return -1;
}

int _find_next_zero_bit(void *addr, unsigned int maxbit, int offset) {

    /* function is not used yet */
    BUG();
    return -1;
}

int _find_first_bit(const unsigned int *p, unsigned size) {

    unsigned int count, no;

    if (size > BITS_PER_INT) {
    	BUG();
    }

	no = *p;
	count = 0;

	if (no) {
		while(!(no & (1 << count))) {
			count++;
		}
	}
	else {
		printk("Can't find first bit of 0\n");
		BUG();
	}

	return count;
}

int _find_next_bit(const unsigned int *p, int size, int offset) {

    /* function is not used yet */
    BUG();
	return -1;

#if 0 /* Not tested */

    unsigned int count, no;

    /* Size is in bits and sizeof in bytes */
    if (size > BITS_PER_INT) {
    	BUG();
    }

	no = *p;
	count = offset;

	if (no) {
		while( !(no & (1 << count++)));
	}

	return count;
#endif
}
