/*
 * Copyright (C) 2014-2018 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/wait.h>

/*
 * This entry code handles the initialization of the BSS section.
 *
 */
extern char __bss_start[], __bss_end[];
extern int main(int argc, char **argv);
extern void mutex_init(void);

void finish_child(int signum) {
	int saved_errno;
	int pid;

	saved_errno = errno;
	while ((pid = waitpid((pid_t)(-1), NULL, WNOHANG)) > 0)
		printf("[%d] Terminated\n", pid);

	errno = saved_errno;
}

/*
 * Handling the ctrl/C key (SIGINT)
 */
void term_default(int signum) {
	if (signum == SIGTERM)
		printf("[%d] Terminated\n", getpid());

	/* Force the process to quit */
	exit(0);
}

__attribute__((__section__(".head"))) int __entryC(void *args) {
	struct sigaction sa;

	memset(&sa, 0, sizeof(struct sigaction));

	char *cp = __bss_start;
	int argc;
	char **argv;

	/* Zero out BSS */
	while (cp < __bss_end)
		*cp++ = 0;

	/*
	 * Prepare argc & argv
	 * The arguments are placed in a argument page on top of the user space.
	 * The first four bytes are used to store argc, and the array of string pointers
	 * follow right after. The strings themselves are placed after the array of pointers.
	 */
	argc = *((int *) args);

	/* Just give the beginning of the array of pointers */
	argv = (char **) (args + 4);

#ifdef __ARM__
	__environ  = (char **) (args + 4 + 4*argc);
#else
	__environ  = (char **) (args + 4 + 8*argc);
#endif

	sa.sa_handler = term_default;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGKILL, &sa, NULL);

	sa.sa_handler = finish_child;
	sigaction(SIGCHLD, &sa, NULL);

	return main(argc, argv);

}


