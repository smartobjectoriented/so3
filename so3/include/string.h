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

#ifndef STRING_H
#define STRING_H

#ifndef NULL
#define NULL 0
#endif

#include <stdarg.h>
#include <types.h>

void *memchr(const void *, int, size_t);
int memcmp(const void *, const void *, size_t);
void *memcpy(void *, const void *, size_t);
void *memmove(void *, const void *, size_t);
void *memset(void *, int, size_t);

void downcase(char *str);

void uppercase(char *str, int len);

char *strchr(const char *, int);
char *strsep(char **str, const char *sep);
char *strsep(char **str, const char *sep);

int strcmp(const char *, const char *);
int strncmp(const char *, const char *, size_t);
size_t strnlen(const char * s, size_t count);

char *strdup(const char *s);

int sprintf(char *buf, const char *fmt, ...);
int scnprintf(char * buf, size_t size, const char *fmt, ...);

char *strcpy(char *, const char *);
size_t strlen(const char *);
char *strncpy(char *, const char *, size_t);
char *strcat(char *dest, const char *src);

int vsprintf(char *s, const char *fmt, va_list ap);
int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);
int vsscanf(const char *buf, const char *fmt, va_list args);
int sscanf(const char * buf, const char * fmt, ...);
int sprintf(char *buf, const char *fmt, ...);
int snprintf(char * buf, size_t size, const char *fmt, ...);

char *kvasprintf(const char *fmt, va_list ap);
char *kasprintf(const char *fmt, ...);

unsigned long simple_strtoul(const char *cp, char **endp, unsigned int base);
long simple_strtol(const char *cp, char **endp, unsigned int base);
unsigned long long simple_strtoull(const char *cp, char **endp, unsigned int base);
long long simple_strtoll(const char *cp, char **endp, unsigned int base);

char *strrchr(const char *s, int c);
char *strchrnul(const char *s, int c);

/**
 * kbasename - return the last part of a pathname.
 *
 * @path: path to extract the filename from.
 */
static inline const char *kbasename(const char *path)
{
	const char *tail = strrchr(path, '/');
	return tail ? tail + 1 : path;
}

#endif /* STRING_H */
