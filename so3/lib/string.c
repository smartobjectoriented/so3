/*
 * Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
 * Copyright (C) 2014 Romain Bornet <romain.bornet@heig-vd.ch>
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

#include <compiler.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>
#include <heap.h>

#define __ALIGN (sizeof(size_t))
#define ONES ((size_t)-1/UCHAR_MAX)
#define HIGHS (ONES * (UCHAR_MAX/2+1))
#define HASZERO(x) ((x)-(ONES & ~(x) & HIGHS))

#define BITOP(a,b,op) \
 ((a)[(size_t)(b)/(8*sizeof *(a))] op (size_t)1<<((size_t)(b)%(8*sizeof *(a))))

void uppercase(char *str, int len)
{
	int i;

	for (i = 0; i < len; i++) {
		*str = toupper(*str);
		str++;
	}
}

/*
 * Convert a string to lowercase.
 */
void downcase(char *str)
{
	while (*str != '\0') {
		*str = tolower(*str);
		str++;
	}
}

/* http://clc-wiki.net/wiki/memchr#Implementation */
void *memchr(const void *s, int c, size_t n)
{
    unsigned char *p = (unsigned char*)s;
    while( n-- )
        if( *p != (unsigned char)c )
            p++;
        else
            return p;
    return 0;
}

/* http://clc-wiki.net/wiki/memcmp#Implementation */
int memcmp(const void* s1, const void* s2,size_t n)
{
    const unsigned char *p1 = s1, *p2 = s2;
    while(n--)
        if( *p1 != *p2 )
            return *p1 - *p2;
        else
            p1++,p2++;
    return 0;
}

/* http://clc-wiki.net/wiki/memcpy#Implementation */
void *memcpy(void *dest, const void *src, size_t n)
{
    char *dp = dest;
    const char *sp = src;
    while (n--)
        *dp++ = *sp++;
    return dest;
}

/* http://clc-wiki.net/wiki/memmove#Implementation
 * using naive and non-portable '__np_anyptrlt' macro: see website for info */
#define __np_anyptrlt(p1, p2) ((p1) < (p2))

void *memmove(void *dest, const void *src, size_t n)
{
    unsigned char *pd = dest;
    const unsigned char *ps = src;
    if (__np_anyptrlt(ps, pd))
        for (pd += n, ps += n; n--;)
            *--pd = *--ps;
    else
        while(n--)
            *pd++ = *ps++;
    return dest;
}

/* http://clc-wiki.net/wiki/memset#Implementation */
void *memset(void *s, int c, size_t n)
{
    unsigned char* p=s;
    while(n--)
        *p++ = (unsigned char)c;
    return s;
}

/* http://clc-wiki.net/wiki/strcmp#Implementation */
int strcmp(const char* s1, const char* s2)
{
    while(*s1 && (*s1==*s2))
        s1++,s2++;
    return *(const unsigned char*)s1-*(const unsigned char*)s2;
}

/**
 * strncmp - Compare two length-limited strings
 * @cs: One string
 * @ct: Another string
 * @count: The maximum number of bytes to compare
 */
int strncmp(const char * cs,const char * ct,size_t count)
{
        register signed char __res = 0;

        while (count) {
                if ((__res = *cs - *ct++) != 0 || !*cs++)
                        break;
                count--;
        }

        return __res;
}

char *strcat(char *dest, const char *src)
{
	strcpy(dest + strlen(dest), src);
	return dest;
}

char *strcpy(char *dest, const char* src)
{
    char *ret = dest;
    while ((*dest++ = *src++))
        ;
    return ret;
}

size_t strnlen(const char * s, size_t count)
{
	const char *sc;

	for (sc = s; count-- && *sc != '\0'; ++sc)
		/* nothing */;
	return sc - s;
}

/* http://clc-wiki.net/wiki/strlen#Implementation */
size_t strlen(const char *s) {
    size_t i;
    for (i = 0; s[i] != '\0'; i++) ;
    return i;
}

/* http://clc-wiki.net/wiki/strncpy#Implementation */
char *strncpy(char *dest, const char *src, size_t n)
{
    char *ret = dest;
    do {
        if (!n--)
            return ret;
    } while ( (*dest++ = *src++) );
    while (n--)
        *dest++ = 0;
    return ret;
}

char *strchrnul(const char *s, int c)
{
	size_t *w, k;

	c = (unsigned char)c;
	if (!c) return (char *)s + strlen(s);

	for (; (uintptr_t)s % __ALIGN; s++)
		if (!*s || *(unsigned char *)s == c) return (char *)s;
	k = ONES * c;
	for (w = (void *)s; !HASZERO(*w) && !HASZERO(*w^k); w++);
	for (s = (void *)w; *s && *(unsigned char *)s != c; s++);
	return (char *)s;
}


size_t strcspn(const char *s, const char *c)
{
	const char *a = s;
	size_t byteset[32/sizeof(size_t)];

	if (!c[0] || !c[1]) return strchrnul(s, *c)-a;

	memset(byteset, 0, sizeof byteset);
	for (; *c && BITOP(byteset, *(unsigned char *)c, |=); c++);
	for (; *s && !BITOP(byteset, *(unsigned char *)s, &); s++);
	return s-a;
}

char *strsep(char **str, const char *sep)
{
	char *s = *str, *end;
	if (!s) return NULL;
	end = s + strcspn(s, sep);
	if (*end) *end++ = 0;
	else end = 0;
	*str = end;
	return s;
}

char *strdup(const char *s)
{
	size_t l = strlen(s);
	char *d = malloc(l+1);
	if (!d) return NULL;
	return memcpy(d, s, l+1);
}

