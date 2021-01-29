# GNU Makefile for building user programs to run on top of SO3
#
# Things to be aware of:
#   As we are included from the top level Makefile, the current directory
#   is not the one where this Makefile is saved, but the one of the main Makefile.

ifeq ($(USR_DIR),)
USR_DIR = $(PWD)/..
endif

export USR_DIR

LIBC_DIR = $(USR_DIR)/libc
export LIBC_DIR

-include $(USR_DIR)/Makefile.common

LIB = crt0 crt1  pthread inet eabi_compat malloc shgetc intscan floatscan mutex sbrk

CFLAGS	+= -I$(LIBC_DIR)/include

all: $(NLIB)

$(NLIB): libc_root libc_subdirs
	$(AR) rv $@ *.o string/*.o stdio/*.o fcntl/*.o mman/*.o misc/*.o dirent/*.o math/*.o errno/*.o multibyte/*.o ctype/*.o thread/*.o exit/*.o stdlib/*.o unistd/*.o env/*.o malloc/*.o process/*.o prng/*.o signal/*.o time/*.o network/*.o >  /dev/null 

libc_root: $(patsubst %,%.o,$(LIB))

libc_subdirs:
	make -C string
	make -C stdio
	make -C fcntl
	make -C misc
	make -C math
	make -C errno
	make -C multibyte
	make -C ctype
	make -C thread
	make -C exit
	make -C stdlib
	make -C dirent
	make -C unistd
	make -C env
	make -C malloc
	make -C exit
	make -C process
	make -C signal
	make -C time
	make -C prng
	make -C mman
	make -C network

crt.o: crt.S
	$(CC) $(AFLAGS) $(CFLAGS) $(CPPFLAGS) -c $< -o $@
%.o: %.S
	$(CC) $(AFLAGS) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

%.o: %.c 
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@
clean:
	make -C string clean
	make -C stdio clean
	make -C fcntl clean
	make -C math clean
	make -C dirent clean
	make -C misc clean
	make -C errno clean
	make -C multibyte clean
	make -C ctype clean
	make -C thread clean
	make -C exit clean
	make -C stdlib clean
	make -C unistd clean
	make -C env clean
	make -C malloc clean
	make -C exit clean
	make -C process clean
	make -C signal clean
	make -C time clean
	make -C prng clean
	make -C mman clean
	make -C network clean
	rm -f *.o
	rm -f libso3.a



