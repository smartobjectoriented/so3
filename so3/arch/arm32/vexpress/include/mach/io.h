/*
 * Copyright (C) 2020-2022 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifndef MACH_IO
#define MACH_IO

/* vexpress-sysregs flags as defined in linux:drivers/mfd/vexpress-sysreg.c */

#define SYS_ID                  0x000
#define SYS_SW                  0x004
#define SYS_LED                 0x008
#define SYS_100HZ               0x024
#define SYS_FLAGSSET            0x030
#define SYS_FLAGSCLR            0x034
#define SYS_NVFLAGS             0x038
#define SYS_NVFLAGSSET          0x038
#define SYS_NVFLAGSCLR          0x03c
#define SYS_MCI                 0x048
#define SYS_FLASH               0x04c
#define SYS_CFGSW               0x058
#define SYS_24MHZ               0x05c
#define SYS_MISC                0x060
#define SYS_DMA                 0x064
#define SYS_PROCID0             0x084
#define SYS_PROCID1             0x088
#define SYS_CFGDATA             0x0a0
#define SYS_CFGCTRL             0x0a4
#define SYS_CFGSTAT             0x0a8

#define SYS_HBI_MASK            0xfff
#define SYS_PROCIDx_HBI_SHIFT   0

#define SYS_MCI_CARDIN          (1 << 0)
#define SYS_MCI_WPROT           (1 << 1)

#define SYS_MISC_MASTERSITE     (1 << 14)


#endif /* MACH_IO */

