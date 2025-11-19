/*
 * Copyright (C) 2025 Clément Dieperink <clement.dieperink@heig-vd.ch>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef ASM_ARM_HWCAP_H
#define ASM_ARM_HWCAP_H

/*
 * This isn't used be MUSL on AArch64, so simply let it to 0 for now.
 */
#define HWCAP_ELF 0

#endif /* ASM_ARM_HWCAP */
