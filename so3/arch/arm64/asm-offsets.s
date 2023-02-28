	.arch armv8-a
	.file	"asm-offsets.c"
// GNU C89 (GNU Toolchain for the A-profile Architecture 10.2-2020.11 (arm-10.16)) version 10.2.1 20201103 (aarch64-none-linux-gnu)
//	compiled by GNU C version 4.8.5 20150623 (Red Hat 4.8.5-39), GMP version 4.3.2, MPFR version 3.1.6, MPC version 1.0.3, isl version isl-0.15-1-g835ea3a-GMP

// GGC heuristics: --param ggc-min-expand=100 --param ggc-min-heapsize=131072
// options passed:  -nostdinc -I include
// -I /home/rossierd/soo.tech/so3/so3/include
// -I /home/rossierd/soo.tech/so3/so3/include/net -I .
// -I /home/rossierd/soo.tech/so3/so3/.
// -I /home/rossierd/soo.tech/so3/so3/lib/libfdt
// -I /home/rossierd/soo.tech/so3/so3/arch/arm64/include/
// -I /home/rossierd/soo.tech/so3/so3/arch/arm64/virt64/include/
// -iprefix /opt/toolchains/arm/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/bin/../lib/gcc/aarch64-none-linux-gnu/10.2.1/
// -isysroot /opt/toolchains/arm/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/bin/../aarch64-none-linux-gnu/libc
// -D __KERNEL__ -D __SO3__ -D BITS_PER_LONG=64
// -isystem /opt/toolchains/arm/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/bin/../lib/gcc/aarch64-none-linux-gnu/10.2.1/include
// -MD arch/arm64/.asm-offsets.s.d
// /home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c -mlittle-endian
// -mabi=lp64 -auxbase-strip arch/arm64/asm-offsets.s -g -Wall -Wundef
// -Wstrict-prototypes -Wno-trigraphs -Werror=implicit-function-declaration
// -Wno-format-security -Wno-frame-address -Wframe-larger-than=1024
// -Wno-unused-but-set-variable -Wunused-const-variable=0
// -Wdeclaration-after-statement -Wno-pointer-sign -Werror=implicit-int
// -Werror=strict-prototypes -Werror=date-time
// -Werror=incompatible-pointer-types -Werror=designated-init -std=gnu90
// -fno-builtin -ffreestanding -fno-strict-aliasing -fno-common -fno-PIE
// -fno-dwarf2-cfi-asm -fno-ipa-sra -funwind-tables
// -fno-delete-null-pointer-checks -fno-stack-protector
// -fomit-frame-pointer -fno-var-tracking-assignments -fno-strict-overflow
// -fno-merge-all-constants -fmerge-constants -fstack-check=no
// -fconserve-stack -fno-function-sections -fno-data-sections
// -fno-allow-store-data-races -fverbose-asm
// options enabled:  -faggressive-loop-optimizations -fallocation-dce
// -fasynchronous-unwind-tables -fauto-inc-dec -fearly-inlining
// -feliminate-unused-debug-symbols -feliminate-unused-debug-types
// -ffp-int-builtin-inexact -ffunction-cse -fgcse-lm -fgnu-unique -fident
// -finline-atomics -fipa-stack-alignment -fira-hoist-pressure
// -fira-share-save-slots -fira-share-spill-slots -fivopts
// -fkeep-static-consts -fleading-underscore -flifetime-dse -fmath-errno
// -fmerge-constants -fmerge-debug-strings -fomit-frame-pointer -fpeephole
// -fplt -fprefetch-loop-arrays -freg-struct-return
// -fsched-critical-path-heuristic -fsched-dep-count-heuristic
// -fsched-group-heuristic -fsched-interblock -fsched-last-insn-heuristic
// -fsched-rank-heuristic -fsched-spec -fsched-spec-insn-heuristic
// -fsched-stalled-insns-dep -fschedule-fusion -fsemantic-interposition
// -fshow-column -fshrink-wrap-separate -fsigned-zeros
// -fsplit-ivs-in-unroller -fssa-backprop -fstdarg-opt
// -fstrict-volatile-bitfields -fsync-libcalls -ftrapping-math
// -ftree-cselim -ftree-forwprop -ftree-loop-if-convert -ftree-loop-im
// -ftree-loop-ivcanon -ftree-loop-optimize -ftree-parallelize-loops=
// -ftree-phiprop -ftree-reassoc -ftree-scev-cprop -funit-at-a-time
// -funwind-tables -fverbose-asm -fwrapv -fwrapv-pointer
// -fzero-initialized-in-bss -mfix-cortex-a53-835769
// -mfix-cortex-a53-843419 -mglibc -mlittle-endian
// -momit-leaf-frame-pointer -moutline-atomics -mpc-relative-literal-loads

	.text
