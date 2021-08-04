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
.extern mtrap_frame
.extern strap_frame

.extern __ll_handle_mmode_trap
.extern __ll_handle_smode_trap

.section ".head.text","ax"

 # SO3 Kernel entry point
_start:

  # Disable IRQs for in reg directly
  csrw CSR_MIE, zero
  csrw CSR_MIP, zero
  csrw CSR_SIE, zero
  csrw CSR_SIP, zero

  # clear ie flag for s mode and m mode (interrupt enable)
  csrci mstatus, SR_MIE
  csrci mstatus, SR_SIE

  # Preserve the (physical address) of device tree base in s9,
  # a1 contains device tree adress
  mv s9, a1

  # Initialize stack pointer to the stack defined in the link script
  la   sp, .LCstack_top
  ld   sp, (sp)

  # RISC-V does not have a vector table (it could but it is not used for now).
  # It has a single address that then redirects to correct handlers. Base address

  # is defined here for machine mode
  la    t0, __ll_handle_mmode_trap
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
    la      t0, __ll_handle_smode_trap
    csrw    stvec, t0

    # Init sscratch and msscratch to trap_frame, they can't use same frame
    # in case a machine irq raises while doing smode isr.
    la      t0, mtrap_frame
    csrw    mscratch, t0
    la      t0, strap_frame
    csrw    sscratch, t0

    # At mret call, the pc is set back to mepc. Here we preload jump address to kernel entry
    la      t0, kernel_start
    csrw    mepc, t0

    # Timers interrupts are machine interrupts. They will be forwarded to supervisor timer interrupt
    # by hand.. Since after this file, we'll switch to supervisor mode, it's the last moment we can
    # do it.
    # Supervisor interrupts will be enabled from supervisor kernel from the timer device and from
    # irq controller device if they are in the device tree.
    li      t0, IE_MTIE
    csrw    mie, t0

    # Enable machine irqs globally for the same reasons. In supervisor mode,
    # machine irqs will always be active, no need to enable useless things. c.f. in specs section 3.1.6.1 (p.20)
    # https://github.com/riscv/riscv-isa-manual/releases/download/Ratified-IMFDQC-and-Priv-v1.11/riscv-privileged-20190608.pdf
    li      t0, (PROCESSOR_SUPERVISOR_MODE << SR_MPP_SHIFT)
    csrs    mstatus, t0

    # Have to be active to allow supervisor timer interrupt and external interrupts
    li      t0, IE_SEIE | IE_STIE
    csrw    mideleg, t0

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
