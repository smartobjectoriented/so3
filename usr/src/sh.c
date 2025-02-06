/*
 * Copyright (C) 2014-2017 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#include <sys/types.h>
#include <sys/wait.h>

#include <syscall.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <syscall.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>

#define TOKEN_NR 10
#define ARGS_MAX 16

char tokens[TOKEN_NR][80];
char prompt[] = "so3% ";
char file_buff[500];

void parse_token(char *str)
{
	int i = 0;
	char *next_token;

	next_token = strtok(str, " ");
	if (!next_token)
		return;

	strcpy(tokens[i++], next_token);

	while ((next_token = strtok(NULL, " ")) != NULL)
		strcpy(tokens[i++], next_token);
}

/**
 * Remove 0 before command
 */
void trim(char *buffer, int n)
{
	int i;
	char *new_buff = calloc(80, sizeof(char));
	for (i = 0; i < n; i++) {
		if (buffer[i] != 0) {
			break;
		}
	}
	memcpy(new_buff, buffer + i, n - i);
	memcpy(buffer, new_buff, n);
	free(new_buff);
}

/**
 * Detect if its a escape sequence
 */
int is_escape_sequence(const char *str)
{
	return str[0] == '\x1b' && str[1] == '[';
}

/**
 * Escape arrow key sequence to avoid interpret them
 */
void escape_arrow_key(char *buffer, int size)
{
	int i, j;
	char *new_buff = calloc(size, sizeof(char));
	i = j = 0;
	while (i < size) {
		if (is_escape_sequence(&buffer[i])) {
			i += 3;
		} else {
			new_buff[j++] = buffer[i++];
		}
	}
	memcpy(buffer, new_buff, size);
	free(new_buff);
}

/**
 * More secure way and escaped way to get user input
 */
void get_user_input(char *buffer, int buf_size)
{
	if (buffer == NULL || buf_size <= 0) {
		return;
	}

	memset(buffer, 0, buf_size);

	if (fgets(buffer, buf_size, stdin) != NULL) {
		escape_arrow_key(buffer, buf_size);
		trim(buffer, buf_size);
		size_t len = strlen(buffer);
		if (len > 0 && buffer[len - 1] == '\n') {
			buffer[len - 1] = '\0';
		}
	}
}

/*
 * Process the command with the different tokens
 */
