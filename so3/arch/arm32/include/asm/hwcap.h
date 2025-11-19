/*
 *  Copyright (C) 2025 Clément Dieperink <clement.dieperink@heig-vd.ch>
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ASM_ARM_HWCAP_H
#define ASM_ARM_HWCAP_H

/* clang-format off */

/*
 * Hardware capabilities flags - for AT_HWCAP
 * Copied from Linux arch/arm/include/asm/procinfo.h
 */
#define HWCAP_SWP	(1 << 0)
#define HWCAP_HALF	(1 << 1)
#define HWCAP_THUMB	(1 << 2)
#define HWCAP_26BIT	(1 << 3)	/* Play it safe */
#define HWCAP_FAST_MULT	(1 << 4)
#define HWCAP_FPA	(1 << 5)
#define HWCAP_VFP	(1 << 6)
#define HWCAP_EDSP	(1 << 7)
#define HWCAP_JAVA	(1 << 8)
#define HWCAP_IWMMXT	(1 << 9)
#define HWCAP_CRUNCH	(1 << 10)	/* Obsolete */
#define HWCAP_THUMBEE	(1 << 11)
#define HWCAP_NEON	(1 << 12)
#define HWCAP_VFPv3	(1 << 13)
#define HWCAP_VFPv3D16	(1 << 14)	/* also set for VFPv4-D16 */
#define HWCAP_TLS	(1 << 15)
#define HWCAP_VFPv4	(1 << 16)
#define HWCAP_IDIVA	(1 << 17)
#define HWCAP_IDIVT	(1 << 18)
#define HWCAP_VFPD32	(1 << 19)	/* set if VFP has 32 regs (not 16) */
#define HWCAP_IDIV	(HWCAP_IDIVA | HWCAP_IDIVT)
#define HWCAP_LPAE	(1 << 20)
#define HWCAP_EVTSTRM	(1 << 21)
#define HWCAP_FPHP	(1 << 22)
#define HWCAP_ASIMDHP	(1 << 23)
#define HWCAP_ASIMDDP	(1 << 24)
#define HWCAP_ASIMDFHM	(1 << 25)
#define HWCAP_ASIMDBF16	(1 << 26)
#define HWCAP_I8MM	(1 << 27)

/*
 * ARMv7 flags used by Linux, see arch/arm/mm/proc-v7.S, macro __v7_proc
 */
#define HWCAP_ELF (HWCAP_SWP | HWCAP_HALF | HWCAP_THUMB | HWCAP_FAST_MULT | HWCAP_EDSP | HWCAP_TLS)

/* clang-format on */

#endif /* ASM_ARM_HWCAP */
