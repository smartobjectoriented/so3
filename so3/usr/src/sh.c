/*
 * Copyright (C) 2014-2026 REDS Institute from HEIG-VD <daniel.rossier@heig-vd.ch>
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
 * so3 shell: read a line, tokenize it (with quoting and $VAR expansion), then
 * either run a builtin or fork+exec a pipeline of external programs (each
 * looked up as "<name>.elf").
 *
 * Supported syntax (operators must be surrounded by whitespace):
 *   cmd a b              run cmd with arguments
 *   cmd1 | cmd2 | cmd3   pipeline (any number of stages)
 *   cmd > file           stdout to file (truncate)
 *   cmd >> file          stdout to file (append)
 *   cmd < file           stdin from file
 *   cmd &                run in background
 *   'literal'  "with $VAR and \" escapes"   quoting
 *   $VAR  ${VAR}         environment variable expansion
 */

#include <sys/types.h>
#include <sys/wait.h>

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <termios.h>

#define MAX_LINE 256 /* longest command line accepted              */
#define MAX_TOKENS 64 /* most tokens (words + operators) per line    */
#define MAX_ARGS 64 /* most arguments passed to one program        */
#define MAX_CMDS 16 /* most stages in a pipeline                   */
#define MAX_PATH 128 /* longest "<name>.elf" path                  */
#define TOK_STORE (MAX_LINE * 4) /* backing store for expanded tokens */
#define VAR_NAME 64 /* longest environment variable name           */
#define HIST_MAX 16 /* most command lines remembered               */
#define MAX_CWD 256 /* longest cwd shown in the prompt (kernel cap) */

#define ELF_SUFFIX ".elf"

extern char **environ; /* current environment (POSIX) */

static const char prompt[] = "so3% ";

/* One parsed pipeline stage: its argv plus optional redirections. */
struct command {
	char *argv[MAX_ARGS];
	int argc;
	char *in_file; /* < file            (NULL if none) */
	char *out_file; /* > / >> file       (NULL if none) */
	int append; /* out_file uses >>  (else >)       */
};

/*
 * SIGINT (Ctrl-C): the shell itself must not die. Only note the event with an
 * async-signal-safe write; the main loop reprints the prompt.
 */