.Ltext0:
	.align	2
	.global	main
	.type	main, %function
main:
.LFB50:
	.file 1 "/home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c"
	.loc 1 44 1
// /home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c:60: 	BLANK();
	.loc 1 60 2
#APP
// 60 "/home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c" 1
	
->	
// 0 "" 2
// /home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c:62: 	DEFINE(OFFSET_X0,		offsetof(struct cpu_regs, x0));
	.loc 1 62 2
// 62 "/home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c" 1
	
->OFFSET_X0 0 offsetof(struct cpu_regs, x0)	//
// 0 "" 2
// /home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c:63: 	DEFINE(OFFSET_X1,		offsetof(struct cpu_regs, x1));
	.loc 1 63 2
// 63 "/home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c" 1
	
->OFFSET_X1 8 offsetof(struct cpu_regs, x1)	//
// 0 "" 2
// /home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c:64: 	DEFINE(OFFSET_X2,		offsetof(struct cpu_regs, x2));
	.loc 1 64 2
// 64 "/home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c" 1
	
->OFFSET_X2 16 offsetof(struct cpu_regs, x2)	//
// 0 "" 2
// /home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c:65: 	DEFINE(OFFSET_X3,		offsetof(struct cpu_regs, x3));
	.loc 1 65 2
// 65 "/home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c" 1
	
->OFFSET_X3 24 offsetof(struct cpu_regs, x3)	//
// 0 "" 2
// /home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c:66: 	DEFINE(OFFSET_X4,		offsetof(struct cpu_regs, x4));
	.loc 1 66 2
// 66 "/home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c" 1
	
->OFFSET_X4 32 offsetof(struct cpu_regs, x4)	//
// 0 "" 2
// /home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c:67: 	DEFINE(OFFSET_X5,		offsetof(struct cpu_regs, x5));
	.loc 1 67 2
// 67 "/home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c" 1
	
->OFFSET_X5 40 offsetof(struct cpu_regs, x5)	//
// 0 "" 2
// /home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c:68: 	DEFINE(OFFSET_X6,		offsetof(struct cpu_regs, x6));
	.loc 1 68 2
// 68 "/home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c" 1
	
->OFFSET_X6 48 offsetof(struct cpu_regs, x6)	//
// 0 "" 2
// /home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c:69: 	DEFINE(OFFSET_X7,		offsetof(struct cpu_regs, x7));
	.loc 1 69 2
// 69 "/home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c" 1
	
->OFFSET_X7 56 offsetof(struct cpu_regs, x7)	//
// 0 "" 2
// /home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c:70: 	DEFINE(OFFSET_X8,		offsetof(struct cpu_regs, x8));
	.loc 1 70 2
// 70 "/home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c" 1
	
->OFFSET_X8 64 offsetof(struct cpu_regs, x8)	//
// 0 "" 2
// /home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c:71: 	DEFINE(OFFSET_X9,		offsetof(struct cpu_regs, x9));
	.loc 1 71 2
// 71 "/home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c" 1
	
->OFFSET_X9 72 offsetof(struct cpu_regs, x9)	//
// 0 "" 2
// /home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c:72: 	DEFINE(OFFSET_X10,		offsetof(struct cpu_regs, x10));
	.loc 1 72 2
// 72 "/home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c" 1
	
->OFFSET_X10 80 offsetof(struct cpu_regs, x10)	//
// 0 "" 2
// /home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c:73: 	DEFINE(OFFSET_X11,		offsetof(struct cpu_regs, x11));
	.loc 1 73 2
// 73 "/home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c" 1
	
->OFFSET_X11 88 offsetof(struct cpu_regs, x11)	//
// 0 "" 2
// /home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c:74: 	DEFINE(OFFSET_X12,		offsetof(struct cpu_regs, x12));
	.loc 1 74 2
// 74 "/home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c" 1
	
->OFFSET_X12 96 offsetof(struct cpu_regs, x12)	//
// 0 "" 2
// /home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c:75: 	DEFINE(OFFSET_X13,		offsetof(struct cpu_regs, x13));
	.loc 1 75 2
// 75 "/home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c" 1
	
->OFFSET_X13 104 offsetof(struct cpu_regs, x13)	//
// 0 "" 2
// /home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c:76: 	DEFINE(OFFSET_X14,		offsetof(struct cpu_regs, x14));
	.loc 1 76 2
