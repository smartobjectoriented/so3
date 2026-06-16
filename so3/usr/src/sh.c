/*
 * Copyright (C) 2014-2026 Daniel Rossier <daniel.rossier@heig-vd.ch>
 * Copyright (C) 2017-2018 Xavier Ruppen <xavier.ruppen@heig-vd.ch>
 * Copyright (C) 2017 Alexandre Malki <alexandre.malki@heig-vd.ch>
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
 * so3 shell: read a line, tokenize it, then either run a builtin or fork+exec
 * an external program (looked up as "<name>.elf"). Supports a trailing '&' for
 * background, a single '|' pipe and a single '>' redirection.
 */

#include <sys/types.h>
#include <sys/wait.h>

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>

#define MAX_LINE 256 /* longest command line accepted              */
#define MAX_TOKENS 64 /* most whitespace-separated tokens per line   */
#define MAX_ARGS 64 /* most arguments passed to one program        */
#define MAX_PATH 128 /* longest "<name>.elf" path                  */

#define ELF_SUFFIX ".elf"

extern char **environ; /* current environment (POSIX) */

static const char prompt[] = "so3% ";

/* Set by the SIGINT handler so the main loop can react outside signal ctx. */
static volatile sig_atomic_t got_sigint;

/*
 * SIGINT (Ctrl-C): the shell itself must not die. Only note the event with an
 * async-signal-safe write; the main loop reprints the prompt.
 */
static void sigint_handler(int sig)
{
	(void) sig;
	got_sigint = 1;
	write(STDOUT_FILENO, "\n", 1);
}

/* Reap any finished background children without blocking. */
static void reap_children(void)
{
	while (waitpid(-1, NULL, WNOHANG) > 0)
		;
}

/*
 * Strip CSI escape sequences (ESC '[' ... final-byte) in place, so e.g. arrow
 * keys do not end up inside the command. Bounded: never reads past the NUL.
 */
static void strip_escapes(char *s)
{
	char *r = s, *w = s;

	while (*r) {
		if (r[0] == '\x1b' && r[1] == '[') {
			r += 2;
			while (*r && (*r < '@' || *r > '~'))
				r++;
			if (*r)
				r++; /* skip the final byte */
		} else
			*w++ = *r++;
	}
	*w = '\0';
}

/*
 * Read one line from stdin into buf (NUL-terminated, newline removed).
 * Returns the line length, or -1 on EOF / interrupted read.
 */
static int read_line(char *buf, int size)
{
	size_t len;

	if (fgets(buf, size, stdin) == NULL) {
		clearerr(stdin); /* clear EINTR/EOF state for the next read */
		return -1;
	}

	strip_escapes(buf);

	len = strlen(buf);
	if (len > 0 && buf[len - 1] == '\n')
		buf[--len] = '\0';

	return (int) len;
}

/* Split line on spaces/tabs into tok[] (pointers into line). Returns count. */
static int tokenize(char *line, char *tok[], int max)
{
	int n = 0;
	char *p = strtok(line, " \t");

	while (p != NULL && n < max) {
		tok[n++] = p;
		p = strtok(NULL, " \t");
	}

	return n;
}

/* -------------------------------------------------------------------------- */
/* Builtins                                                                   */
/* -------------------------------------------------------------------------- */

static int builtin_exit(int argc, char *argv[])
{
	(void) argc;
	(void) argv;

	/* The root shell (pid 1) cannot exit: nothing would reap it. */
	if (getpid() == 1) {
		printf("The shell root process can not be terminated...\n");
		return 0;
	}

	exit(0);
}

static int builtin_env(int argc, char *argv[])
{
	int i;

	(void) argc;
	(void) argv;

	for (i = 0; environ[i] != NULL; i++)
		printf("%s\n", environ[i]);

	return 0;
}

static int builtin_setenv(int argc, char *argv[])
{
	if (argc == 3)
		setenv(argv[1], argv[2], 1); /* always overwrite */
	else if (argc == 2)
		unsetenv(argv[1]);
	else
		printf("usage: setenv NAME [VALUE]\n");

	return 0;
}

/* Map a "kill" signal flag ("-9", "-USR1", ...) to a signal number, or -1. */
static int parse_signal(const char *flag)
{
	if (!strcmp(flag, "-9") || !strcmp(flag, "-KILL"))
		return SIGKILL;
	if (!strcmp(flag, "-15") || !strcmp(flag, "-TERM"))
		return SIGTERM;
	if (!strcmp(flag, "-USR1"))
		return SIGUSR1;
	if (!strcmp(flag, "-USR2"))
		return SIGUSR2;

	return -1;
}

static int builtin_kill(int argc, char *argv[])
{
	int sig, pid;

	if (argc == 2) {
		sig = SIGTERM;
		pid = atoi(argv[1]);
	} else if (argc == 3) {
		sig = parse_signal(argv[1]);
		pid = atoi(argv[2]);
		if (sig < 0) {
			printf("kill: unknown signal '%s'\n", argv[1]);
			return 0;
		}
	} else {
		printf("usage: kill [-9|-15|-USR1|-USR2] PID\n");
		return 0;
	}

	if (pid <= 0) {
		printf("kill: invalid pid\n");
		return 0;
	}

	if (kill(pid, sig) < 0)
		printf("kill: failed to signal pid %d\n", pid);

	return 0;
}

