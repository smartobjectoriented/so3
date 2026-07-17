#ifndef BANNER_H
#define BANNER_H

#include <version.h>

/*
 * CONFIG_ARCH ("arm32", "arm64", ...) is spelled out in the banner because the
 * same platform can be built either way — on a Raspberry Pi 4 in particular,
 * where a 32- and a 64-bit chain differ only by which kernel*.img the firmware
 * picks up, there is otherwise no way to tell from the console which one
 * actually booted.
 */
#define SO3_BANNER \
	"\n\n********** Smart Object Oriented SO3 Operating System **********\n\
Copyright (c) 2014-2026 REDS Institute, HEIG-VD, Yverdon\n\
Version " SO3_KERNEL_VERSION " (" CONFIG_ARCH ")\n\n"

#endif /* BANNER_H */
