/*
 * Copyright (C) 2014-2017 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifndef SYSCALL_H
#define SYSCALL_H

#ifndef __ASSEMBLY__
#include <sys/ptrace.h>
#include <sys/stat.h>
#endif

/* System call codes, passed in r0 to tell the kernel which system call to do. */

#define syscallHalt			0
#define syscallExit			1
#define syscallExecve			2
#define syscallWaitpid			3
#define syscallRead			4
#define syscallWrite			5
#define syscallPause			6
#define syscallFork	 		7
#define syscallPtrace			8
#define syscallReaddir			9
#define syscallChdir			10
#define syscallGetcwd			11
#define syscallCreate			12
#define syscallUnlink			13
#define syscallOpen			14
#define syscallClose			15
#define syscallThreadCreate		16
#define syscallThreadJoin		17
#define syscallThreadExit		18
#define syscallPipe			19
#define syscallIoctl			20
#define syscallFcntl			21
#define syscallDup			22
#define syscallDup2			23
#define syscallSchedSetParam 		25
#define syscallSocket 			26
#define syscallBind			27
#define syscallListen			28
#define syscallAccept 			29
#define syscallConnect			30
#define syscallRecv			31
#define syscallSend			32
#define syscallSendTo			33
#define syscallStat			34
#define syscallMmap			35
#define syscallEndProc			36

/* getPID syscall */
#define syscallGetpid			37

/* time management */
#define syscallGetTimeOfDay		38
#define syscallSetTimeOfDay		39
#define syscallClockGetTime		40

#define syscallThreadYield		43

#define syscallSbrk			45

#define syscallSigaction    		46
#define syscallKill         		47
#define syscallSigreturn         	48

#define syscallLseek			50

#define syscallMutexLock		60
#define syscallMutexUnlock		61

#define syscallPs                       63

#define syscallNanosleep		70

#define syscallSysinfo			99

#define syscallSetsockopt		110
#define syscallRecvfrom			111


#define SYSINFO_DUMP_HEAP	0
#define SYSINFO_DUMP_SCHED	1
#define SYSINFO_TEST_MALLOC	2
#define SYSINFO_PRINTK	 	3

#ifndef __ASSEMBLY__

#include <bits/alltypes.h>

#include <netinet/in.h>
#include <pthread.h>
#include <types.h>
#include <inet.h>
#include <signal.h>

extern int errno;

/* The system call interface. These are the operations the SO3 kernel needs to
 * support, to be able to run user programs.
 *
 * Each of these is invoked by a user program by simply calling the
 * procedure; an assembly language stub stores the syscall code (see above) into $r0
 * and executes a syscall instruction. The kernel exception handler is then invoked.
 */

void sys_halt();

/* PROCESS MANAGEMENT SYSCALLS: exit(), exec(), fork(), waitpid() */

/**
 * Terminate the current process immediately. Any open file descriptors belonging to the
 * process are closed. Any children of the process no longer have a parent process.
 *
 * status is returned to the parent process as this process's exit status and can be
 * collected using the join syscall. A process exiting normally should set status to 0.
 *
 * exit() never returns.
 */
void sys_exit(int status) __attribute__((noreturn));

/**
 * Execute the program stored in the specified file, with the specified arguments.
 * The current process image is replaced by the program found in the file.
 *
 * file is a null-terminated string that specifies the name of the file containing the
 * executable. Note that this string must include the ".elf" extension.
 *
 * argc specifies the number of arguments to pass to the child process. This number must
 * be non-negative.
 *
 * argv is an array of pointers to null-terminated strings that represent the arguments
 * to pass to the child process. argv[0] points to the first argument, and argv[argc-1]
 * points to the last argument.
 *
 * exec() returns the child process's process ID, which can be passed to join(). On
 * error, returns -1.
 */

int sys_execve(const char *path, char *const argv[], char *const envp[]);

/**
 * This system call are used to wait for state changes in a child of the calling
 * process, and obtain information about the child whose state has changed.
 * A state change is considered to be: the child terminated; the child was stopped.
 *
 * By default, waitpid() waits only for terminated children, but this behavior
 * is modifiable via the options argument, as described above.
 *
 * If status is not NULL, waitpid() store status information in the int to which it
 * points. This integer can be inspected with the macros above (which take the
 * integer itself as an argument, not a pointer to it, as is done in waitpid()!)
 *
 * on success, returns the process ID of the child whose state has changed;
 * if WNOHANG was specified and one or more child(ren) specified by pid exist,
 * but have not yet changed state, then 0 is returned. On error, -1 is returned.
 */
