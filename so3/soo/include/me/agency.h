/*
 * Copyright (C) 2019-2020 David Truan <david.truan@heig-vd.ch>
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


#ifndef AGENCY_H
#define AGENCY_H

/* Struct embedding the version numbers of three main the agency components*/
typedef struct {
    unsigned int itb;
    unsigned int uboot;
    unsigned int rootfs;
} upgrade_versions_args_t;

extern const unsigned char upgrade_image[];
extern const unsigned long upgrade_image_length;

extern upgrade_versions_args_t get_agency_versions(void);


#endif /* AGENCY_H */
