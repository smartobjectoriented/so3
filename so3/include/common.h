/*
 * Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
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
#include <printk.h>
#include <string.h>

#endif /* __ASSEMBLY__ */

#ifdef CONFIG_AVZ

/*
 * CPU #0 is the primary (non-RT) Agency CPU.
 * CPU #1 is the hard RT Agency CPU.
 * CPU #2 is the second (SMP) Agency CPU.
 * CPU #3 is the ME CPU.
 */

#define AGENCY_CPU		0
#define AGENCY_RT_CPU     	1

#define ME_CPU		 	3

#endif /* CONFIG_AVZ */


#ifndef __ASSEMBLY__

extern addr_t __end[];

#ifdef CONFIG_AVZ

#include <avz/console.h>

/*
 * Pseudo-usr mode allows the hypervisor to switch back to the right stack (G-stach/H-stack) depending on whether
 * the guest issued a hypercall or if an interrupt occurred during some processing in the hypervisor.
 * 0 means we are in some hypervisor code, 1 means we are in some guest code.
 */
extern addr_t pseudo_usr_mode[];

#endif /* CONFIG_AVZ */

#ifdef DEBUG
#undef DBG

#ifdef CONFIG_AVZ
#define DBG(fmt, ...) \
    do { \
		lprintk("[soo:avz] %s:%i > "fmt, __func__, __LINE__, ##__VA_ARGS__); \
    } while(0)

#else
#define DBG(fmt, ...) \
    do { \
	lprintk("%s:%i > "fmt, __func__, __LINE__, ##__VA_ARGS__); \
    } while(0)
#endif

#else
#define DBG(fmt, ...)
#endif

#define DIV_ROUND_CLOSEST(x, divisor)(                  \
 {                                                       \
   typeof(x) __x = x;                              \
   typeof(divisor) __d = divisor;                  \
     (((typeof(x))-1) > 0 ||                         \
     ((typeof(divisor))-1) > 0 || (__x) > 0) ?      \
       (((__x) + ((__d) / 2)) / (__d)) :       \
       (((__x) - ((__d) / 2)) / (__d));        \
 }                                                       \
)

#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))

#define max(a, b) ({\
		typeof(a) _a = a;\
		typeof(b) _b = b;\
		_a > _b ? _a : _b; })

#define min(a, b) ({\
		typeof(a) _a = a;\
		typeof(b) _b = b;\
		_a < _b ? _a : _b; })

/**
 * container_of - cast a member of a structure out to the containing structure
 *
 * @ptr:        the pointer to the member.
 * @type:       the type of the container struct this is embedded in.
 * @member:     the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({ \
  typeof( ((type *)0)->member ) *__mptr = (ptr); \
  (type *)( (char *)__mptr - offsetof(type,member) );})

void kernel_panic(void);
void _bug(char *file, int line);

static inline void panic(const char *fmt, ...) {
	va_list args;
	static char buf[128];

	va_start(args, fmt);
	(void)vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	printk("%s", buf);
	kernel_panic();

}

extern void panic(const char *format, ...) __attribute__ ((format (printf, 1, 2)));

#define BUG()	_bug(__FILE__, __LINE__)
#define BUG_ON(p)  do { if (unlikely(p)) BUG();  } while (0)

#define assert_failed(p)                                        \
do {                                                            \
	lprintk("Assertion '%s' failed on CPU #%d, line %d, file %s\n", p , smp_processor_id(),     \
                    __LINE__, __FILE__);                         \
        kernel_panic();                                             \
} while (0)

#define ASSERT(p) \
     do { if ( unlikely(!(p)) ) assert_failed(#p); } while (0)

typedef enum {
	BOOT_STAGE_INIT, BOOT_STAGE_IRQ_INIT, BOOT_STAGE_SCHED, BOOT_STAGE_IRQ_ENABLE, BOOT_STAGE_COMPLETED
} boot_stage_t;
extern boot_stage_t boot_stage;

/* To keep the original CPU ID so that we can avoid
 * undesired activities running on another CPU.
 */
extern uint32_t origin_cpu;

extern void __backtrace(void);
/*
 * ..and if you can't take the strict
 * types, you can specify one yourself.
 *
 * Or not use min/max at all, of course.
 */
#define min_t(type,x,y) \
        ({ type __x = (x); type __y = (y); __x < __y ? __x: __y; })
#define max_t(type,x,y) \
        ({ type __x = (x); type __y = (y); __x > __y ? __x: __y; })

/*
 * Check at compile time that something is of a particular type.
 * Always evaluates to 1 so you may use it easily in comparisons.
 */
#define typecheck(type,x)                       \
({	type __dummy;                           \
	typeof(x) __dummy2;                     \
	(void)(&__dummy == &__dummy2);          \
	1;                                      \
})

/*
 * This looks more complex than it should be. But we need to
 * get the type for the ~ right in round_down (it needs to be
 * as wide as the result!), and we want to evaluate the macro
 * arguments just once each.
 */
#define __round_mask(x, y) ((__typeof__(x))((y)-1))
/**
 * round_up - round up to next specified power of 2
 * @x: the value to round
 * @y: multiple to round up to (must be a power of 2)
 *
 * Rounds @x up to next multiple of @y (which must be a power of 2).
 * To perform arbitrary rounding up, use roundup() below.
 */
#define round_up(x, y) ((((x)-1) | __round_mask(x, y))+1)
/**
 * round_down - round down to next specified power of 2
 * @x: the value to round
 * @y: multiple to round down to (must be a power of 2)
 *
 * Rounds @x down to next multiple of @y (which must be a power of 2).
 * To perform arbitrary rounding down, use rounddown() below.
 */
#define round_down(x, y) ((x) & ~__round_mask(x, y))

#endif /* __ASSEMBLY__ */

#endif /* COMMON_H */
