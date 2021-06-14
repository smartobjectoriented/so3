	.arch armv7-a
	.eabi_attribute 28, 1	@ Tag_ABI_VFP_args
	.eabi_attribute 20, 1	@ Tag_ABI_FP_denormal
	.eabi_attribute 21, 1	@ Tag_ABI_FP_exceptions
	.eabi_attribute 23, 3	@ Tag_ABI_FP_number_model
	.eabi_attribute 24, 1	@ Tag_ABI_align8_needed
	.eabi_attribute 25, 1	@ Tag_ABI_align8_preserved
	.eabi_attribute 26, 2	@ Tag_ABI_enum_size
	.eabi_attribute 30, 6	@ Tag_ABI_optimization_goals
	.eabi_attribute 34, 1	@ Tag_CPU_unaligned_access
	.eabi_attribute 18, 4	@ Tag_ABI_PCS_wchar_t
	.file	"asm-offsets.c"
@ GNU C89 (GNU Toolchain for the A-profile Architecture 9.2-2019.12 (arm-9.10)) version 9.2.1 20191025 (arm-none-linux-gnueabihf)
@	compiled by GNU C version 4.8.1, GMP version 4.3.2, MPFR version 3.1.6, MPC version 1.0.3, isl version isl-0.15-1-g835ea3a-GMP

@ GGC heuristics: --param ggc-min-expand=100 --param ggc-min-heapsize=131072
@ options passed:  -nostdinc -I include -I . -I include -I include/net -I .
@ -I ./lib/libfdt -I arch/arm32/include/ -I arch/arm32/rpi4/include/
@ -iprefix /opt/toolchain/arm/gcc-arm-9.2-2019.12-x86_64-arm-none-linux-gnueabihf/bin/../lib/gcc/arm-none-linux-gnueabihf/9.2.1/
@ -isysroot /opt/toolchain/arm/gcc-arm-9.2-2019.12-x86_64-arm-none-linux-gnueabihf/bin/../arm-none-linux-gnueabihf/libc
@ -D __KERNEL__ -U arm -D BITS_PER_LONG=32 -D KBUILD_STR(s)=#s
@ -D KBUILD_BASENAME=KBUILD_STR(asm_offsets)
@ -include include/generated/autoconf.h
@ -isystem /opt/toolchain/arm/gcc-arm-9.2-2019.12-x86_64-arm-none-linux-gnueabihf/bin/../lib/gcc/arm-none-linux-gnueabihf/9.2.1/include
@ -MD arch/arm32/.asm-offsets.s.d arch/arm32/asm-offsets.c -mlittle-endian
@ -mabi=aapcs-linux -mabi=aapcs-linux -mno-thumb-interwork -mfpu=vfp -marm
@ -mfloat-abi=hard -mtls-dialect=gnu -march=armv7-a+fp
@ -auxbase-strip arch/arm32/asm-offsets.s -g -Wall -Wundef
@ -Wstrict-prototypes -Wno-trigraphs -Werror=implicit-function-declaration
@ -Wno-format-security -Wno-frame-address -Wframe-larger-than=1024
@ -Wno-unused-but-set-variable -Wunused-const-variable=0
@ -Wdeclaration-after-statement -Wno-pointer-sign -Werror=implicit-int
@ -Werror=strict-prototypes -Werror=date-time
@ -Werror=incompatible-pointer-types -Werror=designated-init -std=gnu90
@ -fno-builtin -ffreestanding -fno-strict-aliasing -fno-common -fno-PIE
@ -fno-dwarf2-cfi-asm -fno-ipa-sra -fno-delete-null-pointer-checks
@ -fno-stack-protector -fomit-frame-pointer -fno-var-tracking-assignments
@ -fno-strict-overflow -fno-merge-all-constants -fmerge-constants
@ -fstack-check=no -fconserve-stack -fno-function-sections
@ -fno-data-sections -funwind-tables -fverbose-asm
@ --param allow-store-data-races=0
@ options enabled:  -faggressive-loop-optimizations -fassume-phsa
@ -fauto-inc-dec -fearly-inlining -feliminate-unused-debug-types
@ -ffp-int-builtin-inexact -ffunction-cse -fgcse-lm -fgnu-runtime
@ -fgnu-unique -fident -finline-atomics -fipa-stack-alignment
@ -fira-hoist-pressure -fira-share-save-slots -fira-share-spill-slots
@ -fivopts -fkeep-static-consts -fleading-underscore -flifetime-dse
@ -flto-odr-type-merging -fmath-errno -fmerge-constants
@ -fmerge-debug-strings -fomit-frame-pointer -fpeephole -fplt
@ -fprefetch-loop-arrays -freg-struct-return
@ -fsched-critical-path-heuristic -fsched-dep-count-heuristic
@ -fsched-group-heuristic -fsched-interblock -fsched-last-insn-heuristic
@ -fsched-rank-heuristic -fsched-spec -fsched-spec-insn-heuristic
@ -fsched-stalled-insns-dep -fsemantic-interposition -fshow-column
@ -fshrink-wrap-separate -fsigned-zeros -fsplit-ivs-in-unroller
@ -fssa-backprop -fstdarg-opt -fstrict-volatile-bitfields -fsync-libcalls
@ -ftrapping-math -ftree-cselim -ftree-forwprop -ftree-loop-if-convert
@ -ftree-loop-im -ftree-loop-ivcanon -ftree-loop-optimize
@ -ftree-parallelize-loops= -ftree-phiprop -ftree-reassoc -ftree-scev-cprop
@ -funit-at-a-time -funwind-tables -fverbose-asm -fwrapv -fwrapv-pointer
@ -fzero-initialized-in-bss -marm -mbe32 -mglibc -mlittle-endian
@ -mpic-data-is-text-relative -msched-prolog -munaligned-access
@ -mvectorize-with-neon-quad

	.text