pid_t sys_waitpid(pid_t pid, int *status, int options);

/* FILE MANAGEMENT SYSCALLS: creat, open, read, write, close, unlink
 *
 * A file descriptor is a small, non-negative integer that refers to a file on disk
 * or to a stream (such as console input, console output, and network connections).
 * A file descriptor can be passed to read() and write() to read/write the corresponding
 * file/stream. A file descriptor can also be passed to close() to release the file
 * descriptor and any associated resources.
 */

/**
 * Suspend the execution during <delay> millisecond(s).
 */
void sys_pause(int delay);

/**
 * Suspend the execution during <req.tv_usec> microseconds.
 * <rem> is not used at the moment.
 */
int sys_nanosleep(const struct timespec *req, struct timespec *rem);

/**
 * Fork a new process according to the standard UNIX fork system call.
 */
int sys_fork(void);

/**
 * creat() is equivalent to open() with flags equal to O_CREAT|O_WRONLY|O_TRUNC
 */
int sys_creat(const char *pathname, mode_t mode);

/**
 * Delete a file from the file system. If no processes have the file open, the file is
 * deleted immediately and the space it was using is made available for reuse.
 *
 * If any processes still have the file open, the file will remain in existence until the
 * last file descriptor referring to it is closed. However, creat() and open() will not
 * be able to return new file descriptors for the file until it is deleted.
 *
 * Returns 0 on success, or -1 if an error occurred.
 */
int sys_unlink(char *name);

/**
 * open - open and possibly create a file or device
 *
 * Given a pathname for a file, open() returns a file descriptor, a small, nonnegative
 * integer for use in subsequent system calls (read(2), write(2), seek(2), etc.).
 * The file descriptor returned by a successful call will be the lowest-numbered file
 * descriptor not currently open for the process.
 *
 * open() return the new file descriptor, or -1 if an error occurred
 * (in which case, errno is set appropriately).
 */
int sys_open(const char *pathname, int flags, int mode);

int sys_write(int fd, const void *buf, size_t count);
int sys_read(int fd, void *buf, size_t count);

int sys_readdir(int fd, char *buf, int len);

/**
 * Close a file descriptor, so that it no longer refers to any file or stream and may be
 * reused.
 *
 * If the file descriptor refers to a file, all data written to it by write() will be
 * flushed to disk before close() returns.
 * If the file descriptor refers to a stream, all data written to it by write() will
 * eventually be flushed (unless the stream is terminated remotely), but not
 * necessarily before close() returns.
 *
 * The resources associated with the file descriptor are released. If the descriptor is
 * the last reference to a disk file which has been removed using unlink, the file is
 * deleted (this detail is handled by the file system implementation).
 *
 * Returns 0 on success, or -1 if an error occurred.
 */
int sys_close(int fd);

/**
 * The ioctl() function manipulates the underlying device parameters of
 * special files. In particular, many operating characteristics of character
 * special files (e.g., terminals) may be controlled with ioctl()
 * requests. The argument fd must be an open file descriptor.
 *
 * The second argument is a device-dependent request code. The third
 * argument is an untyped pointer to memory.
 *
 * Returns 0 on success or -1 if an error occurred, and errno is set.
 */
int sys_ioctl(int fd, int cmd, void *val);

/**
 * fcntl() performs one of the operations described below on the open file descriptor fd.  The operation is determined by cmd.
 * fcntl() can take an optional third argument.  Whether or not this argument is required is determined by cmd.
 * The required argument type is indicated in parentheses after each cmd name (in most cases, the
 * required type is int, and we identify the argument using the name arg), or void is specified if the argument is not required.
 *
 * Certain of the operations below are supported only since a particular Linux kernel version.
 * The preferred method of checking whether the host kernel supports a particular operation  is  to  invoke  fcntl()
 * with the desired cmd value and then test whether the call failed with EINVAL, indicating that the kernel does not recognize this value.
 *
 * See man page for returned value.
 */
int sys_fcntl(int fd, int cmd, void *val);

/*
 *  lseek() repositions the file offset of the open file description associated with the file descriptor fd to the argument offset
 *  according to the directive whence as follows:
 *
 *    SEEK_SET
 *            The file offset is set to offset bytes.
 *
 *    SEEK_CUR
 *            The file offset is set to its current location plus offset bytes.
 *
 *    SEEK_END
 *            The file offset is set to the size of the file plus offset bytes.
 *
 *     lseek()  allows  the  file  offset  to  be set beyond the end of the file (but this does not change the size of the file).
 *     If data is later written at this point, subsequent reads of the data in the gap (a "hole") return null bytes ('\0') until data is actually written into the gap.
 *
 */

