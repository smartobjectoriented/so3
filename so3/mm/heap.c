/*
 * Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
 * Copyright (C) 2015-2016 Xavier Ruppen <xavier.ruppen@heig-vd.ch>
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

#if 0
#define DEBUG
#endif

#include <common.h>
#include <heap.h>
#include <string.h>
#include <sizes.h>
#include <spinlock.h>

#include <asm/processor.h>

#define NR_DEMANDS		4096

extern addr_t __heap_base_addr;

static DEFINE_SPINLOCK(heap_lock);

/* Head of the quick list */
static mem_chunk_t *quick_list;

extern void consistency(void);
extern void __consistency(int addr, int victim, int size, int requested);

/*
 * Initialization of the quick-fit list.
 */
void heap_init(void)
{
	quick_list = (mem_chunk_t *) &__heap_base_addr;

	/* Initialize all the heap to 0 to have a better compression ratio. */
	memset(quick_list, 0, HEAP_SIZE);

	quick_list->next_list = NULL;
	quick_list->next_chunk = NULL;
	quick_list->head = quick_list;

	/* Reserve the place for the first mem_chunk */
	quick_list->size = HEAP_SIZE - sizeof(mem_chunk_t);
	quick_list->padding_bytes = 0;

	printk("SO3: allocating a kernel heap of %d bytes at address %p.\n", quick_list->size, quick_list);

	DBG("[list_init] List initialized. sizeof(mem_chunk_t) = %d bytes, sizeof(int) = %d bytes\n", sizeof(mem_chunk_t), sizeof(int));
}

uint32_t heap_size(void) {
	mem_chunk_t *list = quick_list;
	mem_chunk_t *chunk = quick_list;
	uint32_t total_size = 0;
	uint32_t flags;

	flags = spin_lock_irqsave(&heap_lock);

	while (list) {
		while (chunk) {
			total_size += chunk->size;
			chunk = chunk->next_chunk;
		}

		list = list->next_list;
		chunk = list;
	}

	spin_unlock_irqrestore(&heap_lock, flags);

	return total_size;
}


/*
 * Print the content of the quick-fit list.
 */
void dump_heap(const char *info)
{
	int i, j;
	mem_chunk_t *list = quick_list;
	mem_chunk_t *chunk = quick_list;
	uint32_t total_size = 0;

	i = j = 0;

	while (list) {
		printk("[%s - print_list] [0x%p] List #%d\n", info, list, j++);

		while (chunk) {
			printk("[%s - print_list] [%p] %d size = %d bytes, head = %p, next_list = %p, next_chunk = %p ---> \n",
		 	       info, chunk, i++, chunk->size, chunk->head, chunk->next_list,  chunk->next_chunk);
			total_size += chunk->size;

			chunk = chunk->next_chunk;
		}
		i = 0;

		list = list->next_list;
		chunk = list;
	}
	printk("  [%s] Heap total remaining size: %d\n", info, total_size);
}

void print_chunk(mem_chunk_t *chunk, const char *caller, const char *name)
{
	DBG("[%s - print_chunk] [%p] [%s] %d bytes, next_list = %p, next_chunk = %p\n",
	    caller, chunk, name, chunk->size, chunk->next_list, chunk->next_chunk);
}

/*
 * Re-init a chunk
 */
static void reset_chunk(mem_chunk_t *chunk)
{
	DBG("[reset_chunk] %p\n", chunk);

	chunk->next_list = NULL;
	chunk->next_chunk = NULL;
	chunk->head = NULL;

	/* Of course, size is preserved. */
}

/*
 * Remove a memchunk from its current position in the quick list.
 */