.Ltext0:
	.align	2
	.global	main
	.arch armv7-a
	.syntax unified
	.arm
	.fpu vfp
	.type	main, %function
main:
	.fnstart
.LFB45:
	.file 1 "arch/arm32/asm-offsets.c"
	.loc 1 52 1
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
@ arch/arm32/asm-offsets.c:53: 	BLANK();
	.loc 1 53 2
	.syntax divided
@ 53 "arch/arm32/asm-offsets.c" 1
	
->	
@ 0 "" 2
@ arch/arm32/asm-offsets.c:55: 	DEFINE(OFFSET_TCB_CPU_REGS, 	offsetof(tcb_t, cpu_regs));
	.loc 1 55 2
@ 55 "arch/arm32/asm-offsets.c" 1
	
->OFFSET_TCB_CPU_REGS #140 offsetof(tcb_t, cpu_regs)	@
@ 0 "" 2
@ arch/arm32/asm-offsets.c:57: 	BLANK();
	.loc 1 57 2
@ 57 "arch/arm32/asm-offsets.c" 1
	
->	
@ 0 "" 2
@ arch/arm32/asm-offsets.c:59: 	DEFINE(OFFSET_R0,			offsetof(cpu_regs_t, r0));
	.loc 1 59 2
@ 59 "arch/arm32/asm-offsets.c" 1
	
->OFFSET_R0 #0 offsetof(cpu_regs_t, r0)	@
@ 0 "" 2
@ arch/arm32/asm-offsets.c:60: 	DEFINE(OFFSET_R1,			offsetof(cpu_regs_t, r1));
	.loc 1 60 2
@ 60 "arch/arm32/asm-offsets.c" 1
	
->OFFSET_R1 #4 offsetof(cpu_regs_t, r1)	@
@ 0 "" 2
@ arch/arm32/asm-offsets.c:61: 	DEFINE(OFFSET_R2,			offsetof(cpu_regs_t, r2));
	.loc 1 61 2
@ 61 "arch/arm32/asm-offsets.c" 1
	
->OFFSET_R2 #8 offsetof(cpu_regs_t, r2)	@
@ 0 "" 2
@ arch/arm32/asm-offsets.c:62: 	DEFINE(OFFSET_R3,			offsetof(cpu_regs_t, r3));
	.loc 1 62 2
