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

#ifndef FB_H
#define FB_H

#define IOCTL_FB_HRES 1 /* Horizontal Resolution */
#define IOCTL_FB_VRES 2 /* Vertical Resolution */
#define IOCTL_FB_SIZE 3 /* Total Size. Depends on color format */

/* 
 * Ensures compatibility with user-space applications.  
 * User-space will avoid calling `mmap` on a framebuffer driver  
 * that identifies itself as "fake".  
 */
#define IOCTL_FB_IS_REAL 4

#define IOCTL_FB_BPP 5 /* Bit per pixel. */

#endif /* FB_H */