static void remove_chunk(mem_chunk_t *chunk)
{
	mem_chunk_t *__chunk, *head;

	DBG("[remove_chunk] %p\n", chunk);

	__chunk = chunk;
	head = chunk->head;

	ASSERT(head != NULL);

	/* Special case if the memchunk to be removed is the head of its list */
	if (head == chunk) {
		/* The memchunk is at the head of the list */
		head = quick_list;

		/* Update the memchunk list references if necessary */
		if (chunk == quick_list)

			/* Check if the memchunk is at the head of the quick list */
			quick_list = ((chunk->next_chunk == NULL) ? chunk->next_list : chunk->next_chunk);

		else {
			/* Look for the head predecessor of this chunk */
			while (head->next_list != chunk)
				head = head->next_list;

			/* Update the previous head to the next_list */
			if (chunk->next_chunk == NULL)
				head->next_list = chunk->next_list;
			else
				head->next_list = chunk->next_chunk;

		}

		if (chunk->next_chunk != NULL) {
			/* We also need to update the head of each chunk of associated list */
			head = chunk->next_chunk;
			head->next_list = chunk->next_list; /* Just the head */

			chunk = head;
			do {
				chunk->head = head;
				chunk = chunk->next_chunk;
			} while (chunk != NULL);
		}

	} else {

		while (head->next_chunk != chunk)
			head = head->next_chunk;

		/* Found, detach the mem chunk */
		head->next_chunk = chunk->next_chunk;
	}

	reset_chunk(__chunk);

#ifdef DEBUG
	dump_heap(__func__);
#endif

}

/*
 * Insert a new mem_chunk in the quick list.
 */
static void append_chunk(mem_chunk_t *chunk)
{
	mem_chunk_t *tmp_list, *last;
	mem_chunk_t *tmp_chunk;

	DBG("[append_chunk] %p with size %d padding: %d\n", chunk, chunk->size, chunk->padding_bytes);

	reset_chunk(chunk);

	if (!quick_list) {
		DBG("[append_chunk] Adding first entry in quick_list\n");
		quick_list = chunk;
		chunk->head = quick_list;
	} else {

		/* Try to append our chunk to existing list */
		tmp_list = quick_list;

		while ((tmp_list != NULL) && (tmp_list->size < chunk->size)) {
			last = tmp_list; /* Useful in the latest case */
			tmp_list = tmp_list->next_list;
		}

		if (tmp_list != NULL) { /* We found a position in the list (sorted items) */

			if (tmp_list->size == chunk->size) { /* Good, we have found the right size */
				DBG("[append_chunk] found a list with the right size %d\n", chunk->size);
				tmp_chunk = tmp_list;

				/* Go to end of list */
				while (tmp_chunk->next_chunk != NULL)
					tmp_chunk = tmp_chunk->next_chunk;

				/* Append our new free chunk */
				tmp_chunk->next_chunk = chunk;
				chunk->head = tmp_list;

			} else { /* Not the right size, but tmp_list->size is greater than chunk->size, so create a list entry and insert right before */

				DBG("[append_chunk] creating list of new size %d\n", chunk->size);

				if (tmp_list == quick_list) { /* This entry is the quick_list origin, update it */
					chunk->next_list = quick_list;
					chunk->head = chunk;
					quick_list = chunk;

				} else { /* Somewhere else in the quick list ... */

					tmp_chunk = quick_list;
					while (tmp_chunk->next_list != tmp_list)
						tmp_chunk = tmp_chunk->next_list;

					/* Insert it - tmp_chunk now refers to the previous list. */

					tmp_chunk->next_list = chunk;

					chunk->next_list = tmp_list;
					chunk->head = chunk;
				}
			}

		} else { /* Definitively at the end of the list */

			DBG("[append_chunk] adding at the end of quick_list with new size %d\n", chunk->size);

			last->next_list = chunk;
			chunk->head = chunk;
		}
	}

#ifdef DEBUG
	dump_heap(__func__);
#endif
}

/*
 * Find neighbouring chunks and merge them
 */