@ 62 "arch/arm32/asm-offsets.c" 1
	
->OFFSET_R3 #12 offsetof(cpu_regs_t, r3)	@
@ 0 "" 2
@ arch/arm32/asm-offsets.c:63: 	DEFINE(OFFSET_R4,			offsetof(cpu_regs_t, r4));
	.loc 1 63 2
@ 63 "arch/arm32/asm-offsets.c" 1
	
->OFFSET_R4 #16 offsetof(cpu_regs_t, r4)	@
@ 0 "" 2
@ arch/arm32/asm-offsets.c:64: 	DEFINE(OFFSET_R5,			offsetof(cpu_regs_t, r5));
	.loc 1 64 2
@ 64 "arch/arm32/asm-offsets.c" 1
	
->OFFSET_R5 #20 offsetof(cpu_regs_t, r5)	@
@ 0 "" 2
@ arch/arm32/asm-offsets.c:65: 	DEFINE(OFFSET_R6,			offsetof(cpu_regs_t, r6));
	.loc 1 65 2
@ 65 "arch/arm32/asm-offsets.c" 1
	
->OFFSET_R6 #24 offsetof(cpu_regs_t, r6)	@
@ 0 "" 2
@ arch/arm32/asm-offsets.c:66: 	DEFINE(OFFSET_R7,			offsetof(cpu_regs_t, r7));
	.loc 1 66 2
@ 66 "arch/arm32/asm-offsets.c" 1
	
->OFFSET_R7 #28 offsetof(cpu_regs_t, r7)	@
@ 0 "" 2
@ arch/arm32/asm-offsets.c:67: 	DEFINE(OFFSET_R8,			offsetof(cpu_regs_t, r8));
	.loc 1 67 2
@ 67 "arch/arm32/asm-offsets.c" 1
	
->OFFSET_R8 #32 offsetof(cpu_regs_t, r8)	@
@ 0 "" 2
@ arch/arm32/asm-offsets.c:68: 	DEFINE(OFFSET_R9,			offsetof(cpu_regs_t, r9));
	.loc 1 68 2
@ 68 "arch/arm32/asm-offsets.c" 1
	
->OFFSET_R9 #36 offsetof(cpu_regs_t, r9)	@
@ 0 "" 2
@ arch/arm32/asm-offsets.c:69: 	DEFINE(OFFSET_R10,			offsetof(cpu_regs_t, r10));
	.loc 1 69 2
@ 69 "arch/arm32/asm-offsets.c" 1
	
->OFFSET_R10 #40 offsetof(cpu_regs_t, r10)	@
@ 0 "" 2
@ arch/arm32/asm-offsets.c:70: 	DEFINE(OFFSET_FP,			offsetof(cpu_regs_t, fp));
	.loc 1 70 2
@ 70 "arch/arm32/asm-offsets.c" 1
	
->OFFSET_FP #44 offsetof(cpu_regs_t, fp)	@
@ 0 "" 2
@ arch/arm32/asm-offsets.c:71: 	DEFINE(OFFSET_IP,			offsetof(cpu_regs_t, ip));
	.loc 1 71 2
@ 71 "arch/arm32/asm-offsets.c" 1
	
->OFFSET_IP #48 offsetof(cpu_regs_t, ip)	@
@ 0 "" 2
@ arch/arm32/asm-offsets.c:72: 	DEFINE(OFFSET_SP,			offsetof(cpu_regs_t, sp));
	.loc 1 72 2
@ 72 "arch/arm32/asm-offsets.c" 1
	
->OFFSET_SP #52 offsetof(cpu_regs_t, sp)	@
@ 0 "" 2
@ arch/arm32/asm-offsets.c:73: 	DEFINE(OFFSET_LR,			offsetof(cpu_regs_t, lr));
	.loc 1 73 2
@ 73 "arch/arm32/asm-offsets.c" 1
	
->OFFSET_LR #56 offsetof(cpu_regs_t, lr)	@
@ 0 "" 2
@ arch/arm32/asm-offsets.c:74: 	DEFINE(OFFSET_PC,			offsetof(cpu_regs_t, pc));
	.loc 1 74 2
