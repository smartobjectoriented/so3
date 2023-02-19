#ifndef PERCPU_H
#define PERCPU_H

#include <common.h>

#define PERCPU_SHIFT 13
#define PERCPU_SIZE  (1UL << PERCPU_SHIFT)

#define __GENERIC_PER_CPU

void percpu_init_areas(void);
void free_percpu_area(unsigned int cpu);
int init_percpu_area(unsigned int cpu);

/*
 * per_cpu_offset() is the offset that has to be added to a
 * percpu variable to get to the instance for a certain processor.
 *
 * Most arches use the __per_cpu_offset array for those offsets but
 * some arches have their own ways of determining the offset (x86_64, s390).
 */
#ifndef __per_cpu_offset
extern unsigned long __per_cpu_offset[CONFIG_NR_CPUS];

#define per_cpu_offset(x) (__per_cpu_offset[x])
#endif

/* var is in discarded region: offset to particular copy we want */
#define per_cpu(var, cpu) (*RELOC_HIDE(&per_cpu__##var, __per_cpu_offset[cpu]))
#define __get_cpu_var(var) per_cpu(var, smp_processor_id())
#define __raw_get_cpu_var(var) per_cpu(var, raw_smp_processor_id())


/* Separate out the type, so (int[3], foo) works. */
#define __DEFINE_PER_CPU(type, name, suffix)                      \
    __attribute__((__section__(".bss.percpu")))        \
    __typeof__(type) per_cpu_##name

#define DECLARE_PER_CPU(type, name) extern __typeof__(type) per_cpu__##name

#define EXPORT_PER_CPU_SYMBOL(var) EXPORT_SYMBOL(per_cpu__##var)
#define EXPORT_PER_CPU_SYMBOL_GPL(var) EXPORT_SYMBOL_GPL(per_cpu__##var)

/*
 * Separate out the type, so (int[3], foo) works.
 *
 * The _##name concatenation is being used here to prevent 'name' from getting
 * macro expanded, while still allowing a per-architecture symbol name prefix.
 */
#define DEFINE_PER_CPU(type, name) __DEFINE_PER_CPU(type, _##name, )
#define DEFINE_PER_CPU_READ_MOSTLY(type, name) \
	__DEFINE_PER_CPU(type, _##name, .read_mostly)

#define this_cpu(var)    __get_cpu_var(var)

/* Linux compatibility. */
#define get_cpu_var(var) this_cpu(var)
#define put_cpu_var(var)

#endif /* PERCPU_H */
