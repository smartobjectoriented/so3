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
#include <semaphore.h>
#include <heap.h>
#include <timer.h>


mutex_t light_protect_mutex;

/*
 * Initialize this module (see description in sys.h)
 */
void sys_init(void)
{
    mutex_init(&light_protect_mutex);
}

/*
 * Return the current time in ms
 */
u32_t sys_now(void)
{
    return NOW() / 1000000ull;
}

/*
 * Return the current time in ns
 */
u32_t sys_jiffies(void)
{
    return NOW();
}


#if SYS_LIGHTWEIGHT_PROT

sys_prot_t sys_arch_protect(void)
{
    /* We can use mutex because SO3 mutexes support recursive calls */
    mutex_lock(&light_protect_mutex);
    return 1;
}

void sys_arch_unprotect(sys_prot_t pval)
{
    mutex_unlock(&light_protect_mutex);
}

#endif /* SYS_LIGHTWEIGHT_PROT */


void sys_arch_msleep(u32_t delay_ms)
{
    msleep(delay_ms);
}

/*
 * create a new mutex
 */
err_t sys_mutex_new(sys_mutex_t *mutex)
{
    mutex_t *so3_mutex;
    LWIP_ASSERT("mutex != NULL", mutex != NULL);

    so3_mutex = (mutex_t*)malloc(sizeof(mutex_t));
    mutex_init(so3_mutex);

    mutex->mut = (void*)so3_mutex;

    if(mutex->mut == NULL) {
        return ERR_MEM;
    }
    return ERR_OK;
}

/*
 * Lock the mutex
 */
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

err_t sys_sem_new(sys_sem_t *sem, u8_t initial_count)
{
    int i = 1;
    LWIP_ASSERT("sem != NULL", sem != NULL);
    LWIP_ASSERT("initial_count invalid (count >= 0)", (initial_count >= 0));

    sem->sem = (sem_t*)malloc(sizeof(sem_t));

    if(sem->sem == NULL) {
        return ERR_MEM;
    }


    sem_init(sem->sem);

    /* The semaphore initial value is 1. The needed value must be set accordingly */
    if(initial_count == 0)
            sem_down(sem->sem);

    while(initial_count > i++){
        sem_up(sem->sem);
    }

    return ERR_OK;
}

void sys_sem_signal(sys_sem_t *sem)
{
    LWIP_ASSERT("sem != NULL", sem != NULL);
    LWIP_ASSERT("sem->sem != NULL", sem->sem != NULL);

    sem_up(sem->sem);
}

u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout_ms)
{
    int start_time = sys_now();

    if(timeout_ms > 0){
        if(sem_timeddown(sem->sem, timeout_ms * 1000000ull))
            return SYS_ARCH_TIMEOUT;
    } else {
        sem_down(sem->sem);
    }

    return sys_now() - start_time;
}

void sys_sem_free(sys_sem_t *sem)
{
    LWIP_ASSERT("sem != NULL", sem != NULL);
    LWIP_ASSERT("sem->mut != NULL", sem->sem != NULL);

    free(sem->sem);

    sem->sem = NULL;
}





err_t sys_mbox_new(sys_mbox_t *sys_mbox, int size)
{
    _mbox_t *mbox;
    sem_t *not_empty;
    sem_t *not_full;
    mutex_t *mutex;
    LWIP_UNUSED_ARG(size);

    mbox = (_mbox_t *)malloc(sizeof(_mbox_t));
    not_empty = (sem_t*)malloc(sizeof(sem_t));
    not_full = (sem_t*)malloc(sizeof(sem_t));
    mutex = (mutex_t*)malloc(sizeof(mutex_t));

    if (mbox == NULL || not_empty == NULL || not_full == NULL || mutex == NULL) {
        return ERR_MEM;
    }


    sem_init(not_empty);
    sem_init(not_full);
    sem_down(not_empty);
    sem_down(not_full);


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
        sem_down(mbox->not_full);
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
        sem_up(mbox->not_empty);
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
        sem_up(mbox->not_empty);
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
    int start_time = sys_now();
    _mbox_t* mbox;

    LWIP_ASSERT("invalid mbox", (sys_mbox != NULL) && (sys_mbox->mbox != NULL));

    mbox = sys_mbox->mbox;

    /* The mutex lock is quick so we don't bother with the timeout
       stuff here. */
    mutex_lock(mbox->mutex);

    while (mbox->first == mbox->last) {
        mutex_unlock(mbox->mutex);

        /* We block while waiting for a mail to arrive in the mailbox. We
           must be prepared to timeout. */
        if (timeout_ms > 0) {
            if (sem_timeddown(mbox->not_empty, timeout_ms * 1000000ull)) {
                return SYS_ARCH_TIMEOUT;
            }
        } else {
            sem_down(mbox->not_empty);
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
        sem_up(mbox->not_full);
    }

    mutex_unlock(mbox->mutex);

    return sys_now() - start_time;
}

u32_t sys_arch_mbox_tryfetch(sys_mbox_t *sys_mbox, void **msg)
{
    _mbox_t* mbox;
    LWIP_ASSERT("invalid mbox", (sys_mbox != NULL) && (sys_mbox->mbox != NULL));

    // TODO Patch
    if(sys_mbox == NULL || sys_mbox->mbox == NULL)
        return SYS_MBOX_EMPTY;

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
        sem_up(mbox->not_full);
    }

    mutex_unlock(mbox->mutex);

    return 0;
}

void sys_mbox_free(sys_mbox_t *sys_mbox)
{

    if ((sys_mbox != NULL) && (sys_mbox->mbox != NULL)) {
        _mbox_t *mbox = sys_mbox->mbox;
        SYS_STATS_DEC(mbox.used);

        free(mbox->not_empty);
        free(mbox->not_full);
        free(mbox->mutex);
        mbox->not_empty = mbox->not_full = mbox->mutex = NULL;
        free(mbox);
    }

}

struct _thread_function_adapter_data  {
    void* arg;
    lwip_thread_fn function;
};
typedef struct _thread_function_adapter_data _thread_function_adapter_data_t;


void *_thread_function_adapter(void *arg){
    _thread_function_adapter_data_t *adapter_data = (_thread_function_adapter_data_t*)arg;

    adapter_data->function(adapter_data->arg);
    free(adapter_data);

    return NULL;
}

sys_thread_t sys_thread_new(const char *name, lwip_thread_fn function, void *arg, int stacksize, int prio)
{
    sys_thread_t sys_thread;
    _thread_function_adapter_data_t *adapter_data = malloc(sizeof(_thread_function_adapter_data_t));

    LWIP_UNUSED_ARG(stacksize);

    adapter_data->arg = arg;
    adapter_data->function = function;

    sys_thread.thread_handle = kernel_thread(_thread_function_adapter, name, adapter_data, prio);

    return sys_thread;
}
