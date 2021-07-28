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
 * Inspired by Stephen Marz low level handling for his rust OS
 *
 */

.section ".vectors.text", "ax"

#include <asm-offsets.h>

.globl _vectors
.globl __ll_handle_trap

.extern _start
.extern handle_trap
.extern __stack_top

/* RISC-V uses one address that handles every trap. There is no need for a vector table.
 * Note: There is an option to do it with a vector table if really needed. */
_vectors:
    j   _start

/* Saves system state before entering trap handler and restores it afterwards */
__ll_handle_trap:

    # mscratch has trap frame to store current registers
    csrrw   t6, mscratch, t6

    # Store all registers into trap frame
    sd ra, OFFSET_RA( t6 )
    sd sp, OFFSET_SP( t6 )
    sd gp, OFFSET_GP( t6 )
    sd tp, OFFSET_TP( t6 )
    sd t0, OFFSET_T0( t6 )
    sd t1, OFFSET_T1( t6 )
    sd t2, OFFSET_T2( t6 )
    sd fp, OFFSET_FP( t6 )
    sd s1, OFFSET_S1( t6 )
    sd a0, OFFSET_A0( t6 )
    sd a1, OFFSET_A1( t6 )
    sd a2, OFFSET_A2( t6 )
    sd a3, OFFSET_A3( t6 )
    sd a4, OFFSET_A4( t6 )
    sd a5, OFFSET_A5( t6 )
    sd a6, OFFSET_A6( t6 )
    sd a7, OFFSET_A7( t6 )
    sd s2, OFFSET_S2( t6 )
    sd s3, OFFSET_S3( t6 )
    sd s4, OFFSET_S4( t6 )
    sd s5, OFFSET_S5( t6 )
    sd s6, OFFSET_S6( t6 )
    sd s7, OFFSET_S7( t6 )
    sd s8, OFFSET_S8( t6 )
    sd s9, OFFSET_S9( t6 )
    sd s10, OFFSET_S10( t6 )
    sd s11, OFFSET_S11( t6 )
    sd t3, OFFSET_T3( t6 )
    sd t4, OFFSET_T4( t6 )
    sd t5, OFFSET_T5( t6 )

    # Save real t6 register and keep trap frame pointer in t5 that we already stored
    mv      t5, t6
    csrr    t6, mscratch
    sd      t6, OFFSET_T6( t5 )

    # Restore the kernel trap frame into mscratch
    csrw    mscratch, t5

    # Get ready for C handler and getting debug params ready
    csrr    a0, mepc
    sd      a0, OFFSET_EPC( t5 )
    csrr    a1, mtval
    csrr    a2, mcause
    csrr    a3, mstatus
    sd      a0, OFFSET_STATUS( t5 )
    csrr    a4, mscratch
    /* _NMR_ TODO keep stack pointer like that ? */
    call    handle_trap

    # handle_trap will return the return address via a0
    csrw    mepc, a0
    # Now load the trap frame back into t6
    csrr    t6, mscratch

    ld ra, OFFSET_RA( t6 )
    ld sp, OFFSET_SP( t6 )
    ld gp, OFFSET_GP( t6 )
    ld tp, OFFSET_TP( t6 )
    ld t0, OFFSET_T0( t6 )
    ld t1, OFFSET_T1( t6 )
    ld t2, OFFSET_T2( t6 )
    ld fp, OFFSET_FP( t6 )
    ld s1, OFFSET_S1( t6 )
    ld a0, OFFSET_A0( t6 )
    ld a1, OFFSET_A1( t6 )
    ld a2, OFFSET_A2( t6 )
    ld a3, OFFSET_A3( t6 )
    ld a4, OFFSET_A4( t6 )
    ld a5, OFFSET_A5( t6 )
    ld a6, OFFSET_A6( t6 )
    ld a7, OFFSET_A7( t6 )
    ld s2, OFFSET_S2( t6 )
    ld s3, OFFSET_S3( t6 )
    ld s4, OFFSET_S4( t6 )
    ld s5, OFFSET_S5( t6 )
    ld s6, OFFSET_S6( t6 )
    ld s7, OFFSET_S7( t6 )
    ld s8, OFFSET_S8( t6 )
    ld s9, OFFSET_S9( t6 )
    ld s10, OFFSET_S10( t6 )
    ld s11, OFFSET_S11( t6 )
    ld t3, OFFSET_T3( t6 )
    ld t4, OFFSET_T4( t6 )
    ld t5, OFFSET_T5( t6 )
    # t6 will be reset to old value without anything to here for the load
    ld t6, OFFSET_T6( t6 )

    # Get back to what we were doing
    mret

.LCstack_top:
  .word __stack_top