@ 74 "arch/arm32/asm-offsets.c" 1
	
->OFFSET_PC #60 offsetof(cpu_regs_t, pc)	@
@ 0 "" 2
@ arch/arm32/asm-offsets.c:75: 	DEFINE(OFFSET_PSR,			offsetof(cpu_regs_t, psr));
	.loc 1 75 2
@ 75 "arch/arm32/asm-offsets.c" 1
	
->OFFSET_PSR #64 offsetof(cpu_regs_t, psr)	@
@ 0 "" 2
@ arch/arm32/asm-offsets.c:76: 	DEFINE(OFFSET_SP_USR,			offsetof(cpu_regs_t, sp_usr));
	.loc 1 76 2
@ 76 "arch/arm32/asm-offsets.c" 1
	
->OFFSET_SP_USR #68 offsetof(cpu_regs_t, sp_usr)	@
@ 0 "" 2
@ arch/arm32/asm-offsets.c:77: 	DEFINE(OFFSET_LR_USR,			offsetof(cpu_regs_t, lr_usr));
	.loc 1 77 2
@ 77 "arch/arm32/asm-offsets.c" 1
	
->OFFSET_LR_USR #72 offsetof(cpu_regs_t, lr_usr)	@
@ 0 "" 2
@ arch/arm32/asm-offsets.c:79: 	BLANK();
	.loc 1 79 2
@ 79 "arch/arm32/asm-offsets.c" 1
	
->	
@ 0 "" 2
@ arch/arm32/asm-offsets.c:81: 	DEFINE(OFFSET_SYS_SIGNUM,		offsetof(__sigaction_t, signum));
	.loc 1 81 2
@ 81 "arch/arm32/asm-offsets.c" 1
	
->OFFSET_SYS_SIGNUM #0 offsetof(__sigaction_t, signum)	@
@ 0 "" 2
@ arch/arm32/asm-offsets.c:82: 	DEFINE(OFFSET_SYS_SA,			offsetof(__sigaction_t, sa));
	.loc 1 82 2
@ 82 "arch/arm32/asm-offsets.c" 1
	
->OFFSET_SYS_SA #4 offsetof(__sigaction_t, sa)	@
@ 0 "" 2
@ arch/arm32/asm-offsets.c:84: 	BLANK();
	.loc 1 84 2
@ 84 "arch/arm32/asm-offsets.c" 1
	
->	
@ 0 "" 2
@ arch/arm32/asm-offsets.c:86: 	DEFINE(OFFSET_SA_HANDLER,		offsetof(sigaction_t, sa_handler));
	.loc 1 86 2
@ 86 "arch/arm32/asm-offsets.c" 1
	
->OFFSET_SA_HANDLER #0 offsetof(sigaction_t, sa_handler)	@
@ 0 "" 2
@ arch/arm32/asm-offsets.c:87: 	DEFINE(OFFSET_SA_RESTORER,		offsetof(sigaction_t, sa_restorer));
	.loc 1 87 2
@ 87 "arch/arm32/asm-offsets.c" 1
	
->OFFSET_SA_RESTORER #8 offsetof(sigaction_t, sa_restorer)	@
@ 0 "" 2
@ arch/arm32/asm-offsets.c:89: 	return 0;
	.loc 1 89 9
	.arm
	.syntax unified
	mov	r3, #0	@ _1,
@ arch/arm32/asm-offsets.c:90: }
	.loc 1 90 1
	mov	r0, r3	@, <retval>
	bx	lr	@
.LFE45:
	.fnend
	.size	main, .-main
	.section	.debug_frame,"",%progbits
.Lframe0:
	.4byte	.LECIE0-.LSCIE0
.LSCIE0:
	.4byte	0xffffffff
	.byte	0x3
	.ascii	"\000"
	.uleb128 0x1
	.sleb128 -4
	.uleb128 0xe
	.byte	0xc
	.uleb128 0xd
	.uleb128 0
	.align	2
.LECIE0:
.LSFDE0:
	.4byte	.LEFDE0-.LASFDE0
