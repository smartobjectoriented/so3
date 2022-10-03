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

#include <types.h>
#include <printk.h>

#define unlikely(x)   __builtin_expect((x),0)

extern addr_t __end[];

#ifdef DEBUG
#undef DBG
#define DBG(fmt, ...) \
    do { \
	lprintk("%s:%i > "fmt, __func__, __LINE__, ##__VA_ARGS__); \
    } while(0)
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

#define BUG()	_bug(__FILE__, __LINE__)
#define BUG_ON(p)  do { if (unlikely(p)) BUG();  } while (0)

#define assert_failed(p)                                        \
do {                                                            \
  lprintk("Assertion '%s' failed, line %d, file %s\n", p ,     \
                    __LINE__, __FILE__);                         \
  kernel_panic();                                             \
} while (0)

#define ASSERT(p) \
     do { if ( unlikely(!(p)) ) assert_failed(#p); } while (0)

void dump_heap(const char *info);

typedef enum {
	BOOT_STAGE_INIT, BOOT_STAGE_IRQ_INIT, BOOT_STAGE_SCHED, BOOT_STAGE_IRQ_ENABLE, BOOT_STAGE_COMPLETED
} boot_stage_t;
extern boot_stage_t boot_stage;

/* To keep the original CPU ID so that we can avoid
 * undesired activities running on another CPU.
 */
extern uint32_t origin_cpu;

extern void __backtrace(void);

#endif /* COMMON_H */
