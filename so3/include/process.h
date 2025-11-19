/*
 * Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
 * Copyright (C) 2017-2018 Xavier Ruppen <xavier.ruppen@heig-vd.ch>
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

#ifndef PROCESS_H
#define PROCESS_H

#include <types.h>
#include <list.h>
#include <thread.h>
#include <schedule.h>
#include <elf-struct.h>
#include <completion.h>
#include <memory.h>
#include <signal.h>
#include <spinlock.h>
#include <futex.h>
#include <mutex.h>
#include <syscall.h>

#define PROC_MAX 64
#define PROC_MAX_THREADS CONFIG_MAX_THREADS

/* Maximum stack size for a process, including all thread stacks */
#define INITIAL_STACK_SIZE (CONFIG_THREAD_STACK_SIZE_KB * SZ_1K)

#define FD_MAX 64
#define N_MUTEX 5

typedef enum { PROC_STATE_NEW, PROC_STATE_READY, PROC_STATE_RUNNING, PROC_STATE_WAITING, PROC_STATE_ZOMBIE } proc_state_t;
typedef unsigned int thread_t;

#define PROC_NAME_LEN 80

/* Clone flags */
#define CSIGNAL 0x000000ff /* signal mask to be sent at exit */
#define CLONE_VM 0x00000100 /* set if VM shared between processes */
#define CLONE_FS 0x00000200 /* set if fs info shared between processes */
#define CLONE_FILES 0x00000400 /* set if open files shared between processes */
#define CLONE_SIGHAND 0x00000800 /* set if signal handlers and blocked signals shared */
#define CLONE_PIDFD 0x00001000 /* set if a pidfd should be placed in parent */
#define CLONE_PTRACE 0x00002000 /* set if we want to let tracing continue on the child too */
#define CLONE_VFORK 0x00004000 /* set if the parent wants the child to wake it up on mm_release */
#define CLONE_PARENT 0x00008000 /* set if we want to have the same parent as the cloner */
#define CLONE_THREAD 0x00010000 /* Same thread group? */
#define CLONE_NEWNS 0x00020000 /* New mount namespace group */
#define CLONE_SYSVSEM 0x00040000 /* share system V SEM_UNDO semantics */
#define CLONE_SETTLS 0x00080000 /* create a new TLS for the child */
#define CLONE_PARENT_SETTID 0x00100000 /* set the TID in the parent */
#define CLONE_CHILD_CLEARTID 0x00200000 /* clear the TID in the child */
#define CLONE_DETACHED 0x00400000 /* Unused, ignored */
#define CLONE_UNTRACED 0x00800000 /* set if the tracing process can't force CLONE_PTRACE on this clone */
#define CLONE_CHILD_SETTID 0x01000000 /* set the TID in the child */
#define CLONE_NEWCGROUP 0x02000000 /* New cgroup namespace */
#define CLONE_NEWUTS 0x04000000 /* New utsname namespace */
#define CLONE_NEWIPC 0x08000000 /* New ipc namespace */
#define CLONE_NEWUSER 0x10000000 /* New user namespace */
#define CLONE_NEWPID 0x20000000 /* New pid namespace */
#define CLONE_NEWNET 0x40000000 /* New network namespace */
#define CLONE_IO 0x80000000 /* Clone io context */

/* A page might be linked to several processes, hence this type */
typedef struct {
	struct list_head list;
	page_t *page;
} page_list_t;

struct pcb {
	int pid;
	char name[PROC_NAME_LEN];

	/* Initial entry point used to configure the PC */
	addr_t bin_image_entry;

	/* Full descending stack - refers to a "full" word */
	addr_t stack_top;

	/* Heap management */
	addr_t heap_base;

	/* current position of the heap pointer */
	addr_t heap_pointer;

	/* next anonymous start pointer */
	addr_t next_anon_start;

	/* Number of pages required by this process (including binary image) */
	size_t page_count;

	/* List of futexes */
	struct list_head futex;
	spinlock_t futex_lock;

	/* List of frames (physical pages) belonging to this process */
	struct list_head page_list;

	/* Process 1st-level page table */
	void *pgtable;

	uint32_t exit_status;

	/* Reference to the parent process */
	pcb_t *parent;

	/* Containing thread */
	tcb_t *main_thread;

	/* List of threads of this process, except the main thread which has a specific field <main_thread> */
	struct list_head threads;

	/* Process state */
	int state;

	/* Local file descriptors belonging to the process */
	int fd_array[FD_MAX];

	/* Used to integrate a global system list of all existing process (declared in schedule.c) */
	struct list_head list;

	/* Manage a completion until all running threads complete (used with pthread_exit()) */
	completion_t threads_active;

	/* List of signals handlers for this process */
	sigaction_t sa[_NSIG];

	/* Helper for low level sigaction access */
	__sigaction_t __sa[_NSIG];

	/* Bitmap of the signals set for this process */
	sigset_t sigset_map;

	/* Mutex lock to be used in conjunction with the user space (very temporary) */
	mutex_t *lock;
};
typedef struct pcb pcb_t;

extern struct list_head proc_list;

void add_page_to_proc(pcb_t *pcb, page_t *page);

void create_root_process(void);

SYSCALL_DECLARE(getpid, void);

SYSCALL_DECLARE(execve, const char *filename, char **argv, char **envp);
SYSCALL_DECLARE(fork, void);
SYSCALL_DECLARE(clone, unsigned long flags, unsigned long newsp, int *parent_tid, unsigned long tls, int *child_tid);
SYSCALL_DECLARE(exit_group, int exit_status);
SYSCALL_DECLARE(wait4, int pid, uint32_t *wstatus, uint32_t options, void *rusage);

pcb_t *find_proc_by_pid(uint32_t pid);

int proc_register_fd(int fd);
int proc_new_fd(pcb_t *pcb);
void dump_proc(void);

extern int __exec(const char *file);
extern int __write(int fd, char *buffer, int count);

SYSCALL_DECLARE(brk, void *addr);

#endif /* PROCESS_H */