static void merge_chunks(mem_chunk_t *new_chunk)
{
	mem_chunk_t *tmp_list;
	mem_chunk_t *tmp_chunk, *merged_chunk;

	/*
	 * Inspect the whole quick list to see if some possible merge is feasible. Since a merged chunk may be adjacent to another one, we need to
	 * re-iterate on the quick list to find a possible candidate for a merge.
	 */
recheck:
	tmp_list = quick_list;

	while (tmp_list) {
		tmp_chunk = tmp_list;

		/* Iterate through every chunk */
		while (tmp_chunk) {

			/* Check if merging required. If we do this each time a chunk is freed,
			 * it's sufficient to only check for chunks adjacent to the one just freed */
			if ((char *) new_chunk + new_chunk->size + sizeof(mem_chunk_t) == (char *) tmp_chunk || /* adjacent chunk after */
					(char *) tmp_chunk + tmp_chunk->size + sizeof(mem_chunk_t) == (char *) new_chunk) { /* adjacent chunk before */


				/* tmp_chunk is adjacent to new_chunk. Merge them */
				DBG("[merge_chunks] merging new_chunk [%p] with tmp_chunk [%p]\n", new_chunk, tmp_chunk);
#ifdef DEBUG
				dump_heap(__func__);
#endif

				/* merge these two chunks into a new one */
				if (new_chunk < tmp_chunk) {

					/* new_chunk is before tmp_chunk */
					new_chunk->size += tmp_chunk->size + sizeof(mem_chunk_t); /* Recover space used by memchunk */
					merged_chunk = new_chunk;

				} else {

					/* new_chunk is after tmp_chunk */
					tmp_chunk->size += new_chunk->size + sizeof(mem_chunk_t);
					merged_chunk = tmp_chunk;

				}

				/* Remove both chunks from the list... */
				remove_chunk(tmp_chunk);
				remove_chunk(new_chunk);

				/* ...and append the merged chunk. */
				DBG("[merge_chunks] tmp_chunk: %p new_chunk: %p merged_chunk: %p merged_chunk->size = %d\n", tmp_chunk, new_chunk, merged_chunk, merged_chunk->size);
				append_chunk(merged_chunk);

				/* we will recheck for any adjacent chunk with our merged chunk */
				new_chunk = merged_chunk;
				goto recheck;
			}

			tmp_chunk = tmp_chunk->next_chunk;
		}

		tmp_list = tmp_list->next_list;
	}


	DBG("[merge_chunks] merge completed\n");

#ifdef DEBUG
	dump_heap(__func__);
#endif
}

/*
 * Allocate a memory area of a certain size (<size> bytes).
 * The address can be requested to be aligned according to @alignment which has to be a power of 2.
 */
static void *__malloc(size_t requested, unsigned int alignment)
{
	mem_chunk_t *victim;
	mem_chunk_t *remaining = NULL; /* new free chunk if size < victim->size */
	mem_chunk_t tmp_memchunk; /* Used for possible shifting of the structure */
	void *addr = NULL, *tmp_addr;
	uint32_t flags;

#ifdef DEBUG
	dump_heap(__func__);
#endif

	/*
	* We disable the IRQs to be safe. Using a mutex is dangerous since, a context switch may lead
	* to a (re-)malloc in a waitqueue.
	*/
	flags = spin_lock_irqsave(&heap_lock);

	DBG("[malloc] requested size = %d, mem_chunk_size = %d bytes\n", requested, sizeof(mem_chunk_t));

	/* find the best fit in our list */
	victim = quick_list;

next_list:
	/* We check victim to be different of NULL since we might iterate several times */
	while (victim && (victim->size < requested))
		victim = victim->next_list;

	if (!victim) {
		/* not enough free space left */
		/* FIXME: do sbrk() here to request more space. Request less space in init() */
		printk("[malloc] Not enough free space, requested = %x\n", requested);

		spin_unlock_irqrestore(&heap_lock, flags);

		dump_heap("Heap overflow");

		return NULL;
	}

	/* Go to the end of the list to avoid overhead time to re-process head of each chunk */
	while (victim->next_chunk != NULL)
		victim = victim->next_chunk;

	/* Calculate the *real* address that will be returned to the caller */
	addr = (char *) victim + sizeof(mem_chunk_t);

	/* Store the requested size if the area becomes larger than requested because of the lack of space to store remaining chunk. */
	victim->req_size = requested;

	/*
	 * We enforce the address returned to be word-aligned so that if some data which requires word-alignment
	 * (for example in case of ldrex/strex based monitors) will stay aligned.
	 */

	alignment = ((alignment == 0) ? sizeof(int) : alignment);

	/* Is a specific alignment requested ? (alignment must be a power of 2) */

	if (alignment > 0) {
		tmp_addr = (void *) (((addr_t) addr + alignment - 1) & -((signed) alignment));

		ASSERT(tmp_addr >= addr);

		/* If there is a required shift, padding bytes are considered as payload bytes of the chunk */
		if ((unsigned int) (tmp_addr - addr) + requested  > victim->size) {

			/* Look at a next possible victim in this list */
			victim = victim->head->next_list;

			goto next_list;

		} else {

			victim->padding_bytes = tmp_addr - addr;

			addr = tmp_addr;
		}
	}

	/* This chunk isn't free anymore */

	remove_chunk(victim);

	/* Set the chunk signature to be able to identify already free'd chunks */
	victim->sig = CHUNK_SIG;

	/*
	* If there is a remaining size, and if this remaining size supports storing a mem_chunk_t, then we
	* can append a new (free) memchunk, otherwise remaining bytes are lost.
	*/
	if (victim->size > requested + victim->padding_bytes + sizeof(mem_chunk_t)) {

		/* Split this chunk and append in the list. */
		remaining = (mem_chunk_t *) ((char *) victim + sizeof(mem_chunk_t) + requested + victim->padding_bytes);
		remaining->size = victim->size - requested - sizeof(mem_chunk_t) - victim->padding_bytes;
		remaining->padding_bytes = 0;

		/* Re-calculate the size of the allocated chunk. */
		victim->size = requested + victim->padding_bytes;

		append_chunk(remaining);

	}

	/*
	* If there are some padding bytes, we need to shift the mem_chunk_t of these bytes
	* so that it is possible to retrieve the structure when freeing.
	*/
	if (victim->padding_bytes) {
		memcpy(&tmp_memchunk, victim, sizeof(mem_chunk_t));
		memcpy(((char *) victim) + victim->padding_bytes, &tmp_memchunk, sizeof(mem_chunk_t));
	}

#ifdef DEBUG
	dump_heap(__func__);
#endif

	spin_unlock_irqrestore(&heap_lock, flags);

	return addr;
}

