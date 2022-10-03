/**
 * \file roxml_utils.h
 * \brief misc functions required in libroxml
 *
 * (C) Copyright 2015
 * Tristan Lelong <tristan.lelong@libroxml.net>
 *
 * SPDX-Licence-Identifier:     LGPL-2.1+
 * The author added a static linking exception, see License.txt.
 */

#ifndef ROXML_UTILS_H
#define ROXML_UTILS_H

#include <heap.h>
#include <schedule.h>
#include <mutex.h>
#include <strtox.h>

#if 0 /* SOO.tech */
#include <stdlib.h>
#endif

#if defined(_WIN32)
#include "roxml_win32_native.h"
#else

#if 0
#include <pthread.h>
#endif

#endif

#if(CONFIG_XML_THREAD_SAFE==0)
ROXML_STATIC_INLINE ROXML_INT unsigned long int roxml_thread_id(node_t *n)
{
	return 0;
}

ROXML_STATIC_INLINE ROXML_INT int roxml_lock_init(node_t *n)
{
	return 0;
}

ROXML_STATIC_INLINE ROXML_INT int roxml_lock_destroy(node_t *n)
{
	return 0;
}

ROXML_STATIC_INLINE ROXML_INT int roxml_lock(node_t *n)
{
	return 0;
}

ROXML_STATIC_INLINE ROXML_INT int roxml_unlock(node_t *n)
{
	return 0;
}
#else /* CONFIG_XML_THREAD_SAFE==1 */
ROXML_STATIC_INLINE ROXML_INT unsigned long int roxml_thread_id(node_t *n)
{
	/* return pthread_self(); */
	return current()->tid;
}

ROXML_STATIC_INLINE ROXML_INT int roxml_lock_init(node_t *n)
{
	xpath_tok_table_t *table = (xpath_tok_table_t *)n->priv;
	table->lock = malloc(sizeof(struct mutex));

	mutex_init((struct mutex *) table->lock);

	return 0;
}

ROXML_STATIC_INLINE ROXML_INT int roxml_lock_destroy(node_t *n)
{
	xpath_tok_table_t *table = (xpath_tok_table_t *)n->priv;

	free(table->lock);

	return 0;
}

ROXML_STATIC_INLINE ROXML_INT int roxml_lock(node_t *n)
{
	xpath_tok_table_t *table;
	while (n->prnt)
		n = n->prnt;
	
	table = (xpath_tok_table_t *)n->priv;

	mutex_lock((struct mutex *) table->lock);

	return 0;
}

ROXML_STATIC_INLINE ROXML_INT int roxml_unlock(node_t *n)
{
	xpath_tok_table_t *table;
	while (n->prnt)
		n = n->prnt;
	
	table = (xpath_tok_table_t *)n->priv;

	mutex_unlock((struct mutex *) table->lock);

	return 0;
}
#endif /* CONFIG_XML_THREAD_SAFE */

#ifdef CONFIG_XML_FLOAT
ROXML_STATIC_INLINE ROXML_INT double roxml_strtonum(const char *str, char **end)
{
	return strtod(str, end);
}
#else /* CONFIG_XML_FLOAT */
ROXML_STATIC_INLINE ROXML_INT double roxml_strtonum(const char *str, char **end)
{
	int value = strtol(str, end, 0);

	/* if the value is a float:
	 * it must be considered a number and we floor it
	 */
	if (end && *end && **end == '.')
		strtol(*(end+1), end, 0);

	return value;

}
#endif /* CONFIG_XML_FLOAT */

#endif /* ROXML_UTILS */
