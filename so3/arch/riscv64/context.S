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

# Manage various context-related code (context switch)
#include <linkage.h>
#include <asm-offsets.h>
#include <asm/csr.h>

.global __switch_context

.global __thread_prologue_kernel
.global __thread_prologue_user

.extern thread_prologue

.globl __get_syscall_stack_arg
.globl __get_syscall_args_ext
.globl __exec

# Not used because there is no userspace for now
__exec:
    nop
    j __exec

# Not used because there is no userspace for now
__get_syscall_stack_arg:
    nop
    j __get_syscall_stack_arg

# Not used because there is no userspace for now
__get_syscall_args_ext:
    nop
    j __get_syscall_args_ext

# Kernel thread initial entry point
# Called once per thread
__thread_prologue_kernel:

    # Prepare to jump into C code
    move a0, s4      # tcb->th_fn
    move a1, s5      # tcb->th_arg

    # Re-enable IRQs
    csrsi CSR_STATUS, SR_IE

    jal ra, thread_prologue

# Not used because there is no userspace for now
__thread_prologue_user:
    nop
    j __thread_prologue_user

# a0: address of tcb prev
# a1: address of tcb next
# IRQs are disabled
__switch_context:

    # directly loads context if there is no previous
    beqz a0, load_ctx

save_ctx:

    # Save regs in tcb var
    sd ra, OFFSET_TCB_CPU_REGS + OFFSET_RA( a0 )
    sd sp, OFFSET_TCB_CPU_REGS + OFFSET_SP( a0 )
    sd gp, OFFSET_TCB_CPU_REGS + OFFSET_GP( a0 )
    sd tp, OFFSET_TCB_CPU_REGS + OFFSET_TP( a0 )
    sd t0, OFFSET_TCB_CPU_REGS + OFFSET_T0( a0 )
    sd t1, OFFSET_TCB_CPU_REGS + OFFSET_T1( a0 )
    sd t2, OFFSET_TCB_CPU_REGS + OFFSET_T2( a0 )
    sd fp, OFFSET_TCB_CPU_REGS + OFFSET_FP( a0 )
    sd s1, OFFSET_TCB_CPU_REGS + OFFSET_S1( a0 )
    sd a0, OFFSET_TCB_CPU_REGS + OFFSET_A0( a0 )
    sd a1, OFFSET_TCB_CPU_REGS + OFFSET_A1( a0 )
    sd a2, OFFSET_TCB_CPU_REGS + OFFSET_A2( a0 )
    sd a3, OFFSET_TCB_CPU_REGS + OFFSET_A3( a0 )
    sd a4, OFFSET_TCB_CPU_REGS + OFFSET_A4( a0 )
    sd a5, OFFSET_TCB_CPU_REGS + OFFSET_A5( a0 )
    sd a6, OFFSET_TCB_CPU_REGS + OFFSET_A6( a0 )
    sd a7, OFFSET_TCB_CPU_REGS + OFFSET_A7( a0 )
    sd s2, OFFSET_TCB_CPU_REGS + OFFSET_S2( a0 )
    sd s3, OFFSET_TCB_CPU_REGS + OFFSET_S3( a0 )
    sd s4, OFFSET_TCB_CPU_REGS + OFFSET_S4( a0 )
    sd s5, OFFSET_TCB_CPU_REGS + OFFSET_S5( a0 )
    sd s6, OFFSET_TCB_CPU_REGS + OFFSET_S6( a0 )
    sd s7, OFFSET_TCB_CPU_REGS + OFFSET_S7( a0 )
    sd s8, OFFSET_TCB_CPU_REGS + OFFSET_S8( a0 )
    sd s9, OFFSET_TCB_CPU_REGS + OFFSET_S9( a0 )
    sd s10, OFFSET_TCB_CPU_REGS + OFFSET_S10( a0 )
    sd s11, OFFSET_TCB_CPU_REGS + OFFSET_S11( a0 )
    sd t3, OFFSET_TCB_CPU_REGS + OFFSET_T3( a0 )
    sd t4, OFFSET_TCB_CPU_REGS + OFFSET_T4( a0 )
    sd t5, OFFSET_TCB_CPU_REGS + OFFSET_T5( a0 )
    sd t6, OFFSET_TCB_CPU_REGS + OFFSET_T6( a0 )

load_ctx:

    # Load rest of registers
    ld ra, OFFSET_TCB_CPU_REGS + OFFSET_RA( a1 )
    ld sp, OFFSET_TCB_CPU_REGS + OFFSET_SP( a1 )
    ld gp, OFFSET_TCB_CPU_REGS + OFFSET_GP( a1 )
    ld tp, OFFSET_TCB_CPU_REGS + OFFSET_TP( a1 )
    ld t0, OFFSET_TCB_CPU_REGS + OFFSET_T0( a1 )
    ld t1, OFFSET_TCB_CPU_REGS + OFFSET_T1( a1 )
    ld t2, OFFSET_TCB_CPU_REGS + OFFSET_T2( a1 )
    ld fp, OFFSET_TCB_CPU_REGS + OFFSET_FP( a1 )
    ld s1, OFFSET_TCB_CPU_REGS + OFFSET_S1( a1 )
    ld a0, OFFSET_TCB_CPU_REGS + OFFSET_A0( a1 )
    // a1 has to be done last to avoid memory issues
    ld a2, OFFSET_TCB_CPU_REGS + OFFSET_A2( a1 )
    ld a3, OFFSET_TCB_CPU_REGS + OFFSET_A3( a1 )
    ld a4, OFFSET_TCB_CPU_REGS + OFFSET_A4( a1 )
    ld a5, OFFSET_TCB_CPU_REGS + OFFSET_A5( a1 )
    ld a6, OFFSET_TCB_CPU_REGS + OFFSET_A6( a1 )
    ld a7, OFFSET_TCB_CPU_REGS + OFFSET_A7( a1 )
    ld s2, OFFSET_TCB_CPU_REGS + OFFSET_S2( a1 )
    ld s3, OFFSET_TCB_CPU_REGS + OFFSET_S3( a1 )
    ld s4, OFFSET_TCB_CPU_REGS + OFFSET_S4( a1 )
    ld s5, OFFSET_TCB_CPU_REGS + OFFSET_S5( a1 )
    ld s6, OFFSET_TCB_CPU_REGS + OFFSET_S6( a1 )
    ld s7, OFFSET_TCB_CPU_REGS + OFFSET_S7( a1 )
    ld s8, OFFSET_TCB_CPU_REGS + OFFSET_S8( a1 )
    ld s9, OFFSET_TCB_CPU_REGS + OFFSET_S9( a1 )
    ld s10, OFFSET_TCB_CPU_REGS + OFFSET_S10( a1 )
    ld s11, OFFSET_TCB_CPU_REGS + OFFSET_S11( a1 )
    ld t3, OFFSET_TCB_CPU_REGS + OFFSET_T3( a1 )
    ld t4, OFFSET_TCB_CPU_REGS + OFFSET_T4( a1 )
    ld t5, OFFSET_TCB_CPU_REGS + OFFSET_T5( a1 )
    ld t6, OFFSET_TCB_CPU_REGS + OFFSET_T6( a1 )
    ld a1, OFFSET_TCB_CPU_REGS + OFFSET_A1( a1 )

    ret

# Label is only used with processes.. No need to implement it yet, we are stuck with threads
# for now
__save_context:
    nop
    j __save_context
