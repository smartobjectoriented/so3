/*
 * semicall.S - assembly stub for making semihosting call
 *
 * Copyright (c) 2015 Linaro Limited
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Linaro Limited nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 */

#ifndef __aarch64__
        .syntax unified
#endif

        .text

#if defined(__thumb__)
        .thumb
#endif

        .globl __semi_call
        .type __semi_call, function
__semi_call:
#if defined(__aarch64__)
        hlt 0xf000
        ret
#else

#if defined(USE_HLT)
        /* semihosting 2.0 hlt trap. We have to use literals
         * here because the assembler refuses to assemble hlt
         * for ARMv7 processors (but it is a semihosting trap
         * regardless)
         */

#if defined(__thumb__)
        .short 0xbabc /* hlt 0x3c */
#else
        .word 0xe10f0070 /* hlt 0xf000 */
#endif

#else
        /* traditional svc */
#if defined(__thumb__)
#if defined(USE_BKPT)
        /* M-profile T32 */
        bkpt 0xab
#else
        /* A+R Profile T32 */
        svc 0xab
#endif
#else
        svc 0x123456
#endif

#endif

        bx lr

#endif
