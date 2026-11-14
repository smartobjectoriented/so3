#
# Makefile for some libs needed in the kernel.
#

lib-y += string.o string_helpers.o lprintk.o
lib-y += ctype.o
lib-y += vsprintf.o printk.o
ifneq ($(CONFIG_ARCH_X86_64),y)
lib-y += strtod.o
endif
lib-y += list_sort.o
lib-y += hashmap.o

