/*
 * Copyright (C) 2025 André Costa <andre_miguel_costa@hotmail.com>
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
#ifndef LOG_H
#define LOG_H

#include <printk.h>

#ifdef CONFIG_VLOGS_FRONTEND
#include <soo/soo.h>
#include <soo/dev/vlogs.h>
#endif

#define LOG_LEVEL_CRITICAL 1
#define LOG_LEVEL_ERROR 2
#define LOG_LEVEL_WARNING 3
#define LOG_LEVEL_INFO 4
#define LOG_LEVEL_DEBUG 5
#define LOG_LEVEL_TRACE 6

#ifndef CONFIG_LOG_LEVEL
#define CONFIG_LOG_LEVEL LOG_LEVEL_INFO
#endif

#if defined(CONFIG_AVZ)

#define LOG(level, fmt, ...)                                                                     \
	do {                                                                                     \
		lprintk("[SO3:AVZ " #level "] <%s:%d> " fmt, __func__, __LINE__, ##__VA_ARGS__); \
	} while (0)

#elif defined(CONFIG_VLOGS_FRONTEND)

#define LOG(level, fmt, ...)                                                                                                   \
	do {                                                                                                                   \
		if (vlogs_ready())                                                                                             \
			vlogs_write("[ME:%d][" #level "] <%s:%d> " fmt, get_ME_desc()->slotID, __func__, __LINE__,             \
				    ##__VA_ARGS__);                                                                            \
		else                                                                                                           \
			lprintk("[ME:%d][" #level "] <%s:%d> " fmt, get_ME_desc()->slotID, __func__, __LINE__, ##__VA_ARGS__); \
	} while (0)

#else

#define LOG(level, fmt, ...)                                                                 \
	do {                                                                                 \
		lprintk("[SO3 " #level "] <%s:%d> " fmt, __func__, __LINE__, ##__VA_ARGS__); \
	} while (0)

#endif /* !CONFIG_AVZ */

#if CONFIG_LOG_LEVEL >= LOG_LEVEL_CRITICAL
#define LOG_CRITICAL(fmt, ...) LOG(CRITICAL, fmt, ##__VA_ARGS__)

#else
#define LOG_CRITICAL(fmt, ...)
#endif

#if CONFIG_LOG_LEVEL >= LOG_LEVEL_ERROR
#define LOG_ERROR(fmt, ...) LOG(ERROR, fmt, ##__VA_ARGS__)

#else
#define LOG_ERROR(fmt, ...)
#endif

#if CONFIG_LOG_LEVEL >= LOG_LEVEL_WARNING
#define LOG_WARNING(fmt, ...) LOG(WARNING, fmt, ##__VA_ARGS__)

#else
#define LOG_WARNING(fmt, ...)
#endif

#if CONFIG_LOG_LEVEL >= LOG_LEVEL_INFO
#define LOG_INFO(fmt, ...) LOG(INFO, fmt, ##__VA_ARGS__)

#else
#define LOG_INFO(fmt, ...)
#endif

#if CONFIG_LOG_LEVEL >= LOG_LEVEL_DEBUG
#define LOG_DEBUG(fmt, ...) LOG(DEBUG, fmt, ##__VA_ARGS__)

#else
#define LOG_DEBUG(fmt, ...)
#endif

#if CONFIG_LOG_LEVEL >= LOG_LEVEL_TRACE
#define LOG_TRACE(fmt, ...) LOG(TRACE, fmt, ##__VA_ARGS__)

#else
#define LOG_TRACE(fmt, ...)
#endif

#endif