off_t sys_lseek(int fd, off_t offset, int whence);

/**
 * This IOCTL command returns the number of column and lines of the given
 * fd. the result is returned in the following form:
 * int val[0] = #lines
 * int val[1] = #columns
 */
#define IOR_CONSOLE_SIZE 1

/**
 * This IOCTL command returns the machine link address
 * int *val = pointer to machine link address
 */
#define IOR_NETWORK_ADDRESS 2

/**
 * Clone a thread.
 *
 * Returns the thread ID on success to the parent and 0 to the child, or
 * -1 on error and set errno.

 * entryState=true: first invocation by the parent (routine and args has to be passed)
 * entryState=false: second invocation by the child (which retrieved routine and args from the kernel)
 */
int sys_thread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void *), void *arg);

/**
 * The thread_join() function waits for the thread specified by thread to
 * terminate. If that thread has already terminated, then thread_join()
 * returns immediately. The thread specified by thread must be joinable.
 *
 * If retval is not NULL, then thread_join() copies the exit status of
 * the target thread (i.e., the value that the target thread supplied to
 * thread_join(3)) into the location pointed to by *retval.
 *
 * On success, thread_join() returns 0; on error, it returns an error
 * number.
 */
int sys_thread_join(pthread_t thread, void **value_ptr);

/**
 * The calling thread is terminated. This function does not return.
 */
void sys_thread_exit(int *exit_status);

/*
 * Yield to another thread.
 */
int sys_thread_yield(void);

/**
 * Open a pipe and returns the two file descriptors in the array fd.
 *
 * Returns 0 on success, or -1 on error and set errno.
 */
int sys_pipe(int fd[]);

/**
 * Duplicate an open file descriptor.
 *
 * Example: 
 * 	dup2(1, 2); // Redirects messages from stderr to stdout
 *
 * Returns -1 on error, or fildes2 on success.
 */
int sys_dup(int oldfd);


/**
 * Duplicate an open file descriptor.
 *
 * Example: 
 * 	dup2(1, 2); // Redirects messages from stderr to stdout
 *
 * Returns -1 on error, or fildes2 on success.
 */
int sys_dup2(int newfd, int oldfd);

/**
 * Change priority of given thread
 *
 * Returns 0 on success, or -1 on error and set errno.
 */
int sys_sched_setparam(int threadId, int priority);

/**
 * Creates an endpoint for network communication (socket).
 * The domain argument specifies a communication domain. Starts with AF_
 * The socket has the indicated type, which specifies the communication semantics.
 * The protocol specifies a particular protocol to be used with the socket.
 *
 * Return a file descriptor on success, or -1 and set errno.
 */
int sys_socket(int domain, int type, int protocol);

/**
 * Assigns a port to a specific socket
 *
 * Returns 0 on success, or -1 on error and set errno.
 */
int sys_bind(int socket, const struct sockaddr *addr, int port);

/**
 * Marks the socket as passive and ready to accept incoming
 * connection requests using accept(). the argument backlog represent
 * the maximum number of pending requests.
 *
 * Returns 0 on success, or -1 on error and set errno.
 */
int sys_listen(int socket, int backlog);

/**
 * Attempt to accept a single connection on the specified local port and return
 * a new file descriptor referring to the connection. Block until new requests.
 * cli_addr and cli_port are fill up with remote client informations.
 *
 * Returns a new file descriptor on success, or -1 on error and set errno.
 */
int sys_accept(int socket, struct sockaddr *cli_addr, socklen_t *cli_port);

/**
 * Attempt to initiate a new connection (client side) to the specified port
 * on the specific remote host. Block until the remote host response.
 *
 * Example : connect(s, 3, 1111);
 *
 * Returns 0 on success, or -1 on error and set errno.
 */
int sys_connect(int socket, const struct sockaddr *si, socklen_t addrlen);

/**
 * This system call is used to receive messages from a socket. If no messages are
 * available on the socket, the receive calls wait for a message to arrive.
 *
 * Returns the number of byte read. On error, -1 is returned, this can append if
 * a network stream has been terminated by the remote host and no more data is available.
 * See errno variable to have more information on the error.
 */
int sys_recv(int socket_fd, void *buffer, int count, int flags);

