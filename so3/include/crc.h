/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2009
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Prafulla Wadaskar <prafulla@marvell.com>
 */

#ifndef _UBOOT_CRC_H
#define _UBOOT_CRC_H

#include <types.h>

/* lib/crc8.c */
unsigned int crc8(unsigned int crc_start, const unsigned char *vptr, int len);

/* lib/crc32.c */
uint32_t crc32 (uint32_t, const unsigned char *, uint);
uint32_t crc32_wd (uint32_t, const unsigned char *, uint, uint);
uint32_t crc32_no_comp (uint32_t, const unsigned char *, uint);

/* lib/crc32c.c */
void crc32c_init(uint32_t *, uint32_t);
uint32_t crc32c_cal(uint32_t, const char *, int, uint32_t *);

unsigned int xcrc32 (const unsigned char *buf, int len, unsigned int init);

#endif /* _UBOOT_CRC_H */