void process_cmd(void)
{
	int i, pid_child, background, arg_pos, arg_pos2, redirection, byte_read;
	char *argv[ARGS_MAX], *argv2[ARGS_MAX];
	char filename[30];
	int pid, sig, pid_child2, fd;
	int pipe_on = 0;
	int pipe_fd[2];

	if (!strcmp(tokens[0], "dumpsched")) {
		sys_info(1, 0);
		return;
	}

	if (!strcmp(tokens[0], "dumpproc")) {
		sys_info(4, 0);
		return;
	}

	if (!strcmp(tokens[0], "exit")) {
		if (getpid() == 1) {
			printf("The shell root process can not be terminated...\n");
			return;
		} else
			exit(0);

		/* If the shell is the root shell, there is a failure on exit() */
		return;
	}

	/* setenv */
	if (!strcmp(tokens[0], "setenv")) {
		/* second arg present ? */
		if (tokens[1][0] != 0) {
			/* third arg present gets(user_input);? */
			if (tokens[2][0] != 0) {
				/* Set the env. var. (always overwrite) */
				setenv(tokens[1], tokens[2], 1);
			} else
				unsetenv(tokens[1]);
		}
		return;
	}

	/* env */
	if (!strcmp(tokens[0], "env")) {
		/* This function print the environment vars */
		for (i = 0; __environ[i] != NULL; i++)
			printf("%s\n", __environ[i]);

		return;
	}

	/* kill */
	if (!strcmp(tokens[0], "kill")) {
		/* Send a signal to a process */
		sig = 0;

		if (tokens[2][0] == 0) {
			sig = SIGTERM;
			pid = atoi(tokens[1]);
		} else {
			if (!strcmp(tokens[1], "-USR1")) {
				sig = SIGUSR1;
				pid = atoi(tokens[2]);
			} else if (!strcmp(tokens[1], "-9")) {
				sig = SIGKILL;
				pid = atoi(tokens[2]);
			}
		}

		kill(pid, sig);

		return;
	}

	/* General case - prepare to launch the application */

	/* Prepare the arguments to be passed to exec() syscall */
	arg_pos = 0;
	arg_pos2 = 0;
	background = 0;
	redirection = 0;
	while (!background && tokens[arg_pos][0] != 0) {
		/* Check for & */
		if (!strcmp(tokens[arg_pos], "&"))
			background = 1;
		else {
			if (!strcmp(tokens[arg_pos], "|")) {
				pipe_on = 1;
				argv[arg_pos] = NULL;
			} else if (!strcmp(tokens[arg_pos], ">")) {
				redirection = 1;
				argv[arg_pos] = NULL;
			} else {
				if (pipe_on) {
					argv2[arg_pos2] = tokens[arg_pos];
					arg_pos2++;
				} else if (redirection) {
					argv2[0] = tokens[arg_pos];
				} else
					argv[arg_pos] = tokens[arg_pos];
			}
			arg_pos++;
		}
	}
	/* Terminate the list of args properly */
	if (pipe_on)
		argv2[arg_pos2] = NULL;
	else
		argv[arg_pos] = NULL;

	pid_child = fork();

	if (!pid_child) { /* Execution in the child */

		if (pipe_on) {
			pipe(pipe_fd);
			pid_child2 = fork();

			if (!pid_child2) {
				close(pipe_fd[0]);

				dup2(pipe_fd[1], STDOUT_FILENO);

				strcpy(filename, argv[0]);
				strcat(filename, ".elf");

				if (execv(filename, argv) == -1) {
					printf("%s: exec failed.\n", argv[0]);
					exit(-1);
				}

			} else {
				close(pipe_fd[1]);

				dup2(pipe_fd[0], STDIN_FILENO);

				strcpy(filename, argv2[0]);
				strcat(filename, ".elf");

				if (execv(filename, argv2) == -1) {
					printf("%s: exec failed.\n", argv2[0]);
					exit(-1);
				}
			}

		} else if (redirection) {
			fd = open(argv2[0], O_WRONLY | O_CREAT);
			if (fd < 0) {
				printf("Error opening/creating output file...\n");
				return;
			}

			pipe(pipe_fd);
			pid_child2 = fork();
			if (!pid_child2) {
				close(pipe_fd[0]);
				dup2(pipe_fd[1], STDOUT_FILENO);
				strcpy(filename, argv[0]);
				strcat(filename, ".elf");

				if (execv(filename, argv) == -1) {
					printf("%s: exec failed.\n", argv[0]);
					exit(-1);
				}

			} else {
				close(pipe_fd[1]);
				while ((byte_read = read(pipe_fd[0], file_buff,
							 500)) > 0) {
					write(fd, file_buff, byte_read);
				}
				close(fd);
			}

		} else {
			strcpy(filename, tokens[0]);
			argv[0] = tokens[0];

			/* We are looking for an executable with .elf extension */
			strcat(filename, ".elf");

			if (execv(filename, argv) == -1) {
				printf("%s: exec failed.\n", argv[0]);
				exit(-1);
			}
		}
	} else { /* Execution in the parent */

		/* If the process is running in background, waitpid() will
		 * be called when the SIGCHLD signal is received.
		 */
		if (!background)
			/* Wait for our child to be finished. */
			waitpid(pid_child, NULL, 0);
		else
			/* Display the PID */
			printf("[%d]\n", pid_child);
	}
}

/*
 * Ignore the SIGINT signal, but we re-display the prompt to be elegant ;-)
 */
void sigint_sh_handler(int sig)
{
	printf("%s", prompt);
	fflush(stdout);
}
/*d
 * Main entry point of the shell application.
 */
int main(int argc, char *argv[])
{
	char user_input[80];
	int i;
	struct sigaction sa;

	memset(&sa, 0, sizeof(struct sigaction));

	sa.sa_handler = sigint_sh_handler;
	sigaction(SIGINT, &sa, NULL);

	while (1) {
		/* Reset all tokens */
		for (i = 0; i < TOKEN_NR; i++)
			tokens[i][0] = 0;

		printf("%s", prompt);
		fflush(stdout);

		get_user_input(user_input, 80);

		if (strcmp(user_input, ""))
			parse_token(user_input);

		/* Check if there is at least one token to be processed */
		if (tokens[0][0] != 0)
			process_cmd();
	}
}
