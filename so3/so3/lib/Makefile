#
# Makefile for some libs needed in the kernel.
#

lib-y += string.o string_helpers.o lprintk.o
lib-y += ctype.o
lib-y += vsprintf.o printk.o
ifneq ($(CONFIG_ARCH_X86_64),y)
lib-y += strtod.o

# strtod() computes in doubles; libroxml's XPath is its only caller.

CFLAGS_REMOVE_strtod.o = -mgeneral-regs-only
endif

lib-y += list_sort.o
lib-y += hashmap.o