// 76 "/home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c" 1
	
->OFFSET_X14 112 offsetof(struct cpu_regs, x14)	//
// 0 "" 2
// /home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c:77: 	DEFINE(OFFSET_X15,		offsetof(struct cpu_regs, x15));
	.loc 1 77 2
// 77 "/home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c" 1
	
->OFFSET_X15 120 offsetof(struct cpu_regs, x15)	//
// 0 "" 2
// /home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c:78: 	DEFINE(OFFSET_X16,		offsetof(struct cpu_regs, x16));
	.loc 1 78 2
// 78 "/home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c" 1
	
->OFFSET_X16 128 offsetof(struct cpu_regs, x16)	//
// 0 "" 2
// /home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c:79: 	DEFINE(OFFSET_X17,		offsetof(struct cpu_regs, x17));
	.loc 1 79 2
// 79 "/home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c" 1
	
->OFFSET_X17 136 offsetof(struct cpu_regs, x17)	//
// 0 "" 2
// /home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c:80: 	DEFINE(OFFSET_X18,		offsetof(struct cpu_regs, x18));
	.loc 1 80 2
// 80 "/home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c" 1
	
->OFFSET_X18 144 offsetof(struct cpu_regs, x18)	//
// 0 "" 2
// /home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c:81: 	DEFINE(OFFSET_X19,		offsetof(struct cpu_regs, x19));
	.loc 1 81 2
// 81 "/home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c" 1
	
->OFFSET_X19 152 offsetof(struct cpu_regs, x19)	//
// 0 "" 2
// /home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c:82: 	DEFINE(OFFSET_X20,		offsetof(struct cpu_regs, x20));
	.loc 1 82 2
// 82 "/home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c" 1
	
->OFFSET_X20 160 offsetof(struct cpu_regs, x20)	//
// 0 "" 2
// /home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c:83: 	DEFINE(OFFSET_X21,		offsetof(struct cpu_regs, x21));
	.loc 1 83 2
// 83 "/home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c" 1
	
->OFFSET_X21 168 offsetof(struct cpu_regs, x21)	//
// 0 "" 2
// /home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c:84: 	DEFINE(OFFSET_X22,		offsetof(struct cpu_regs, x22));
	.loc 1 84 2
// 84 "/home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c" 1
	
->OFFSET_X22 176 offsetof(struct cpu_regs, x22)	//
// 0 "" 2
// /home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c:85: 	DEFINE(OFFSET_X23,		offsetof(struct cpu_regs, x23));
	.loc 1 85 2
// 85 "/home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c" 1
	
->OFFSET_X23 184 offsetof(struct cpu_regs, x23)	//
// 0 "" 2
// /home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c:86: 	DEFINE(OFFSET_X24,		offsetof(struct cpu_regs, x24));
	.loc 1 86 2
// 86 "/home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c" 1
	
->OFFSET_X24 192 offsetof(struct cpu_regs, x24)	//
// 0 "" 2
// /home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c:87: 	DEFINE(OFFSET_X25,		offsetof(struct cpu_regs, x25));
	.loc 1 87 2
// 87 "/home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c" 1
	
->OFFSET_X25 200 offsetof(struct cpu_regs, x25)	//
// 0 "" 2
// /home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c:88: 	DEFINE(OFFSET_X26,		offsetof(struct cpu_regs, x26));
	.loc 1 88 2
// 88 "/home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c" 1
	
->OFFSET_X26 208 offsetof(struct cpu_regs, x26)	//
// 0 "" 2
// /home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c:89: 	DEFINE(OFFSET_X27,		offsetof(struct cpu_regs, x27));
	.loc 1 89 2
// 89 "/home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c" 1
	
->OFFSET_X27 216 offsetof(struct cpu_regs, x27)	//
// 0 "" 2
// /home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c:90: 	DEFINE(OFFSET_X28,		offsetof(struct cpu_regs, x28));
	.loc 1 90 2
// 90 "/home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c" 1
	
->OFFSET_X28 224 offsetof(struct cpu_regs, x28)	//
// 0 "" 2
// /home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c:91: 	DEFINE(OFFSET_FP,		offsetof(struct cpu_regs, fp));
	.loc 1 91 2
// 91 "/home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c" 1
	
->OFFSET_FP 232 offsetof(struct cpu_regs, fp)	//
// 0 "" 2
// /home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c:92: 	DEFINE(OFFSET_LR,		offsetof(struct cpu_regs, lr));
	.loc 1 92 2
