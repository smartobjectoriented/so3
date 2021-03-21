#ifndef MALLOC_H
#define MALLOC

#include <libc.h>
#include <stddef.h>

void *malloc(size_t size);
void free(void *ptr);

/* quickfit */
void print_heap(void);

/* bestfit */
void printHeap(void);

#endif
