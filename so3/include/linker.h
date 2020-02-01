/*
 * include/linker_lists.h
 *
 * Implementation of linker-generated arrays
 *
 * Copyright (C) 2012 Marek Vasut <marex@denx.de>
 * Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
 * Copyright (C) 2019 David Truan <david.truan@heig-vd.ch>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef LINKER_H
#define LINKER_H

/*
 * There is no use in including this from ASM files, but that happens
 * anyway, e.g. PPC kgdb.S includes command.h which incluse us.
 * So just don't define anything when included from ASM.
 */

#if !defined(__ASSEMBLY__)

#define __aligned(x) __attribute__((aligned(x)))

/* Used to determine the number of initcalls in each section */
extern unsigned long __initcall_driver_core, __initcall_driver_core_end, __initcall_driver_postcore, __initcall_driver_postcore_end;

#define ll_entry_start(_type, _level) \
({				      \
	_type *start = (_type *) &__initcall_driver_##_level; \
	start;			      \
})

/* Add a initcall entry (using _type) in the section according to its level (core, postcore, etc.). */
#define ll_entry_declare(_type, _level, _init)				\
	_type _initcall_driver_##_level##_init __aligned(4)		\
			__attribute__((unused,				\
			section(".initcall_driver_"#_level)))

/* Get the number of initcalls of a specific (_level) section. */
#define ll_entry_count(_type, _level)					\
({								\
	_type *start = (_type *) &__initcall_driver_##_level; \
	_type *end = (_type *) &__initcall_driver_##_level##_end; \
	unsigned int _ll_result = end - start;			\
	_ll_result;						\
})

#endif /* __ASSEMBLY__ */

#endif	/* LINKER_H */
