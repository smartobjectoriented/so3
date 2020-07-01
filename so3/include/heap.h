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

#ifndef HEAP_H
#define HEAP_H

#include <types.h>
#include <sizes.h>

#if 0
#define TRACKING
#endif

/*
 * The heap size is defined in arch/arm/so3.lds
 * The value must be strictly the same.
*/
#define HEAP_SIZE 	(4*(SZ_512K))

/*
 * The user space has a 8 MB heap size
 */
#define USR_HEAP_SIZE	(8 * PAGE_SIZE)

#define CHUNK_SIG	0xbeefdead

/* there's a list for each size. The size changes over the process lifetime. */
struct mem_chunk {

	/* Chunk signature to help identifying if a chunk has already be free'd or not. */
	uint32_t sig;

	/* Pointer to the next memchunk of a different size */
	struct mem_chunk *next_list;

	/* Pointer to the next memchunk of this size. */
	struct mem_chunk *next_chunk;

	/* Refer to the head of the (2nd-dimension) list for this size */
	struct mem_chunk *head;

	/*
	 * Size in bytes of the user area (this does NOT include the size of mem_chunk).
	 * This size can be over the requested size if the remaining area is not large enough
	 * to store the mem_chunk structure (use req_size to have the *real* requested area in any case).
	 */
	size_t size;

	/* Real requested size by the user. */
	size_t req_size;

	/* Used in case of specific alignment on address - These bytes are lost */
	size_t padding_bytes;
};
typedef struct mem_chunk mem_chunk_t;


#ifndef TRACKING
void *malloc(size_t size);
void *memalign(size_t size, unsigned int alignment);
#else
void *malloc_log(size_t size, const char *filename, const char *fname, const int line);
void *memalign_log(size_t size, unsigned int alignment, const char *filename, const char *fname, const int line);

#define malloc(x) malloc_log(x, __FILE__, __func__, __LINE__)
#define memalign(x,y) memalign_log(x, y, __FILE__, __func__, __LINE__)

#endif

void free(void *ptr);
void printHeap(void);

uint32_t heap_size(void);

void heap_init(void);
void dump_heap(const char *info);


#endif /* HEAP_H */