.LASFDE0:
	.4byte	.Lframe0
	.4byte	.LFB45
	.4byte	.LFE45-.LFB45
	.align	2
.LEFDE0:
	.text
.Letext0:
	.file 2 "arch/arm32/include/asm/types.h"
	.file 3 "include/types.h"
	.file 4 "arch/arm32/include/asm/memory.h"
	.file 5 "include/common.h"
	.file 6 "include/thread.h"
	.section	.debug_info,"",%progbits
.Ldebug_info0:
	.4byte	0x111
	.2byte	0x4
	.4byte	.Ldebug_abbrev0
	.byte	0x4
	.uleb128 0x1
	.4byte	.LASF22
	.byte	0x1
	.4byte	.LASF23
	.4byte	.LASF24
	.4byte	.Ltext0
	.4byte	.Letext0-.Ltext0
	.4byte	.Ldebug_line0
	.uleb128 0x2
	.byte	0x4
	.byte	0x7
	.4byte	.LASF0
	.uleb128 0x2
	.byte	0x2
	.byte	0x7
	.4byte	.LASF1
	.uleb128 0x2
	.byte	0x1
	.byte	0x6
	.4byte	.LASF2
	.uleb128 0x2
	.byte	0x1
	.byte	0x8
	.4byte	.LASF3
	.uleb128 0x2
	.byte	0x2
	.byte	0x5
	.4byte	.LASF4
	.uleb128 0x3
	.byte	0x4
	.byte	0x5
	.ascii	"int\000"
	.uleb128 0x4
	.4byte	.LASF8
	.byte	0x2
	.byte	0x26
	.byte	0x16
	.4byte	0x5b
	.uleb128 0x2
	.byte	0x4
	.byte	0x7
	.4byte	.LASF5
	.uleb128 0x2
	.byte	0x8
	.byte	0x5
	.4byte	.LASF6
	.uleb128 0x2
	.byte	0x8
	.byte	0x7
	.4byte	.LASF7
	.uleb128 0x4
	.4byte	.LASF9
	.byte	0x3
	.byte	0x3f
	.byte	0x11
	.4byte	0x4f
	.uleb128 0x5
	.4byte	.LASF11
	.byte	0x4
	.byte	0x1b
	.byte	0x12
	.4byte	0x88
	.uleb128 0x6
	.byte	0x4
	.4byte	0x70
	.uleb128 0x2
	.byte	0x1
	.byte	0x8
	.4byte	.LASF10
	.uleb128 0x5
	.4byte	.LASF12
	.byte	0x5
	.byte	0x1b
	.byte	0x11
	.4byte	0x70
	.uleb128 0x7
	.byte	0x7
	.byte	0x4
	.4byte	0x5b
	.byte	0x5
	.byte	0x5c
	.byte	0xe
	.4byte	0xce
	.uleb128 0x8
	.4byte	.LASF13
	.byte	0
	.uleb128 0x8
	.4byte	.LASF14
	.byte	0x1
	.uleb128 0x8
	.4byte	.LASF15
	.byte	0x2
	.uleb128 0x8
	.4byte	.LASF16
	.byte	0x3
	.uleb128 0x8
	.4byte	.LASF17
	.byte	0x4
	.byte	0
	.uleb128 0x4
	.4byte	.LASF18
	.byte	0x5
	.byte	0x5e
	.byte	0x3
	.4byte	0xa1
	.uleb128 0x5
	.4byte	.LASF19
	.byte	0x5
	.byte	0x5f
	.byte	0x15
	.4byte	0xce
	.uleb128 0x5
	.4byte	.LASF20
	.byte	0x5
	.byte	0x64
	.byte	0x11
	.4byte	0x70
	.uleb128 0x5
	.4byte	.LASF21
	.byte	0x6
	.byte	0x2a
	.byte	0x15
	.4byte	0x5b
	.uleb128 0x9
	.4byte	.LASF25
	.byte	0x1
	.byte	0x33
	.byte	0x5
	.4byte	0x48
	.4byte	.LFB45
	.4byte	.LFE45-.LFB45
	.uleb128 0x1
	.byte	0x9c
	.byte	0
	.section	.debug_abbrev,"",%progbits
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
	.uleb128 0x6
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
	.uleb128 0x16
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x39
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x5
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x39
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3c
	.uleb128 0x19
	.byte	0
	.byte	0
	.uleb128 0x6
	.uleb128 0xf
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x7
	.uleb128 0x4
	.byte	0x1
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x39
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x8
	.uleb128 0x28
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x1c
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0x9
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
	.uleb128 0x6
	.uleb128 0x40
	.uleb128 0x18
	.uleb128 0x2117
	.uleb128 0x19
	.byte	0
	.byte	0
	.byte	0
	.section	.debug_aranges,"",%progbits
	.4byte	0x1c
	.2byte	0x2
	.4byte	.Ldebug_info0
	.byte	0x4
	.byte	0
	.2byte	0
	.2byte	0
	.4byte	.Ltext0
	.4byte	.Letext0-.Ltext0
	.4byte	0
	.4byte	0
	.section	.debug_line,"",%progbits
