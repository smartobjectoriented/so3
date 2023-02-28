/*
 * Copyright (C) 2018 Daniel Rossier <daniel.rossier@soo.tech>
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

#ifndef LOGBOOL_H
#define LOGBOOL_H

#include <stdbool.h>

/* Number of (key, value) pair in the hashtable. The number is taken from the code example. */
#define LOGBOOL_HT_SIZE 	4096

/* Maximal size of the logbool string */
#define LOGBOOL_STR_SIZE	160

#if 0
#define ENABLE_LOGBOOL
#endif

typedef struct {
    bool section_state;	/* on / off */
    int cpu;
    int count;   /* Number of visits in this section */
} logbool_t;

struct entry_s {
	char *key;
	logbool_t value;
	struct entry_s *next;
};

typedef struct entry_s entry_t;

struct hashtable_s {
	int size;
	struct entry_s **table;
};

typedef struct hashtable_s logbool_hashtable_t;

#ifdef __AVZ__
extern	logbool_hashtable_t *avz_logbool_ht;
extern void logbool_init(void);
#define logbool_hashtable avz_logbool_ht
#else /* !__AVZ__ */
typedef void(*ht_set_t)(logbool_hashtable_t *, char *, logbool_t);
extern ht_set_t __ht_set;
#define logbool_hashtable avz_shared_info->logbool_ht
#endif

extern void ht_set(logbool_hashtable_t *hashtable, char *key, logbool_t value);
extern int sprintf(char * buf, const char *fmt, ...);

#ifdef ENABLE_LOGBOOL
#define __LOGBOOL_ON \
    { \
        char __logbool_str[LOGBOOL_STR_SIZE]; \
        static logbool_t logbool = { 0 }; \
        sprintf(__logbool_str, "%s:%s:%d", __FILE__, __func__, __LINE__); \
        logbool.section_state = true; \
        logbool.cpu = smp_processor_id(); \
        logbool.count++; \
        ht_set(logbool_hashtable, __logbool_str, logbool);

#define __LOGBOOL_OFF \
        logbool.section_state = false;          \
        ht_set(logbool_hashtable, __logbool_str, logbool); \
    }
#else
#define __LOGBOOL_ON
#define __LOGBOOL_OFF
#endif /* ENABLE_LOGBOOL */

void *ht_create(int size);
void ht_destroy(logbool_hashtable_t *hashtable);

void dump_all_logbool(unsigned char key);


#endif /* LOGBOOL_H */

