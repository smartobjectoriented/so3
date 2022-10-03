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
#include <ptrace.h>
#include <mutex.h>

#define PROC_MAX 64
#define PROC_THREAD_MAX 32

/* Maximum stack size for a process, including all thread stacks */
#define PROC_STACK_SIZE (PROC_THREAD_MAX * THREAD_STACK_SIZE)

#define FD_MAX 		64
#define N_MUTEX		10

typedef enum { PROC_STATE_NEW, PROC_STATE_READY, PROC_STATE_RUNNING, PROC_STATE_WAITING, PROC_STATE_ZOMBIE } proc_state_t;
typedef unsigned int thread_t;

#define PROC_NAME_LEN 80

/* A page might be linked to several processes, hence this type */
typedef struct {
	struct list_head list;
	page_t *page;
} page_list_t;

typedef struct {
	bool tracee;
	enum __ptrace_request req_in_progress;
} ptrace_info_t;

struct pcb {
	int pid;
	char name[PROC_NAME_LEN];

	/* Initial entry point used to configure the PC */
	addr_t bin_image_entry;

	/* Full descending stack - refers to a "full" word */
	addr_t stack_top;

	/* Thread stack slots */
	bool stack_slotID[PROC_THREAD_MAX];

	/* Heap management */
	addr_t heap_base;

	/* current position of the heap pointer */
	addr_t heap_pointer;

	/* Number of pages required by this process (including binary image) */
	size_t page_count;

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

	/* The process might be under a ptrace activity, and hence becoming a tracer (parent) or tracee (child) */
	enum __ptrace_request ptrace_pending_req;

	/* Mutex lock to be used in conjunction with the user space (very temporary) */
	mutex_t lock[N_MUTEX];

};
typedef struct pcb pcb_t;

extern struct list_head proc_list;

int get_user_stack_slot(pcb_t *pcb);
void free_user_stack_slot(pcb_t *pcb, int slotID);

void add_page_to_proc(pcb_t *pcb, page_t *page);

void create_root_process(void);

uint32_t do_getpid(void);

int do_execve(const char *filename, char **argv, char **envp);
int do_fork(void);
void do_exit(int exit_status);
int do_waitpid(int pid, uint32_t *wstatus, uint32_t options);

pcb_t *find_proc_by_pid(uint32_t pid);

int proc_register_fd(int fd);
int proc_new_fd(pcb_t *pcb);
void dump_proc(void);

extern int __exec(const char *file);
extern int __write(int fd, char *buffer, int count);

int do_sbrk(int increment);

#endif /* PROCESS_H */