// 92 "/home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c" 1
	
->OFFSET_LR 240 offsetof(struct cpu_regs, lr)	//
// 0 "" 2
// /home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c:93: 	DEFINE(OFFSET_SP,		offsetof(struct cpu_regs, sp));
	.loc 1 93 2
// 93 "/home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c" 1
	
->OFFSET_SP 248 offsetof(struct cpu_regs, sp)	//
// 0 "" 2
// /home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c:94: 	DEFINE(OFFSET_PC,		offsetof(struct cpu_regs, pc));
	.loc 1 94 2
// 94 "/home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c" 1
	
->OFFSET_PC 256 offsetof(struct cpu_regs, pc)	//
// 0 "" 2
// /home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c:95: 	DEFINE(OFFSET_PSTATE,		offsetof(struct cpu_regs, pstate));
	.loc 1 95 2
// 95 "/home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c" 1
	
->OFFSET_PSTATE 264 offsetof(struct cpu_regs, pstate)	//
// 0 "" 2
// /home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c:97: 	BLANK();
	.loc 1 97 2
// 97 "/home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c" 1
	
->	
// 0 "" 2
// /home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c:99: 	DEFINE(ARM_SMCCC_RES_X0_OFFS,		offsetof(struct arm_smccc_res, a0));
	.loc 1 99 2
// 99 "/home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c" 1
	
->ARM_SMCCC_RES_X0_OFFS 0 offsetof(struct arm_smccc_res, a0)	//
// 0 "" 2
// /home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c:100: 	DEFINE(ARM_SMCCC_RES_X2_OFFS,		offsetof(struct arm_smccc_res, a2));
	.loc 1 100 2
// 100 "/home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c" 1
	
->ARM_SMCCC_RES_X2_OFFS 16 offsetof(struct arm_smccc_res, a2)	//
// 0 "" 2
// /home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c:101: 	DEFINE(ARM_SMCCC_QUIRK_ID_OFFS,	offsetof(struct arm_smccc_quirk, id));
	.loc 1 101 2
// 101 "/home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c" 1
	
->ARM_SMCCC_QUIRK_ID_OFFS 0 offsetof(struct arm_smccc_quirk, id)	//
// 0 "" 2
// /home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c:102: 	DEFINE(ARM_SMCCC_QUIRK_STATE_OFFS,	offsetof(struct arm_smccc_quirk, state));
	.loc 1 102 2
// 102 "/home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c" 1
	
->ARM_SMCCC_QUIRK_STATE_OFFS 8 offsetof(struct arm_smccc_quirk, state)	//
// 0 "" 2
// /home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c:104: 	BLANK();
	.loc 1 104 2
// 104 "/home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c" 1
	
->	
// 0 "" 2
// /home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c:106: 	DEFINE(OFFSET_TCB_CPU_REGS, 	offsetof(tcb_t, cpu_regs));
	.loc 1 106 2
// 106 "/home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c" 1
	
->OFFSET_TCB_CPU_REGS 176 offsetof(tcb_t, cpu_regs)	//
// 0 "" 2
// /home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c:108: 	return 0;
	.loc 1 108 9
#NO_APP
	mov	w0, 0	// _1,
// /home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c:109: }
	.loc 1 109 1
	ret	
.LFE50:
	.size	main, .-main
	.section	.debug_frame,"",@progbits
.Lframe0:
	.4byte	.LECIE0-.LSCIE0
.LSCIE0:
	.4byte	0xffffffff
	.byte	0x3
	.string	""
	.uleb128 0x1
	.sleb128 -8
	.uleb128 0x1e
	.byte	0xc
	.uleb128 0x1f
	.uleb128 0
	.align	3
.LECIE0:
.LSFDE0:
	.4byte	.LEFDE0-.LASFDE0
.LASFDE0:
	.4byte	.Lframe0
	.8byte	.LFB50
	.8byte	.LFE50-.LFB50
	.align	3
.LEFDE0:
	.section	.eh_frame,"a",@progbits
.Lframe1:
	.4byte	.LECIE1-.LSCIE1
.LSCIE1:
	.4byte	0
	.byte	0x3
	.string	"zR"
	.uleb128 0x1
	.sleb128 -8
	.uleb128 0x1e
	.uleb128 0x1
	.byte	0x1b
	.byte	0xc
	.uleb128 0x1f
	.uleb128 0
	.align	3
.LECIE1:
.LSFDE3:
	.4byte	.LEFDE3-.LASFDE3
