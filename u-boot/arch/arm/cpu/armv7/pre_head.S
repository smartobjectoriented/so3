
@ pre_head is inserted before the main _start entry point
@ in order to jump into the EL3 monitor mode to run ATF (ARM trusted-firmware)


#define SUNXI_CPUX_CFG_BASE_A32		0x01700000

#define RVBARADDR0_L	0xA0
#define RVBARADDR0_H	0xA4

.global __pre_head

.section ".pre_head.text", "ax"
__pre_head:
	ldr r0, =SUNXI_CPUX_CFG_BASE_A32

	ldr r1, =0x44000
	str r1, [r0, #RVBARADDR0_L]
	mov r1, #0
	str r1, [r0, #RVBARADDR0_H]

	@ set cpu to AA64 execution state when the cpu boots into after a warm reset
	mrc p15, 0, r2, c12, c0, 2
	orr r2, r2, #0x3
	dsb
	mcr p15, 0, r2, c12, c0, 2
	isb

__LOOP:
	wfi
	b __LOOP
