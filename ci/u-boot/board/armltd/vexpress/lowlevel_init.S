/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
 */

#include <config.h>

/*
 * Routine: save_boot_params (called after reset from start.S)
 * Description: check the running CPU and stall it if not running on
 * the primary CPU.
 * This allows U-boot to run on Qemu/vExpress with a number of CPU
 * greater than 1 (-smp > 1)
 */

.global save_boot_params

/*
 * The stalling code is mainly based on the bootcode portion (smpboot)
 * from Qemu.
 */
__start:
stall_secondary_cpu:
	ldr	r2, GIC_CPU_IF
	ldr r0, BOOT_REG
	mov	r1, #1
	str r1, [r2]
	mov	r1, #0xff
	str r1, [r2, #4]
	dsb	sy

__wfi:
	wfi
	ldr	r1, [r0]
	tst	r1, r1
	beq __wfi
	bx	r1
	@ Never reach this point

	@ GIC CPI IF address
GIC_CPU_IF:
	.word 0x2c002000

@ Store the entry point used at CPU wake-up
BOOT_REG:
	.word 0x1c010030

@ vExpress SRAM
DEST_ADDR:
	.word 0x14000000

__end:

save_boot_params:

	@ Check if we are on the primary (CPU #0)
	@ Read Multiprocessor ID register
    mrc 	p15, 0, r0, c0, c0, 5
	ands	r0, r0, #0x3
	beq		out_primary

	@ Relocate the code handling the secondary CPUs to SRAM
	@ thus allowing U-boot to be relocated itself in the DRAM
	@ without interfering with this code.

	ldr	r0, =__start
	ldr r1, DEST_ADDR

	ldr r2, __code_size

relocate:

	ldr r3, [r0], #1
	str	r3, [r1], #1

	subs r2, r2, #1
	bne	relocate

	@ Now, jump to the stalling code
	ldr	pc, DEST_ADDR

out_primary:

	/* Returns */
	b	save_boot_params_ret

__code_size:
	.word	__end - __start



