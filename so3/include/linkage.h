/*
 * Copyright (C) 2016-2023 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifndef LINKAGE_H
#define LINKAGE_H

#ifdef __ASSEMBLY__

#ifndef ENTRY
#define ENTRY(name)     \
	.globl name;    \
	.align 4, 0x90; \
name:
#endif

#ifndef END
#define END(name) .size name, .- name
#endif

#ifndef ENDPROC
#define ENDPROC(name)           \
	.type name, % function; \
	END(name)
#endif

#endif

#ifdef __ASSEMBLY__
#define _AC(X, Y) X
#define _AT(T, X) X
#else
#define __AC(X, Y) (X##Y)
#define _AC(X, Y) __AC(X, Y)
#define _AT(T, X) ((T)(X))
#endif

#define _UL(x) (_AC(x, UL))
#define _ULL(x) (_AC(x, ULL))

#define _BITUL(x) (_UL(1) << (x))
#define _BITULL(x) (_ULL(1) << (x))

#define UL(x) (_UL(x))
#define ULL(x) (_ULL(x))

#endif /* LINKAGE_H */
