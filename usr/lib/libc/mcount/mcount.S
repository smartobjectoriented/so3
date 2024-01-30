	.globl _mcount
_mcount:
	// Prolog
	stp x29, x30, [sp, #-16]!
	mov x29, sp

	// Stack shift
	// Calculate end source address (caller frame record address)
	ldr x1, [x29]
	ldr x1, [x1]

	// Extend stack by 3 words and keep src and destination addresses for shift
	mov x2, sp // Source address
	sub sp, sp, #24 // Stack extension
	mov x3, sp // Destination address

	// Free stack space between caller and callee frame
loop:
	ldr x4, [x2]
	str x4, [x3]
	add x2, x2, #8
	add x3, x3, #8
	cmp x1, x2
	b.ne loop

	// Fix stack pointers and inject _mcount_exit
	// (_mcount_exit) Store caller frame record address
	str x1, [x3]	
	// (_mcount) Fix frame pointer
	sub x29, x29, #24	
	// (_mcount) Fix callee frame record address
	ldr x1, [x29]	
	sub x1, x1, #24
	str x1, [x29]
	// (callee) Inject _mcount_exit frame record address
	ldr x2, [x1]
	sub x2, x2, #24
	str x2, [x1]
	// (_mcount_exit) Use callee return address as own
	ldr x3, [x1, #8]
	str x3, [x2, #8]
	// (callee) Override return address
	adr x4, _mcount_exit
	str x4, [x1, #8]

	// Call C function
	bl mcount

	// Epilog
	ldp x29, x30, [sp], #16
	ret

	.globl _mcount_exit
_mcount_exit:
	// Save return value
	str x0, [sp, #16]

	// Recover return address and pass it to _mcount_exit
	ldr x0, [x29, #8]
	bl mcount_exit

	// Recover saved return value
	ldr x0, [sp, #16]

	// Epilog
	ldp x29, x30, [sp], #24
	ret 