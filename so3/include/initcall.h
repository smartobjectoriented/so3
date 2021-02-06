/*
 * Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
 * Copyright (C) 2019 David Truan <david.truan@heig-vd.ch>
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

#ifndef INITCALL_H
#define INITCALL_H

#define __aligned(x) __attribute__((aligned(x)))

#define ll_entry_start(_type, _level) 				 	\
({				      				 	\
	extern unsigned long __initcall_##_type##_##_level;	 	\
	_type *start = (_type *) &__initcall_##_type##_##_level; 	\
	start;			      \
})

/*
 * Add a initcall entry (using _type) in the section according to its driver level (core, postcore, etc.).
 */
#define ll_entry_declare(_type, _level, _init)				\
	_type __initcall_##_type##_##_level##_##_init __aligned(4)	\
			__attribute__((unused,				\
			section(".initcall_"#_type"_"#_level)))

/*
 * Get the number of initcalls of a specific driver (_level) section.
 */
#define ll_entry_count(_type, _level)					\
({									\
	extern unsigned long __initcall_##_type##_##_level;		\
	extern unsigned long __initcall_##_type##_##_level##_end;	\
	_type *start = (_type *) &__initcall_##_type##_##_level;	\
	_type *end = (_type *) &__initcall_##_type##_##_level##_end; 	\
	unsigned int _ll_result = end - start;				\
	_ll_result;							\
})


typedef void (*postinit_t)(void);
typedef void (*pre_irq_init_t)(void);

#define REGISTER_PRE_IRQ_INIT(_init) ll_entry_declare(pre_irq_init_t, core, _init) = _init;
#define REGISTER_POSTINIT(_init) ll_entry_declare(postinit_t, core, _init) = _init;


#endif /* INITCALL_H */
