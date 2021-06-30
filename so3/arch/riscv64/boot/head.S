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

 # SO3 Kernel entry point

#include <linkage.h>

#include <generated/autoconf.h>

#include <asm/processor.h>
#include <asm/mmu.h>
#include <asm/memory.h>
#include <thread.h>

.global _start
.global __fdt_addr

.extern clear_bss
.extern __stack_top
.extern __vectors_start

.section ".head.text","ax"


/*
 * TODO enter supervisor mode
 */
.macro enter_smode
    # Nothing yet
    # Set CSR to disable all irqs and enter Smode
.endm

_start:

  enter_smode

  # Configure address of the vector table
  la   t0, .LCvectors
  ld   t0, (t0)

# TODO Find a way to set vector table to replace arm instruction
#  mcr   p15, 0, r1, c12, c0, 0      @ vector table

/*
 *  TODO define where the device tree address is and store it in label
 */

  # Initialize stack pointer to the stack defined in the link script
  la   sp, .LCstack_top
  ld   sp, (sp)

__kernel_main:

    # Prepare to clear bss and zero fp
    la t0, __bss_start
    la t1, __bss_end

    # Clear BSS (and zero fp)
    li fp, 0
    priv_clear_bss:
        sd zero, (t0)
        # We are on 64 bits, 8 bytes, +8 because start is at the bottom of the bss
        add t0, t0, 8
        blt t0, t1, priv_clear_bss

/* TODO see what to do with de DT */
  # ldr   r0, =__fdt_addr
  # str   r9, [r0]

  # C main entry point, never returns
  j     kernel_start


# TODO find equivalent?
# .ltorg

# Addr on 64 bits => 8 bytes to store it
__fdt_addr:
  .8byte    0

.align 8

.LCvirt_entry:
  .8byte __kernel_main

.LCstack_top:
  .8byte __stack_top

.LCvectors:
  .8byte __vectors_start
