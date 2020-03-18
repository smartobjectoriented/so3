//
// Created by julien on 3/14/20.
//

/*
 * Copyright (c) 2017 Simon Goldschmidt
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Simon Goldschmidt <goldsimon@gmx.de>
 *
 */

/* lwIP includes. */
#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/sys.h"
#include "lwip/mem.h"
#include "lwip/stats.h"

#include <delay.h>
#include <mutex.h>
#include <heap.h>
#include <timer.h>




mutex_t *light_protect_mutex;



/* Initialize this module (see description in sys.h) */
void sys_init(void)
{
    light_protect_mutex = (mutex_t*)malloc(sizeof(mutex_t));
    // INIT mutex
}


u32_t sys_now(void)
{
    return NOW() / 1000000ull;
    // time in ms
}

u32_t sys_jiffies(void)
{
    return NOW();
    // return monotonic time in NS
}


/**
 * SYS_LIGHTWEIGHT_PROT
 * define SYS_LIGHTWEIGHT_PROT in lwipopts.h if you want inter-task protection
 * for certain critical regions during buffer allocation, deallocation and memory
 * allocation and deallocation.
 */
#if SYS_LIGHTWEIGHT_PROT

sys_prot_t sys_arch_protect(void)
{
    // We can use mutex because SO3 mutexes support recursive calls
    mutex_lock(light_protect_mutex);
    return 1;
}

void sys_arch_unprotect(sys_prot_t pval)
{
    mutex_unlock(light_protect_mutex);
}

#endif /* SYS_LIGHTWEIGHT_PROT */

void sys_arch_msleep(u32_t delay_ms)
{
    msleep(delay_ms);
}

// If mutex are availible use this code
#if !LWIP_COMPAT_MUTEX

/* Create a new mutex*/
err_t sys_mutex_new(sys_mutex_t *mutex)
{
    mutex_t *so3_mutex;
    LWIP_ASSERT("mutex != NULL", mutex != NULL);

    // Alloc a new mutex
    so3_mutex = (mutex_t*)malloc(sizeof(mutex_t));
    mutex_init(so3_mutex);

    mutex->mut = (void*)so3_mutex;

    if(mutex->mut == NULL) {
        return ERR_MEM;
    }
    return ERR_OK;
}

void sys_mutex_lock(sys_mutex_t *mutex)
{
    LWIP_ASSERT("mutex != NULL", mutex != NULL);
    LWIP_ASSERT("mutex->mut != NULL", mutex->mut != NULL);

    mutex_lock(mutex->mut);
}

void sys_mutex_unlock(sys_mutex_t *mutex)
{
    LWIP_ASSERT("mutex != NULL", mutex != NULL);
    LWIP_ASSERT("mutex->mut != NULL", mutex->mut != NULL);

    mutex_unlock(mutex->mut);
}

void sys_mutex_free(sys_mutex_t *mutex)
{
    LWIP_ASSERT("mutex != NULL", mutex != NULL);
    LWIP_ASSERT("mutex->mut != NULL", mutex->mut != NULL);

    free(mutex->mut);

    mutex->mut = NULL;
}

#endif /* !LWIP_COMPAT_MUTEX */

err_t sys_sem_new(sys_sem_t *sem, u8_t initial_count)
{
    mutex_t *so3_mutex;
    LWIP_ASSERT("sem != NULL", sem != NULL);
    LWIP_ASSERT("initial_count invalid (not 0 or 1)", (initial_count == 0) || (initial_count == 1));

    // Alloc a new mutex
    so3_mutex = (mutex_t*)malloc(sizeof(mutex_t));

    mutex_init(so3_mutex);

    sem->mutex = (void*)so3_mutex;
    sem->counter = initial_count;

    if(sem->counter == NULL  || sem->mutex == NULL) {
        return ERR_MEM;
    }
    return ERR_OK;
}

void sys_sem_signal(sys_sem_t *sem)
{
    LWIP_ASSERT("sem != NULL", sem != NULL);
    LWIP_ASSERT("sem->mutex != NULL", sem->mutex != NULL);

    mutex_lock(sem->mutex);
    sem->counter++;
    mutex_unlock(sem->mutex);
}

u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout_ms)
{
    u32_t start = (u32_t)(get_s_time() / 1000000ull);


    LWIP_ASSERT("sem != NULL", sem != NULL);
    LWIP_ASSERT("sem->mutex != NULL", sem->mutex != NULL);


    mutex_lock(sem->mutex);

    while(sem->counter <= 0)
    {
        mutex_unlock(sem->mutex);
        msleep(2);
        mutex_lock(sem->mutex);

        if(timeout_ms && (u32_t)(get_s_time() / 1000000ull) - start >= timeout_ms)
        {
            mutex_unlock(sem->mutex);
            return SYS_ARCH_TIMEOUT;
        }
    }

    // Take ressource
    sem->counter--;
    mutex_unlock(sem->mutex);

    return 1;
}