static void sigint_handler(int sig)
{
	(void) sig;
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

/* -------------------------------------------------------------------------- */
/* Line editor: history + arrow keys (raw mode)                               */
/* -------------------------------------------------------------------------- */

/*
 * The kernel console is line-disciplined (ICANON|ECHO) by default. For
 * interactive editing we switch it to raw (no canonical line buffering, no
 * echo) only while reading a line, do our own echo / cursor handling, and
 * restore the original mode before running any command (so external programs
 * see a normal cooked terminal). On a non-tty (e.g. piped input) tcgetattr
 * fails and we fall back to fgets() via read_line().
 */

static int raw_ok; /* terminal supports raw mode (is a console) */
static char history[HIST_MAX][MAX_LINE];
static int hist_count;

static void out(const char *s, int n)
{
	if (n > 0)
		write(STDOUT_FILENO, s, n);
}

static void outc(char c)
{
	write(STDOUT_FILENO, &c, 1);
}

static void backspaces(int n)
{
	while (n-- > 0)
		outc('\b');
}

/* Append a line to the history ring (drop oldest when full, skip dups). */
static void history_add(const char *line)
{
	if (line[0] == '\0')
		return;
	if (hist_count > 0 && !strcmp(history[hist_count - 1], line))
		return;

	if (hist_count == HIST_MAX) {
		memmove(history[0], history[1], (HIST_MAX - 1) * MAX_LINE);
		hist_count--;
	}

	strncpy(history[hist_count], line, MAX_LINE - 1);
	history[hist_count][MAX_LINE - 1] = '\0';
	hist_count++;
}

/* Replace the on-screen line content with newl, fixing up the cursor. */
static void replace_line(char *buf, int *len, int *pos, const char *newl)
{
	int nl = (int) strlen(newl);

	backspaces(*pos); /* cursor back to the start of the input */
	out(newl, nl);
	if (*len > nl) { /* erase the tail of the previous (longer) line */
		int diff = *len - nl;
		while (diff-- > 0)
			outc(' ');
		backspaces(*len - nl);
	}

	memcpy(buf, newl, nl);
	buf[nl] = '\0';
	*len = nl;
	*pos = nl;
}

static int enable_raw(struct termios *saved)
{
	struct termios raw;

	if (tcgetattr(STDIN_FILENO, saved) < 0)
		return -1;

	raw = *saved;
	raw.c_lflag &= ~(ICANON | ECHO);
	return tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

/*
 * Read a line with in-place editing, history (Up/Down) and cursor movement
 * (Left/Right). Returns the length, or -1 if raw mode is unavailable (caller
 * should fall back to read_line), or -2 if the line was cancelled (Ctrl-C).
 */
static int read_line_edit(char *buf, int size)
{
	struct termios saved;
	char curstash[MAX_LINE];
	int len = 0, pos = 0, ret;
	int hpos = hist_count; /* history cursor; == count means "new line" */

	if (enable_raw(&saved) < 0)
		return -1;

	curstash[0] = '\0';

	for (;;) {
		char c;
		int r = read(STDIN_FILENO, &c, 1);

		if (r <= 0 || c == 3) { /* EINTR/EOF or Ctrl-C: cancel */
			outc('\n');
			ret = -2;
			goto done;
		}

		if (c == '\r' || c == '\n') {
			outc('\n');
			ret = len;
			goto done;
		} else if (c == 127 || c == 8) { /* backspace */
			if (pos > 0) {
				memmove(&buf[pos - 1], &buf[pos], len - pos);
				pos--;
				len--;
				outc('\b');
				out(&buf[pos], len - pos);
				outc(' ');
				backspaces(len - pos + 1);
			}
		} else if (c == 27) { /* ESC: cursor / navigation keys */
			char a, b;

			if (read(STDIN_FILENO, &a, 1) <= 0)
				continue;
			if (a != '[' && a != 'O') /* unrecognised sequence */
				continue;
			if (read(STDIN_FILENO, &b, 1) <= 0)
				continue;

			/* Extended form "ESC [ <digit> ~" (Home/End on many
			 * terminals): consume up to the trailing '~'. */
			if (a == '[' && b >= '0' && b <= '9') {
				char t;

				do {
					if (read(STDIN_FILENO, &t, 1) <= 0)
						break;
				} while (t != '~');

				if (b == '1' || b == '7') { /* Home */
					backspaces(pos);
					pos = 0;
				} else if (b == '4' || b == '8') { /* End */
					out(&buf[pos], len - pos);
					pos = len;
				}
				continue;
			}

			switch (b) {
			case 'A': /* up: older history */
				if (hpos > 0) {
					if (hpos == hist_count) {
						buf[len] = '\0';
						strcpy(curstash, buf);
					}
					hpos--;
					replace_line(buf, &len, &pos, history[hpos]);
				}
				break;
			case 'B': /* down: newer history */
				if (hpos < hist_count) {
					hpos++;
					replace_line(buf, &len, &pos, hpos == hist_count ? curstash : history[hpos]);
				}
				break;
			case 'C': /* right */
				if (pos < len) {
					outc(buf[pos]);
					pos++;
				}
				break;
			case 'D': /* left */
				if (pos > 0) {
					outc('\b');
					pos--;
				}
				break;
			case 'H': /* Home (ESC [ H / ESC O H) */
				backspaces(pos);
				pos = 0;
				break;
			case 'F': /* End (ESC [ F / ESC O F) */
				out(&buf[pos], len - pos);
				pos = len;
				break;
			}
		} else if (c >= 32 && c < 127) { /* printable: insert at cursor */
			if (len < size - 1) {
				memmove(&buf[pos + 1], &buf[pos], len - pos);
				buf[pos] = c;
				len++;
				out(&buf[pos], len - pos);
				backspaces(len - pos - 1);
				pos++;
			}
		}
	}

done:
	tcsetattr(STDIN_FILENO, TCSANOW, &saved);
	if (ret >= 0) {
		buf[ret] = '\0';
		history_add(buf);
	}
	return ret;
}

/* -------------------------------------------------------------------------- */
/* Tokenizer (quoting + $VAR expansion)                                       */
/* -------------------------------------------------------------------------- */

/* Append c to the token store if there is room. */
static void put(char **w, char *end, char c)
{
	if (*w < end)
		*(*w)++ = c;
}

/* Is c valid in an environment variable name? */
static int is_name_char(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

/*
 * Expand a $VAR or ${VAR} starting at *pp (which points at the '$'), writing
 * the value into the token store. Advances *pp past the reference.
 */
static void expand_var(char **pp, char **w, char *end)
{
	char *p = *pp + 1; /* skip '$' */
	char name[VAR_NAME];
	int ni = 0, braced = 0;
	char *val;

	if (*p == '{') {
		braced = 1;
		p++;
	}

	while (*p && ni < VAR_NAME - 1 && is_name_char(*p))
		name[ni++] = *p++;
	name[ni] = '\0';

	if (braced && *p == '}')
		p++;

	if (ni == 0)
		put(w, end, '$'); /* lone '$' / empty ${} -> literal '$' */
	else {
		val = getenv(name);
		while (val != NULL && *val)
			put(w, end, *val++);
	}

	*pp = p;
}

/*
 * Split line into tok[] (stored NUL-terminated in store), honouring single
 * quotes (literal), double quotes (with $VAR + \ escapes) and backslash
 * escaping. Operators (| < > >> &) must be whitespace-separated. Returns the
 * token count.
 */
static int tokenize(char *line, char *tok[], int max, char *store, int store_size)
{
	char *p = line;
	char *w = store;
	char *end = store + store_size - 1;
	int n = 0;

	while (*p) {
		while (*p == ' ' || *p == '\t')
			p++;
		if (!*p || n >= max)
			break;

		tok[n] = w;

		while (*p && *p != ' ' && *p != '\t') {
			if (*p == '\'') {
				p++;
				while (*p && *p != '\'')
					put(&w, end, *p++);
				if (*p == '\'')
					p++;
			} else if (*p == '"') {
				p++;
				while (*p && *p != '"') {
					if (*p == '$')
						expand_var(&p, &w, end);
					else if (*p == '\\' && (p[1] == '"' || p[1] == '\\' || p[1] == '$')) {
						p++;
						put(&w, end, *p++);
					} else
						put(&w, end, *p++);
				}
				if (*p == '"')
					p++;
			} else if (*p == '\\') {
				p++;
				if (*p)
					put(&w, end, *p++);
			} else if (*p == '$') {
				expand_var(&p, &w, end);
			} else
				put(&w, end, *p++);
		}

		put(&w, end, '\0');
		n++;
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

static int builtin_history(int argc, char *argv[])
{
	int i;

	(void) argc;
	(void) argv;

	for (i = 0; i < hist_count; i++)
		printf("%5d  %s\n", i + 1, history[i]);

	return 0;
}

static int builtin_cd(int argc, char *argv[])
{
	const char *dir = (argc >= 2) ? argv[1] : "/";

	if (chdir(dir) < 0)
		printf("cd: %s: no such directory\n", dir);

	return 0;
}

static int builtin_pwd(int argc, char *argv[])
{
	char buf[256];

	(void) argc;
	(void) argv;

	if (getcwd(buf, sizeof(buf)) != NULL)
		printf("%s\n", buf);
	else
		printf("pwd: cannot determine current directory\n");

	return 0;
}

struct builtin {
	const char *name;
	int (*fn)(int argc, char *argv[]);
};

static const struct builtin builtins[] = {
	{ "exit", builtin_exit },	{ "env", builtin_env }, { "setenv", builtin_setenv }, { "kill", builtin_kill },
	{ "history", builtin_history }, { "cd", builtin_cd },	{ "pwd", builtin_pwd },	      { NULL, NULL },
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
/* Pipeline parsing and execution                                             */
/* -------------------------------------------------------------------------- */

/*
 * Split the token list into pipeline stages around '|', recording '<', '>',
 * '>>' redirections per stage and a trailing '&'. Returns the number of stages,
 * or -1 on a syntax error (already reported).
 */
static int parse_commands(int n, char *tok[], struct command cmds[], int maxc, int *background)
{
	int k = 0, i;

	*background = 0;
	memset(&cmds[0], 0, sizeof(cmds[0]));

	for (i = 0; i < n; i++) {
		char *t = tok[i];

		if (!strcmp(t, "&")) {
			*background = 1;
			break; /* anything after '&' is ignored */
		} else if (!strcmp(t, "|")) {
			if (cmds[k].argc == 0) {
				printf("sh: syntax error near '|'.\n");
				return -1;
			}
			if (++k >= maxc) {
				printf("sh: too many pipeline stages.\n");
				return -1;
			}
			memset(&cmds[k], 0, sizeof(cmds[k]));
		} else if (!strcmp(t, "<")) {
			if (++i >= n) {
				printf("sh: expected file after '<'.\n");
				return -1;
			}
			cmds[k].in_file = tok[i];
		} else if (!strcmp(t, ">") || !strcmp(t, ">>")) {
			int append = (t[1] == '>');
			if (++i >= n) {
				printf("sh: expected file after '%s'.\n", t);
				return -1;
			}
			cmds[k].out_file = tok[i];
			cmds[k].append = append;
		} else if (cmds[k].argc < MAX_ARGS - 1)
			cmds[k].argv[cmds[k].argc++] = t;
	}

	for (i = 0; i <= k; i++) {
		if (cmds[i].argc == 0) {
			printf("sh: syntax error.\n");
			return -1;
		}
		cmds[i].argv[cmds[i].argc] = NULL;
	}

	return k + 1;
}

/* Apply a stage's file redirections in the child. exit()s on failure. */
static void apply_redirections(const struct command *c)
{
	int fd;

	if (c->in_file != NULL) {
		fd = open(c->in_file, O_RDONLY);
		if (fd < 0) {
			printf("sh: cannot open '%s'.\n", c->in_file);
			exit(1);
		}
		dup2(fd, STDIN_FILENO);
		close(fd);
	}

	if (c->out_file != NULL) {
		int flags = O_WRONLY | O_CREAT | (c->append ? O_APPEND : O_TRUNC);
		fd = open(c->out_file, flags, 0644);
		if (fd < 0) {
			printf("sh: cannot open '%s'.\n", c->out_file);
			exit(1);
		}
		dup2(fd, STDOUT_FILENO);
		close(fd);
	}
}

/*
 * Replace the current (child) process image with "<argv[0]>.elf". Never
 * returns on success; on failure it prints an error and exits the child.
 */
static void exec_elf(char *argv[])
{
	char path[MAX_PATH];

	/* A bare command name (no '/') is looked up in the root directory — all
	 * executables live there and there is no PATH. A name containing '/' is
	 * a path, resolved by the kernel against the current working directory. */
	if (strchr(argv[0], '/') != NULL)
		snprintf(path, sizeof(path), "%s%s", argv[0], ELF_SUFFIX);
	else
		snprintf(path, sizeof(path), "/%s%s", argv[0], ELF_SUFFIX);

	execv(path, argv);

	printf("%s: exec failed.\n", argv[0]);
	exit(127);
}

/*
 * Fork+exec each stage, wiring stage i's stdout into stage i+1's stdin via
 * pipes. Explicit redirections (applied after the pipe wiring) take precedence.
 */
static void run_pipeline(struct command cmds[], int ncmd, int background)
{
	pid_t pids[MAX_CMDS];
	int prev_read = -1;
	int started = 0;
	int i;

	for (i = 0; i < ncmd; i++) {
		int pfd[2] = { -1, -1 };
		pid_t pid;

		if (i < ncmd - 1 && pipe(pfd) < 0) {
			printf("sh: pipe failed.\n");
			break;
		}

		pid = fork();
		if (pid < 0) {
			printf("sh: fork failed.\n");
			if (pfd[0] != -1)
				close(pfd[0]);
			if (pfd[1] != -1)
				close(pfd[1]);
			break;
		}

		if (pid == 0) { /* child */
			if (prev_read != -1)
				dup2(prev_read, STDIN_FILENO);
			if (i < ncmd - 1)
				dup2(pfd[1], STDOUT_FILENO);

			if (prev_read != -1)
				close(prev_read);
			if (pfd[0] != -1)
				close(pfd[0]);
			if (pfd[1] != -1)
				close(pfd[1]);

			apply_redirections(&cmds[i]);
			exec_elf(cmds[i].argv);
		}

		/* parent */
		pids[started++] = pid;
		if (prev_read != -1)
			close(prev_read);
		if (i < ncmd - 1) {
			close(pfd[1]);
			prev_read = pfd[0];
		}
	}

	if (prev_read != -1)
		close(prev_read);

	if (background && started > 0)
		printf("[%d]\n", pids[started - 1]);
	else
		for (i = 0; i < started; i++)
			waitpid(pids[i], NULL, 0);
}

/* -------------------------------------------------------------------------- */
/* Main loop                                                                  */
/* -------------------------------------------------------------------------- */

int main(int argc, char *argv[])
{
	char line[MAX_LINE];
	char store[TOK_STORE];
	char *tok[MAX_TOKENS];
	char cwd[MAX_CWD];
	struct command cmds[MAX_CMDS];
	struct sigaction sa;
	int n, ncmd, background;

	(void) argc;
	(void) argv;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sigint_handler;
	sigaction(SIGINT, &sa, NULL);

	/* Use the interactive line editor when stdin is the console; fall back
	 * to plain line input (fgets) for piped/redirected input. */
	raw_ok = (isatty(STDIN_FILENO) == 1);

	while (1) {
		reap_children();

		/* Prompt shows the current working directory, e.g. "/dev % ". */
		if (getcwd(cwd, sizeof(cwd)) != NULL)
			printf("%s %% ", cwd);
		else
			printf("%s", prompt);
		fflush(stdout);

		if (raw_ok) {
			n = read_line_edit(line, sizeof(line));
			if (n == -1) { /* raw mode unavailable after all */
				raw_ok = 0;
				n = read_line(line, sizeof(line));
			}
		} else
			n = read_line(line, sizeof(line));
		if (n < 0) /* EOF or cancelled (Ctrl-C): reprompt */
			continue;

		n = tokenize(line, tok, MAX_TOKENS, store, sizeof(store));
		if (n == 0)
			continue;

		ncmd = parse_commands(n, tok, cmds, MAX_CMDS, &background);
		if (ncmd < 0)
			continue;

		/* A lone builtin runs in the shell itself; anything in a
		 * pipeline or with redirection goes through the fork path. */
		if (ncmd == 1 && !background && cmds[0].in_file == NULL && cmds[0].out_file == NULL &&
		    run_builtin(cmds[0].argc, cmds[0].argv))
			continue;

		run_pipeline(cmds, ncmd, background);
	}

	return 0;
}
