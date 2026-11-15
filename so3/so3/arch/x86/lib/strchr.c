/*
 * Copyright (C) 2026 REDS Institute - HEIG-VD
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <string.h>

char *strchr(const char *s, int c)
{
	for (;; s++) {
		if (*s == (char) c)
			return (char *) s;
		if (!*s)
			return NULL;
	}
}
