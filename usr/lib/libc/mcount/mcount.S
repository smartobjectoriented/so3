	.globl _mcount
_mcount:
	@ Prolog
	stp x29, x30, [sp, #-16]!
	mov x29, sp

	@ Stack shift
	@ Calculate end source address (caller frame record address)
	ldr x0, [sp]
	ldr x0, [x0]

	@ Extend stack by 3 words and keep src and destination addresses
	mov x1, sp @ Destination address
	sub sp, sp, #24 @ Stack extension
	mov x2, sp @ Source address

	@ Free stack space between caller and callee frame
loop:
	ldr x3, [x2]
	str x3, [x1]
	add x1, x1, #8
	add x2, x2, #8
	cmp x0, x2
	b.ne loop


	@TODO Inject _mcount_exit
	@TODO Find callee frame record address (should be done when calculating end source address)
	ldr x0, 
	@ Set _mcount_exit as return function of callee
	adr x3, _mcount_exit
	str x3, [x2, #8]


	@ Fix frame and return addresses

	@ Epilog
	ldp x29, x30, [sp], #16
	ret

	.globl _mcount_exit
_mcount_exit:
	@ Save return value
	str x0, [sp, #16]

	@ Recover return address and pass it to _mcount_exit
	ldp x1, x0, [sp]
	bl _mcount_exit

	@ Recover saved return value
	ldr x0, [sp, #16]

	@ Epilog
	ldp x29, x30, [sp], #16
	ret 