.LASFDE3:
	.4byte	.LASFDE3-.Lframe1
	.4byte	.LFB50-.
	.4byte	.LFE50-.LFB50
	.uleb128 0
	.align	3
.LEFDE3:
	.text
.Letext0:
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.4byte	0x87
	.2byte	0x4
	.4byte	.Ldebug_abbrev0
	.byte	0x8
	.uleb128 0x1
	.4byte	.LASF8
	.byte	0x1
	.4byte	.LASF9
	.4byte	.LASF10
	.8byte	.Ltext0
	.8byte	.Letext0-.Ltext0
	.4byte	.Ldebug_line0
	.uleb128 0x2
	.byte	0x2
	.byte	0x7
	.4byte	.LASF0
	.uleb128 0x2
	.byte	0x1
	.byte	0x6
	.4byte	.LASF1
	.uleb128 0x2
	.byte	0x1
	.byte	0x8
	.4byte	.LASF2
	.uleb128 0x2
	.byte	0x2
	.byte	0x5
	.4byte	.LASF3
	.uleb128 0x3
	.byte	0x4
	.byte	0x5
	.string	"int"
	.uleb128 0x2
	.byte	0x4
	.byte	0x7
	.4byte	.LASF4
	.uleb128 0x2
	.byte	0x8
	.byte	0x5
	.4byte	.LASF5
	.uleb128 0x2
	.byte	0x8
	.byte	0x7
	.4byte	.LASF6
	.uleb128 0x2
	.byte	0x1
	.byte	0x8
	.4byte	.LASF7
	.uleb128 0x4
	.4byte	.LASF11
	.byte	0x1
	.byte	0x2b
	.byte	0x5
	.4byte	0x49
	.8byte	.LFB50
	.8byte	.LFE50-.LFB50
	.uleb128 0x1
	.byte	0x9c
	.byte	0
	.section	.debug_abbrev,"",@progbits
.Ldebug_abbrev0:
	.uleb128 0x1
	.uleb128 0x11
	.byte	0x1
	.uleb128 0x25
	.uleb128 0xe
	.uleb128 0x13
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x1b
	.uleb128 0xe
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x7
	.uleb128 0x10
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0x2
	.uleb128 0x24
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0xe
	.byte	0
	.byte	0
	.uleb128 0x3
	.uleb128 0x24
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0x8
	.byte	0
	.byte	0
	.uleb128 0x4
	.uleb128 0x2e
	.byte	0
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x39
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0x19
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x7
	.uleb128 0x40
	.uleb128 0x18
	.uleb128 0x2117
	.uleb128 0x19
	.byte	0
	.byte	0
	.byte	0
	.section	.debug_aranges,"",@progbits
	.4byte	0x2c
	.2byte	0x2
	.4byte	.Ldebug_info0
	.byte	0x8
	.byte	0
	.2byte	0
	.2byte	0
	.8byte	.Ltext0
	.8byte	.Letext0-.Ltext0
	.8byte	0
	.8byte	0
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.section	.debug_str,"MS",@progbits,1
.LASF10:
	.string	"/home/rossierd/soo.tech/so3/so3"
.LASF4:
	.string	"unsigned int"
.LASF6:
	.string	"long unsigned int"
.LASF1:
	.string	"signed char"
.LASF0:
	.string	"short unsigned int"
.LASF9:
	.string	"/home/rossierd/soo.tech/so3/so3/arch/arm64/asm-offsets.c"
.LASF3:
	.string	"short int"
.LASF8:
	.ascii	"GNU C89 10.2.1 20201103 -mlittle-endian -mabi=lp64 -g -std=g"
	.ascii	"nu90 -fno-builtin -ffreestanding -fno-strict-aliasing -fno-c"
	.ascii	"ommon -fno-PIE -fno-dwarf2-cfi-asm -fno-ipa-sra -funwind-tab"
	.ascii	"les -fno-delete-nul"
	.string	"l-pointer-checks -fno-stack-protector -fomit-frame-pointer -fno-var-tracking-assignments -fno-strict-overflow -fno-merge-all-constants -fmerge-constants -fstack-check=no -fconserve-stack -fno-function-sections -fno-data-sections -fno-allow-store-data-races"
.LASF2:
	.string	"unsigned char"
.LASF5:
	.string	"long int"
.LASF11:
	.string	"main"
.LASF7:
	.string	"char"
	.ident	"GCC: (GNU Toolchain for the A-profile Architecture 10.2-2020.11 (arm-10.16)) 10.2.1 20201103"
	.section	.note.GNU-stack,"",@progbits