struct builtin {
	const char *name;
	int (*fn)(int argc, char *argv[]);
};

static const struct builtin builtins[] = {
	{ "exit", builtin_exit }, { "env", builtin_env }, { "setenv", builtin_setenv },
	{ "kill", builtin_kill }, { NULL, NULL },
};

/* Run argv[0] as a builtin if it is one. Returns 1 if handled, else 0. */
static int run_builtin(int argc, char *argv[])
{
	const struct builtin *b;

	for (b = builtins; b->name != NULL; b++) {
		if (!strcmp(argv[0], b->name)) {
			b->fn(argc, argv);
			return 1;
		}
	}

	return 0;
}

/* -------------------------------------------------------------------------- */
/* External commands                                                          */
/* -------------------------------------------------------------------------- */

/*
 * Replace the current (child) process image with "<argv[0]>.elf". Never
 * returns on success; on failure it prints an error and exits the child.
 */
static void exec_elf(char *argv[])
{
	char path[MAX_PATH];

	snprintf(path, sizeof(path), "%s%s", argv[0], ELF_SUFFIX);

	execv(path, argv);

	printf("%s: exec failed.\n", argv[0]);
	exit(127);
}

/* left | right : connect left's stdout to right's stdin. */
static void run_pipe(char *left[], char *right[])
{
	int fds[2];
	pid_t pl, pr;

	if (pipe(fds) < 0) {
		printf("sh: pipe failed.\n");
		return;
	}

	pl = fork();
	if (pl == 0) {
		dup2(fds[1], STDOUT_FILENO);
		close(fds[0]);
		close(fds[1]);
		exec_elf(left);
	}

	pr = fork();
	if (pr == 0) {
		dup2(fds[0], STDIN_FILENO);
		close(fds[0]);
		close(fds[1]);
		exec_elf(right);
	}

	close(fds[0]);
	close(fds[1]);

	waitpid(pl, NULL, 0);
	waitpid(pr, NULL, 0);
}

/* cmd > file : send cmd's stdout to file (created/truncated). */
static void run_redirect(char *argv[], const char *outfile)
{
	pid_t pid = fork();

	if (pid == 0) {
		int fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (fd < 0) {
			printf("sh: cannot open '%s'.\n", outfile);
			exit(1);
		}
		dup2(fd, STDOUT_FILENO);
		close(fd);
		exec_elf(argv);
	}

	waitpid(pid, NULL, 0);
}

/* Plain command, optionally in background. */
static void run_simple(char *argv[], int background)
{
	pid_t pid = fork();

	if (pid < 0) {
		printf("sh: fork failed.\n");
		return;
	}

	if (pid == 0)
		exec_elf(argv);

	if (background)
		printf("[%d]\n", pid);
	else
		waitpid(pid, NULL, 0);
}

/*
 * Split the token list around the '&', '|' and '>' operators and dispatch to
 * the right runner. Pipe and redirection are mutually exclusive here.
 */
static void run_external(int n, char *tok[])
{
	char *argv[MAX_ARGS], *rhs[MAX_ARGS];
	char *outfile = NULL;
	int background = 0, has_pipe = 0, has_redir = 0;
	int ai = 0, ri = 0, i;

	for (i = 0; i < n; i++) {
		if (!strcmp(tok[i], "&")) {
			background = 1;
			break; /* anything after '&' is ignored */
		} else if (!strcmp(tok[i], "|")) {
			has_pipe = 1;
		} else if (!strcmp(tok[i], ">")) {
			has_redir = 1;
		} else if (has_redir) {
			outfile = tok[i];
		} else if (has_pipe) {
			if (ri < MAX_ARGS - 1)
				rhs[ri++] = tok[i];
		} else {
			if (ai < MAX_ARGS - 1)
				argv[ai++] = tok[i];
		}
	}

	argv[ai] = NULL;
	rhs[ri] = NULL;

	if (ai == 0) {
		printf("sh: syntax error.\n");
		return;
	}

	if (has_pipe) {
		if (ri == 0) {
			printf("sh: expected command after '|'.\n");
			return;
		}
		run_pipe(argv, rhs);
	} else if (has_redir) {
		if (outfile == NULL) {
			printf("sh: expected file after '>'.\n");
			return;
		}
		run_redirect(argv, outfile);
	} else
		run_simple(argv, background);
}

/* -------------------------------------------------------------------------- */
/* Main loop                                                                  */
/* -------------------------------------------------------------------------- */

int main(int argc, char *argv[])
{
	char line[MAX_LINE];
	char *tok[MAX_TOKENS];
	struct sigaction sa;
	int n;

	(void) argc;
	(void) argv;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sigint_handler;
	sigaction(SIGINT, &sa, NULL);

	while (1) {
		reap_children();

		got_sigint = 0;
		printf("%s", prompt);
		fflush(stdout);

		n = read_line(line, sizeof(line));
		if (n < 0) /* EOF or interrupted (e.g. Ctrl-C): just reprompt */
			continue;

		n = tokenize(line, tok, MAX_TOKENS);
		if (n == 0)
			continue;

		if (!run_builtin(n, tok))
			run_external(n, tok);
	}

	return 0;
}
