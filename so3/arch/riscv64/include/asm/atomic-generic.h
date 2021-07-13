
/*
 * Copyright (C) 2005 Silicon Graphics, Inc.
 *	Christoph Lameter <clameter@sgi.com>
 *
 * Allows to provide arch independent atomic definitions without the need to
 * edit all arch specific atomic.h files.
 */

#ifndef ASM_GENERIC_ATOMIC_H
#define ASM_GENERIC_ATOMIC_H

#include <asm/types.h>

/*
 * Support for atomic_long_t
 *
 * Casts for parameters are avoided for existing atomic functions in order to
 * avoid issues with cast-as-lval under gcc 4.x and other limitations that the
 * macros of a platform may have.
 */

#if BITS_PER_LONG == 64

typedef atomic64_t atomic_long_t;

#define ATOMIC_LONG_INIT(i)	ATOMIC64_INIT(i)

static inline long atomic_long_read(atomic_long_t *l)
{

}

static inline void atomic_long_set(atomic_long_t *l, long i)
{

}

static inline void atomic_long_inc(atomic_long_t *l)
{

}

static inline void atomic_long_dec(atomic_long_t *l)
{

}

static inline void atomic_long_add(long i, atomic_long_t *l)
{

}

static inline void atomic_long_sub(long i, atomic_long_t *l)
{

}

#else

typedef atomic_t atomic_long_t;

#define ATOMIC_LONG_INIT(i)	ATOMIC_INIT(i)
static inline long atomic_long_read(atomic_long_t *l)
{

}

static inline void atomic_long_set(atomic_long_t *l, long i)
{

}

static inline void atomic_long_inc(atomic_long_t *l)
{

}

static inline void atomic_long_dec(atomic_long_t *l)
{

}

static inline void atomic_long_add(long i, atomic_long_t *l)
{

}

static inline void atomic_long_sub(long i, atomic_long_t *l)
{

}

#endif

#endif /* ASM_GENERIC_ATOMIC_H */
