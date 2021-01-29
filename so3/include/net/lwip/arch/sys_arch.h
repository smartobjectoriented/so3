/*
 * Copyright (C) 2020 Julien Quartier <julien.quartier@heig-vd.ch>
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

#ifndef SYS_ARCH_H
#define SYS_ARCH_H

#include "lwip/opt.h"
#include "lwip/arch.h"
#include <semaphore.h>
#include <mutex.h>

#define sys_msleep(ms) sys_arch_msleep(ms)

#if SYS_LIGHTWEIGHT_PROT
typedef u32_t sys_prot_t;
#endif

struct _sys_mut {
    mutex_t *mut;
};
typedef struct _sys_mut sys_mutex_t;

#define sys_mutex_valid_val(_mutex) ((_mutex).mut != NULL)
#define sys_mutex_valid(_mutex) (((_mutex) != NULL) && ((_mutex)->mut != NULL))
#define sys_mutex_set_invalid(_mutex) do { if((_mutex) != NULL) (_mutex)->mut = NULL; }while(0)

// https://www.careercup.com/question?id=1892664
// https://stackoverflow.com/questions/20534782/implementing-semaphore-by-using-mutex-operations-and-primitives
struct _sys_sem {
    sem_t *sem;
};
typedef struct _sys_sem sys_sem_t;
#define sys_sem_valid_val(_sema)   ((_sema).sem != NULL)
#define sys_sem_valid(_sema)       (((_sema) != NULL) && ((_sema)->sem != NULL))
#define sys_sem_set_invalid(_sema) do { if((_sema) != NULL) (_sema)->sem = NULL; }while(0)


#define SYS_MBOX_SIZE 128
struct _mbox  {
        u32_t first, last;
        sem_t *not_empty;
        sem_t *not_full;
        void *mutex;
        int *wait_send;
        void *msgs[SYS_MBOX_SIZE];
};
typedef struct _mbox _mbox_t;

struct _sys_mbox {
        _mbox_t *mbox;
};
typedef struct _sys_mbox sys_mbox_t;

#define sys_mbox_valid_val(_mbox)   ((_mbox).mbox != NULL)
#define sys_mbox_valid(_mbox)       (((_mbox) != NULL) && ((_mbox)->mbox != NULL))
#define sys_mbox_set_invalid(_mbox) do { if((_mbox) != NULL) (_mbox)->mbox = NULL; }while(0)

struct _sys_thread {
    void *thread_handle;
};
typedef struct _sys_thread sys_thread_t;



#endif /* SYS_ARCH_H */