void sys_sem_free(sys_sem_t *sem)
{
    LWIP_ASSERT("sem != NULL", sem != NULL);
    LWIP_ASSERT("sem->mut != NULL", sem->mut != NULL);

    free(sem->mutex);

    sem->mutex = NULL;
}

#define SYS_MBOX_SIZE 128
struct _mbox  {
    u32_t first, last;
    sys_sem_t *not_empty;
    sys_sem_t *not_full;
    void *mutex;
    int *wait_send;
    void *msgs[SYS_MBOX_SIZE];
};
typedef struct _mbox _mbox_t;



err_t sys_mbox_new(sys_mbox_t *sys_mbox, int size)
{
    _mbox_t *mbox;
    sys_sem_t *not_empty;
    sys_sem_t *not_full;
    mutex_t *mutex;
    LWIP_UNUSED_ARG(size);

    mbox = (_mbox_t *)malloc(sizeof(_mbox_t));
    not_empty = (sys_sem_t*)malloc(sizeof(sys_sem_t));
    not_full = (sys_sem_t*)malloc(sizeof(sys_sem_t));
    mutex = (mutex_t*)malloc(sizeof(mutex_t));

    if (mbox == NULL || not_empty == NULL || not_full == NULL || mutex == NULL) {
        return ERR_MEM;
    }


    if(sys_sem_new(not_empty, 0) == ERR_MEM || sys_sem_new(not_full, 0) == ERR_MEM){
        return ERR_MEM;
    }

    mutex_init(mutex);

    mbox->first = mbox->last = 0;
    mbox->not_empty = not_empty;
    mbox->not_full = not_full;
    mbox->mutex = mutex;
    mbox->wait_send = 0;

    SYS_STATS_INC_USED(mbox);
    sys_mbox->mbox = mbox;
    return ERR_OK;
}

void sys_mbox_post(sys_mbox_t *sys_mbox, void *msg)
{
    _mbox_t* mbox;
    uint32_t first = 0;

    LWIP_ASSERT("invalid mbox", (sys_mbox_t != NULL) && (sys_mbox->mbox != NULL));


    mbox = sys_mbox->mbox;

    mutex_lock(mbox->mutex);

    while ((mbox->last + 1) >= (mbox->first + SYS_MBOX_SIZE)) {
        mbox->wait_send++;
        mutex_unlock(mbox->mutex);
        sys_arch_sem_wait(mbox->not_full, 0);
        mutex_lock(mbox->mutex);
        mbox->wait_send--;
    }

    mbox->msgs[mbox->last % SYS_MBOX_SIZE] = msg;

    if (mbox->last == mbox->first) {
        first = 1;
    } else {
        first = 0;
    }

    mbox->last++;

    if (first) {
        sys_sem_signal(mbox->not_empty);
    }

    mutex_unlock(mbox->mutex);

}

err_t sys_mbox_trypost(sys_mbox_t *sys_mbox, void *msg)
{
    u8_t first;
    _mbox_t* mbox;

    LWIP_ASSERT("invalid mbox", (sys_mbox != NULL) && (sys_mbox->mbox != NULL));

    mbox = sys_mbox->mbox;

    mutex_lock(mbox->mutex);

    LWIP_DEBUGF(SYS_DEBUG, ("sys_mbox_trypost: mbox %p msg %p\n", (void *)mbox, (void *)msg));

    if ((mbox->last + 1) >= (mbox->first + SYS_MBOX_SIZE)) {
        mutex_unlock(mbox->mutex);
        return ERR_MEM;
    }

    mbox->msgs[mbox->last % SYS_MBOX_SIZE] = msg;

    if (mbox->last == mbox->first) {
        first = 1;
    } else {
        first = 0;
    }

    mbox->last++;

    if (first) {
        sys_sem_signal(mbox->not_empty);
    }

    mutex_unlock(mbox->mutex);

    return ERR_OK;

}

err_t sys_mbox_trypost_fromisr(sys_mbox_t *sys_mbox, void *msg)
{
    return sys_mbox_trypost(sys_mbox, msg);
}

