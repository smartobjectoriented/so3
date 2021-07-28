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

#include <asm/csr.h>

.global _start
.global __fdt_addr

.extern __stack_top
.extern handle_mmode_trap
.extern handle_smode_trap

.section ".head.text","ax"

 # SO3 Kernel entry point
_start:

  # Disable IRQs in reg directly
  csrw mie, zero
  csrw mip, zero
  # clear ie flag (interrupt enable)
  csrci mstatus, SR_MIE

  # Preserve the (physical address) of device tree base in s9,
  # a1 contains device tree adress
  mv s9, a1

  # Initialize stack pointer to the stack defined in the link script
  la   sp, .LCstack_top
  ld   sp, (sp)

  # RISC-V does not have a vector table (it could but it is not used for now).
  # It has a single address that then redirects to correct handlers. Base address
  # is defined here for machine mode
  la    t0, handle_mmode_trap
  csrw  mtvec, t0;

__kernel_main:

    # Prepare to clear bss and zero fp
    la  t0, __bss_start
    la  t1, __bss_end

    # Clear BSS (and zero fp)
    li  fp, 0
    priv_clear_bss:
        sd      zero, (t0)
        # We are on 64 bits, 8 bytes, +8 because start is at the bottom of the bss
        add     t0, t0, 8
        blt     t0, t1, priv_clear_bss

    # Store DT at label
    la      t0, __fdt_addr
    sd      s9, (t0)

    # ##########################################
    # Prepare for switch to supervisor mode
    # ##########################################

    # Setting trap handler for smode
    la      t0, handle_smode_trap
    csrw    stvec, t0

    # At mret call, the pc is set back to mepc. Here we preload jump address to kernel entry
    la      t0, kernel_start
    csrw    mepc, t0

    # Normally, external IRQs and timer IRQs are handled in machine mode.
    # Writting these registers delegates handling to supervisor mode
    # Excpetions will still be handled in machine mode
    # Other IRQs are not fowarded because they should not happen in SO3,
    # they are handled by default with a kernel panic
    li      t0, (1 << IRQ_M_TIMER) | (1 << IRQ_M_EXT) | (1 << IRQ_S_TIMER) | (1 << IRQ_S_EXT)
    csrs    mideleg, t0

    # Make sure supervisor mode irqs are disabled
    csrw    sie, zero
    csrci   mstatus, SR_SIE

    # Enable irqs in mie (machine mode irq enable) because this is the last time we are in
    # machine mode.. And if machine mode irqs are disabled, supervisor irqs won't be thrown.
    # It's important to set machine irqs because that is what they are before beeing
    # redirected to supervisor mode.
    li      t0, (1 << IRQ_M_TIMER) | (1 << IRQ_M_EXT)
    csrs    mie, t0

    # Enable machine irqs globally for the same reasons. SO3 is still irq safe because
    # it only throws 2 type of irqs that are redirected to supervisor mode which has irqs off
    # Write in MPIE (previous mie) because it will be swapped to mie at mret instruction
    li      t0, (PROCESSOR_SUPERVISOR_MODE << SR_MPP_SHIFT) | SR_MPIE
    csrs    mstatus, t0

    # C main entry point, never returns. Enters in supervisor mode
    mret

# Addr on 64 bits => 8 bytes to store it
__fdt_addr:
  .8byte    0

.align 8

.LCvirt_entry:
  .8byte __kernel_main

.LCstack_top:
  .8byte __stack_top