/*
 * Request a chunk of heap memory of @size bytes.
 */
void *malloc(size_t size) {
	return __malloc(size, 0);
}

/*
 * @memalign to retrieve a malloc area of a @requested size with a specific @alignment which is a power of 2.
 */
void *memalign(size_t size, unsigned int alignment) {
	return __malloc(size, alignment);
}

/*
 * Free an allocated area
 */
void free(void *ptr)
{
	uint32_t flags;
	mem_chunk_t *chunk = (mem_chunk_t *)((char *) ptr - sizeof(mem_chunk_t));
	mem_chunk_t tmp_memchunk;

	flags = spin_lock_irqsave(&heap_lock);

	if (chunk->sig != CHUNK_SIG) {
		lprintk("Heap failure: already free'd chunk for address %x...\n", ptr);
		kernel_panic();
	}

	BUG_ON(ptr == NULL);

	/* Reset the chunk signature */
	chunk->sig = 0x0;

	if (chunk->padding_bytes) {
		/* Restore the original position of the memchunk if any padding bytes are considered. */
		memcpy(&tmp_memchunk, chunk, sizeof(mem_chunk_t));

		chunk = (mem_chunk_t *) ((char *) chunk - tmp_memchunk.padding_bytes);

		memcpy(chunk, &tmp_memchunk, sizeof(mem_chunk_t));

		chunk->padding_bytes = 0;
	}

	DBG("[free_chunk] %p with size %d\n", chunk, chunk->size);

#ifdef DEBUG
	dump_heap("free {before}");
#endif
	append_chunk(chunk);
	merge_chunks(chunk);

#ifdef DEBUG
	dump_heap("free {after}");
#endif
	spin_unlock_irqrestore(&heap_lock, flags);
}

/**
 * Invoke malloc by specifying the type of a structure.
 *
 * @param nmemb
 * @param size
 */
void *calloc(size_t nmemb, size_t size) {
        void *ptr;

        ptr = malloc(nmemb*size);
        if (!ptr)
                return ptr;

        memset(ptr, 0, nmemb*size);

        return ptr;
}

/*
 * Re-allocate an existing memory area (previously allocated with malloc).
 * The size can be greater, equal, or less than the original.
 */
void *realloc(void *__ptr, size_t __size) {
	void *alloc;
	struct mem_chunk *chunk;

	chunk = (struct mem_chunk *) (__ptr - sizeof(struct mem_chunk));

	/* Check if the new zone is smaller than the original */
	if (__size < chunk->req_size)
		__size = chunk->req_size;

	alloc = malloc(__size);
	if (!alloc)
		return NULL;

	DBG("Requesting a size of %d\n", __size);
	DBG("Copying a size of %d\n", ((__size < chunk->req_size) ? __size : chunk->req_size));
	DBG("allocation pointer %p\n", alloc);

	memcpy(alloc, __ptr, ((__size < chunk->req_size) ? __size : chunk->req_size));

	free(__ptr);

	return alloc;

}