.Ldebug_line0:
	.section	.debug_str,"MS",%progbits,1
.LASF8:
	.ascii	"__u32\000"
.LASF14:
	.ascii	"BOOT_STAGE_IRQ_INIT\000"
.LASF5:
	.ascii	"unsigned int\000"
.LASF18:
	.ascii	"boot_stage_t\000"
.LASF24:
	.ascii	"/home/rossierd/soo.tech/20-makefiles/so3\000"
.LASF12:
	.ascii	"__end\000"
.LASF0:
	.ascii	"long unsigned int\000"
.LASF7:
	.ascii	"long long unsigned int\000"
.LASF22:
	.ascii	"GNU C89 9.2.1 20191025 -mlittle-endian -mabi=aapcs-"
	.ascii	"linux -mabi=aapcs-linux -mno-thumb-interwork -mfpu="
	.ascii	"vfp -marm -mfloat-abi=hard -mtls-dialect=gnu -march"
	.ascii	"=armv7-a+fp -g -std=gnu90 -fno-builtin -ffreestandi"
	.ascii	"ng -fno-strict-aliasing -fno-common -fno-PIE -fno-d"
	.ascii	"warf2-cfi-asm -fno-ipa-sra -fno-delete-null-pointer"
	.ascii	"-checks -fno-stack-protector -fomit-frame-pointer -"
	.ascii	"fno-var-tracking-assignments -fno-strict-overflow -"
	.ascii	"fno-merge-all-constants -fmerge-constants -fstack-c"
	.ascii	"heck=no -fconserve-stack -fno-function-sections -fn"
	.ascii	"o-data-sections -funwind-tables --param allow-store"
	.ascii	"-data-races=0\000"
.LASF15:
	.ascii	"BOOT_STAGE_SCHED\000"
.LASF16:
	.ascii	"BOOT_STAGE_IRQ_ENABLE\000"
.LASF21:
	.ascii	"__stack_top\000"
.LASF3:
	.ascii	"unsigned char\000"
.LASF25:
	.ascii	"main\000"
.LASF9:
	.ascii	"uint32_t\000"
.LASF17:
	.ascii	"BOOT_STAGE_COMPLETED\000"
.LASF6:
	.ascii	"long long int\000"
.LASF1:
	.ascii	"short unsigned int\000"
.LASF2:
	.ascii	"signed char\000"
.LASF11:
	.ascii	"__sys_l1pgtable\000"
.LASF23:
	.ascii	"arch/arm32/asm-offsets.c\000"
.LASF20:
	.ascii	"origin_cpu\000"
.LASF19:
	.ascii	"boot_stage\000"
.LASF4:
	.ascii	"short int\000"
.LASF13:
	.ascii	"BOOT_STAGE_INIT\000"
.LASF10:
	.ascii	"char\000"
	.ident	"GCC: (GNU Toolchain for the A-profile Architecture 9.2-2019.12 (arm-9.10)) 9.2.1 20191025"
	.section	.note.GNU-stack,"",%progbits
