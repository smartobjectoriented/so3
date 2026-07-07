/*
 * Copyright (C) 2014-2026 REDS Institute from HEIG-VD <daniel.rossier@heig-vd.ch>
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

#ifndef COMMON_H
#define COMMON_H

#include <generated/autoconf.h>

#ifndef __ASSEMBLY__

#include <types.h>
#include <compiler.h>
#include <printk.h>
#include <string.h>
#include <math.h>

#endif /* __ASSEMBLY__ */

#ifdef CONFIG_AVZ

/*
 * CPU #0 is the primary Agency CPU.
 * CPU #1 and #2 are additional (SMP) Agency CPUs.
 * CPU #3 is the capsule CPU.
 */

#define AGENCY_CPU 0

#define S3C_CPU 3

#endif /* CONFIG_AVZ */

#ifndef __ASSEMBLY__

extern addr_t __end[];
extern addr_t __stack_bottom[];

/**
 * container_of - cast a member of a structure out to the containing structure
 *
 * @ptr:        the pointer to the member.
 * @type:       the type of the container struct this is embedded in.
 * @member:     the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member)                              \
	({                                                           \
		typeof(((type *) 0)->member) *__mptr = (ptr);        \
		(type *) ((char *) __mptr - offsetof(type, member)); \
	})

void kernel_panic(void);
void _bug(char *file, int line);

__attribute_printf(1, 2) static inline void panic(const char *fmt, ...)
{
	va_list args;
	static char buf[128];

	va_start(args, fmt);
	(void) vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	printk("%s", buf);
	kernel_panic();
}

#define BUG() _bug(__FILE__, __LINE__)
#define BUG_ON(p)                \
	do {                     \
		if (unlikely(p)) \
			BUG();   \
	} while (0)

#define assert_failed(p)                                                                                                    \
	do {                                                                                                                \
		lprintk("Assertion '%s' failed on CPU #%d, line %d, file %s\n", p, smp_processor_id(), __LINE__, __FILE__); \
		kernel_panic();                                                                                             \
	} while (0)

#define ASSERT(p)                          \
	do {                               \
		if (unlikely(!(p)))        \
			assert_failed(#p); \
	} while (0)

typedef enum {
	BOOT_STAGE_INIT,
	BOOT_STAGE_HEAP_READY,
	BOOT_STAGE_IRQ_INIT,
	BOOT_STAGE_SCHED,
	BOOT_STAGE_IRQ_ENABLE,
	BOOT_STAGE_COMPLETED
} boot_stage_t;
extern boot_stage_t boot_stage;

/* To keep the original CPU ID so that we can avoid
 * undesired activities running on another CPU.
 */
extern uint32_t origin_cpu;

extern void __backtrace(void);

/*
 * Check at compile time that something is of a particular type.
 * Always evaluates to 1 so you may use it easily in comparisons.
 */
#define typecheck(type, x)                      \
	({                                      \
		type __dummy;                   \
		typeof(x) __dummy2;             \
		(void) (&__dummy == &__dummy2); \
		1;                              \
	})

#endif /* __ASSEMBLY__ */

#endif /* COMMON_H */
