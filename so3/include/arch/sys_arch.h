//
// Created by julien on 3/12/20.
//

#ifndef SO3_SYS_ARCH_H
#define SO3_SYS_ARCH_H

#include "lwip/opt.h"
#include "lwip/arch.h"


#define sys_msleep(ms) sys_arch_msleep(ms)

#if SYS_LIGHTWEIGHT_PROT
typedef u32_t sys_prot_t;
#endif

struct _sys_mut {
    void *mut;
};
typedef struct _sys_mut sys_mutex_t;

#define sys_mutex_valid_val(_mutex) ((_mutex).mut != NULL)
#define sys_mutex_valid(_mutex) (((_mutex) != NULL))
#define sys_mutex_set_invalid(_mutex) ((_mutex)->mut = NULL)

// https://www.careercup.com/question?id=1892664
// https://stackoverflow.com/questions/20534782/implementing-semaphore-by-using-mutex-operations-and-primitives
struct _sys_sem {
    u32_t counter;
    void *mutex;

};
typedef struct _sys_sem sys_sem_t;
#define sys_sem_valid_val(_sema)   ((_sema).mutex != NULL)
#define sys_sem_valid(_sema)       (((_sema) != NULL))
#define sys_sem_set_invalid(_sema) ((_sema)->mutex = NULL)


struct _sys_mbox {
    void *mbox;
};
typedef struct _sys_mbox sys_mbox_t;
#define sys_mbox_valid_val(_mbox)   ((_mbox).mbox != NULL)
#define sys_mbox_valid(_mbox)       (((_mbox) != NULL))
#define sys_mbox_set_invalid(_mbox) ((_mbox)->mbox = NULL)

struct _sys_thread {
    void *thread_handle;
};
typedef struct _sys_thread sys_thread_t;

#endif //SO3_SYS_ARCH_H