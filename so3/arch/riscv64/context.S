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

#define WORD_SIZE 8
#define CONTEXT_SIZE (33 * WORD_SIZE)

.global __switch_context

.global __thread_prologue_kernel
.global __thread_prologue_user

.extern thread_prologue

.globl __get_syscall_stack_arg
.globl __get_syscall_args_ext

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
    addi    mscratch, a0, #(OFFSET_TCB_CPU_REGS)

    sd ra, OFFSET_RA( mscratch )
    sd sp, OFFSET_SP( mscratch )
    sd gp, OFFSET_GP( mscratch )
    sd tp, OFFSET_TP( mscratch )
    sd t0, OFFSET_T0( mscratch )
    sd t1, OFFSET_T1( mscratch )
    sd t2, OFFSET_T2( mscratch )
    sd fp, OFFSET_FP( mscratch )
    sd s1, OFFSET_S1( mscratch )
    sd a0, OFFSET_A0( mscratch )
    sd a1, OFFSET_A1( mscratch )
    sd a2, OFFSET_A2( mscratch )
    sd a3, OFFSET_A3( mscratch )
    sd a4, OFFSET_A4( mscratch )
    sd a5, OFFSET_A5( mscratch )
    sd a6, OFFSET_A6( mscratch )
    sd a7, OFFSET_A7( mscratch )
    sd s2, OFFSET_S2( mscratch )
    sd s3, OFFSET_S3( mscratch )
    sd s4, OFFSET_S4( mscratch )
    sd s5, OFFSET_S5( mscratch )
    sd s6, OFFSET_S6( mscratch )
    sd s7, OFFSET_S7( mscratch )
    sd s8, OFFSET_S8( mscratch )
    sd s9, OFFSET_S9( mscratch )
    sd s10, OFFSET_S10( mscratch )
    sd s11, OFFSET_S11( mscratch )
    sd t3, OFFSET_T3( mscratch )
    sd t4, OFFSET_T4( mscratch )
    sd t5, OFFSET_T5( mscratch )
    sd t6, OFFSET_T6( mscratch )

    csrr t0, mstatus
    sd t0, OFFSET_STATUS( mscratch )

    csrr t0, mepc
    sd t0, OFFSET_EPC( mscratch )

load_ctx:

    # load new context into cpu_regs stored into tcb_t of a thread (or process)
    addi    mscratch, a1, #(OFFSET_TCB_CPU_REGS)

    # overwrite csr with new context values
    ld t0, OFFSET_EPC( mscratch )
    csrw mepc, t0


    ld t0, OFFSET_STATUS( mscratch )
    csrw mstatus, t0

    # Load rest of registers
    ld ra, OFFSET_RA( mscratch )
    ld sp, OFFSET_SP( mscratch )
    ld gp, OFFSET_GP( mscratch )
    ld tp, OFFSET_TP( mscratch )
    ld t0, OFFSET_T0( mscratch )
    ld t1, OFFSET_T1( mscratch )
    ld t2, OFFSET_T2( mscratch )
    ld fp, OFFSET_FP( mscratch )
    ld s1, OFFSET_S1( mscratch )
    ld a0, OFFSET_A0( mscratch )
    ld a1, OFFSET_A1( mscratch )
    ld a2, OFFSET_A2( mscratch )
    ld a3, OFFSET_A3( mscratch )
    ld a4, OFFSET_A4( mscratch )
    ld a5, OFFSET_A5( mscratch )
    ld a6, OFFSET_A6( mscratch )
    ld a7, OFFSET_A7( mscratch )
    ld s2, OFFSET_S2( mscratch )
    ld s3, OFFSET_S3( mscratch )
    ld s4, OFFSET_S4( mscratch )
    ld s5, OFFSET_S5( mscratch )
    ld s6, OFFSET_S6( mscratch )
    ld s7, OFFSET_S7( mscratch )
    ld s8, OFFSET_S8( mscratch )
    ld s9, OFFSET_S9( mscratch )
    ld s10, OFFSET_S10( mscratch )
    ld s11, OFFSET_S11( mscratch )
    ld t3, OFFSET_T3( mscratch )
    ld t4, OFFSET_T4( mscratch )
    ld t5, OFFSET_T5( mscratch )
    ld t6, OFFSET_T6( mscratch )

    ret

# Label is only used with processes.. No need to implement it yet, we are stuck with threads
# for now
__save_context:
    nop
    j __save_context