/**
 * This system call is used to receive messages from a socket, and may be
 * used to receive data on a socket whether or not it is connection-oriented.
 * If no messages are available on the socket, the receive calls wait for a
 * message to arrive.
 *
 * Returns the number of byte read. On error, -1 is returned, this can append if
 * a network stream has been terminated by the remote host and no more data is available.
 * See errno variable to have more information on the error.
 */
int sys_recvfrom(int sockfd, void *mem, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen);

/**
 * This system call is used to transmit a messages to another socket.
 * This call may be used only when the socket is in a connected state.
 *
 * Returns the number of byte written. On error, -1 is returned,
 * and errno is set appropriately.
 */
int sys_send(int socket_fd, void *buffer, int count, int flags);

/**
 * This system call is used to transmit a messages to another socket.
 * This call does not need any connected state.
 *
 * Returns the number of byte written. On error, -1 is returned,
 * and errno is set appropriately.
 */
int sys_sendto(int fd, const void *buf, size_t len, int flags, const struct sockaddr *addr, socklen_t alen);

/* 
 * This system call returns information about a file in the buffer
 * pointed by <statbuf>.
 */
int sys_stat(const char *pathname, struct stat *statbuf);


int sys_setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen);


/**
 * This system call is used to map a file (or a portion of it) to a memory buffer
 * in virtual memory. You have to open a file prior this call.
 *
 * start: start of the virtual memory somewhere in the heap area of the process
 * length: represents how many bytes you want to map
 * prot: is the mode of accessing mapped memory (READ, WRITE, READ/WRITE)
 * fd: is the file descriptor of the opened file
 * offset: is where to start mapping in the file
 */
void *sys_mmap(uint32_t start, size_t length, int prot, int fd, off_t offset);

/**
 * The ptrace() system call provides a means by which one process (the "tracer")
 * may observe and control the execution of another process (the "tracee"), and
 * examine and change the tracee's memory and registers. It is primarily used
 * to implement breakpoint debugging and system call tracing.
 *
 * Return 0 on success, except for PTRACE_PEEK* requests which return the requested data.
 * On error -1 is returned and errno is set appropriately.
 */
int sys_ptrace(enum __ptrace_request request, pid_t pid, void *addr, void *data);

/**
 * The ioctl() function manipulates the underlying device parameters of
 * special files. In particular, many operating characteristics of character
 * special files (e.g., terminals) may be controlled with ioctl()
 * requests. The argument fd must be an open file descriptor.
 *
 * The second argument is a device-dependent request code. The third
 * argument is an untyped pointer to memory.
 *
 * Returns 0 on success or -1 if an error occurred, and errno is set.
 */
int sys_ioctl(int fd, int cmd, void *val);


/**
 * getPID syscall
 */
int sys_getpid();

/**
* Time management - time zone is not supported yet.
*/
int sys_gettimeofday(struct timespec *ts, void *tz);
int sys_settimeofday(struct timespec *ts, void *tz);

int sys_clock_gettime(clockid_t clk, struct timespec *ts);

/*
 * sbrk syscall
 *
 * Grows heap
 *
 * increment: number of bytes to add to heap. If increment is not a multiple of PAGE_SIZE,
 * 	it is internally incremented to the next whole page size. (1025 -> 2048 for example)
 * return: Base address of newly allocated space. If there is not enough space left in
 * 	the heap, return (void *) -1 and sets ERRNO to ENOMEM. If increment equals 0, return current heap top.
 */

void *sys_sbrk(int increment);

/*
 * First attempt of mutex implementation in the user space. Mainly used, at the moment,
 * for debugging kernel mutexes.
 */

void sys_mutex_lock(int lock_idx);
void sys_mutex_unlock(int lock_idx);


/*
 * sigaction syscall.
 * Used to change the action taken by a process on receipt of a specific signal.
 *
 * signum: Signal number to handle
 * action: Handler for the signal.
 * old_action: If non-NULL, the previous action is saved in old_action
 */
int sys_sigaction(int signum,  void *sa, void *old);

/*
 * kill syscall.
 * Used to send a signal to a process defined by its PID
 *
 * pid: Process PID.
 * sig: Signal to send.
 */
int sys_kill(pid_t pid, int sig);

/*
 * Syscall used to restore the registers after a signal handler was called
 *
 */
void sys_sigreturn(void);

/*
 * Get system information
 * - @type = 0 : dump heap memory
 */
void sys_info(int type, int val);

#endif /* __ASSEMBLY__ */

#endif /* SYSCALL_H */