u32_t sys_arch_mbox_fetch(sys_mbox_t *sys_mbox, void **msg, u32_t timeout_ms)
{
    u32_t time_needed = 0;
    _mbox_t* mbox;

    LWIP_ASSERT("invalid mbox", (sys_mbox_t != NULL) && (sys_mbox->mbox != NULL));

    mbox = sys_mbox->mbox;

    /* The mutex lock is quick so we don't bother with the timeout
       stuff here. */
    mutex_lock(mbox->mutex);

    while (mbox->first == mbox->last) {
        mutex_unlock(mbox->mutex);

        /* We block while waiting for a mail to arrive in the mailbox. We
           must be prepared to timeout. */
        if (timeout_ms != 0) {
            time_needed = sys_arch_sem_wait(mbox->not_empty, timeout_ms);

            if (time_needed == SYS_ARCH_TIMEOUT) {
                return SYS_ARCH_TIMEOUT;
            }
        } else {
            sys_arch_sem_wait(mbox->not_empty, 0);
        }

        mutex_lock(mbox->mutex);
    }

    if (msg != NULL) {
        LWIP_DEBUGF(SYS_DEBUG, ("sys_mbox_fetch: mbox %p msg %p\n", (void *)mbox, *msg));
        *msg = mbox->msgs[mbox->first % SYS_MBOX_SIZE];
    }
    else{
        LWIP_DEBUGF(SYS_DEBUG, ("sys_mbox_fetch: mbox %p, null msg\n", (void *)mbox));
    }

    mbox->first++;

    if (mbox->wait_send) {
        sys_sem_signal(mbox->not_full);
    }

    mutex_unlock(mbox->mutex);

    return time_needed;
}

u32_t sys_arch_mbox_tryfetch(sys_mbox_t *sys_mbox, void **msg)
{
    _mbox_t* mbox;
    LWIP_ASSERT("invalid mbox", (sys_mbox_t != NULL) && (sys_mbox->mbox != NULL));

    mbox = sys_mbox->mbox;

    mutex_lock(mbox->mutex);

    if (mbox->first == mbox->last) {
        mutex_unlock(mbox->mutex);
        return SYS_MBOX_EMPTY;
    }

    if (msg != NULL) {
        LWIP_DEBUGF(SYS_DEBUG, ("sys_mbox_tryfetch: mbox %p msg %p\n", (void *)mbox, *msg));
        *msg = mbox->msgs[mbox->first % SYS_MBOX_SIZE];
    }
    else{
        LWIP_DEBUGF(SYS_DEBUG, ("sys_mbox_tryfetch: mbox %p, null msg\n", (void *)mbox));
    }

    mbox->first++;

    if (mbox->wait_send) {
        sys_sem_signal(mbox->not_full);
    }

    mutex_unlock(mbox->mutex);

    return 0;
}

void sys_mbox_free(sys_mbox_t *sys_mbox)
{

    if ((sys_mbox != NULL) && (sys_mbox->mbox != NULL)) {
        _mbox_t *mbox = sys_mbox->mbox;
        SYS_STATS_DEC(mbox.used);

        sys_sem_free(mbox->not_empty);
        sys_sem_free(mbox->not_full);
        free(mbox->mutex);
        mbox->not_empty = mbox->not_full = mbox->mutex = NULL;
        free(mbox);
    }

}

sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread, void *arg, int stacksize, int prio)
{

}


#if LWIP_FREERTOS_CHECK_CORE_LOCKING
#if LWIP_TCPIP_CORE_LOCKING

/** Flag the core lock held. A counter for recursive locks. */
static u8_t lwip_core_lock_count;
static TaskHandle_t lwip_core_lock_holder_thread;

void
sys_lock_tcpip_core(void)
{
   sys_mutex_lock(&lock_tcpip_core);
   if (lwip_core_lock_count == 0) {
     lwip_core_lock_holder_thread = xTaskGetCurrentTaskHandle();
   }
   lwip_core_lock_count++;
}

void
sys_unlock_tcpip_core(void)
{
   lwip_core_lock_count--;
   if (lwip_core_lock_count == 0) {
       lwip_core_lock_holder_thread = 0;
   }
   sys_mutex_unlock(&lock_tcpip_core);
}

#endif /* LWIP_TCPIP_CORE_LOCKING */

#if !NO_SYS
static TaskHandle_t lwip_tcpip_thread;
#endif

void
sys_mark_tcpip_thread(void)
{
#if !NO_SYS
  lwip_tcpip_thread = xTaskGetCurrentTaskHandle();
#endif
}

void
sys_check_core_locking(void)
{
  /* Embedded systems should check we are NOT in an interrupt context here */
  /* E.g. core Cortex-M3/M4 ports:
         configASSERT( ( portNVIC_INT_CTRL_REG & portVECTACTIVE_MASK ) == 0 );

     Instead, we use more generic FreeRTOS functions here, which should fail from ISR: */
  taskENTER_CRITICAL();
  taskEXIT_CRITICAL();

#if !NO_SYS
  if (lwip_tcpip_thread != 0) {
    TaskHandle_t current_thread = xTaskGetCurrentTaskHandle();

#if LWIP_TCPIP_CORE_LOCKING
    LWIP_ASSERT("Function called without core lock",
                current_thread == lwip_core_lock_holder_thread && lwip_core_lock_count > 0);
#else /* LWIP_TCPIP_CORE_LOCKING */
    LWIP_ASSERT("Function called from wrong thread", current_thread == lwip_tcpip_thread);
#endif /* LWIP_TCPIP_CORE_LOCKING */
  }
#endif /* !NO_SYS */
}

#endif /* LWIP_FREERTOS_CHECK_CORE_LOCKING*/
