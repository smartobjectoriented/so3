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

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <syscall.h>
#include <stdlib.h>
#include <signal.h>

#define TOKEN_NR	10
#define ARGS_MAX	16

char tokens[TOKEN_NR][80];
char prompt[] = "so3% ";

void parse_token(char *str) {
	int i = 0;
	char *next_token;

	next_token = strtok(str, " ");
	if (!next_token)
		return;

	strcpy(tokens[i++], next_token);

	while ((next_token = strtok(NULL, " ")) != NULL)
		strcpy(tokens[i++], next_token);
}

/*
 * Process the command with the different tokens
 */
void process_cmd(void) {
	int i, pid_child, background, arg_pos, arg_pos2;
	char *argv[ARGS_MAX], *argv2[ARGS_MAX];
	char filename[30];
	int pid, sig, pid_child2;
	int pipe_on = 0;
	int pipe_fd[2];

	if (!strcmp(tokens[0], "dumpsched")) {
		sys_info(1, 0);
		return ;
	}

	if (!strcmp(tokens[0], "dumpproc")) {
		sys_info(4, 0);
		return ;
	}

	if (!strcmp(tokens[0], "exit")) {
		if (getpid() == 1) {
			printf("The shell root process can not be terminated...\n");
			return ;
		} else
			exit(0);

		/* If the shell is the root shell, there is a failure on exit() */
		return ;
	}

	/* setenv */
	if (!strcmp(tokens[0], "setenv")) {
		/* second arg present ? */
		if (tokens[1][0] != 0) {
			/* third arg present ? */
			if (tokens[2][0] != 0) {
				/* Set the env. var. (always overwrite) */
				setenv(tokens[1], tokens[2], 1);
			} else
				unsetenv(tokens[1]);
		}
		return ;
	}

	/* env */
	if (!strcmp(tokens[0], "env")) {
		/* This function print the environment vars */
		for (i = 0; __environ[i] != NULL; i++)
			printf("%s\n", __environ[i]);

		return ;
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

		return ;
	}

	/* General case - prepare to launch the application */

	/* Prepare the arguments to be passed to exec() syscall */
	arg_pos = 0;
	arg_pos2 = 0;
	background = 0;
	while (!background && tokens[arg_pos][0] != 0) {
		/* Check for & */
		if (!strcmp(tokens[arg_pos], "&"))
			background = 1;
		else {
			if (!strcmp(tokens[arg_pos], "|")) {
				pipe_on = 1;
				argv[arg_pos] = NULL;
			} else {
				if (pipe_on) {
					argv2[arg_pos2] = tokens[arg_pos];
					arg_pos2++;
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

			}
			else {
				close(pipe_fd[1]);

				dup2(pipe_fd[0], STDIN_FILENO);

				strcpy(filename, argv2[0]);
				strcat(filename, ".elf");

				if (execv(filename, argv2) == -1) {
					printf("%s: exec failed.\n", argv2[0]);
					exit(-1);
				}
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
void sigint_sh_handler(int sig) {

	printf("%s", prompt);
	fflush(stdout);
}
/*
 * Main entry point of the shell application.
 */
void main(int argc, char *argv[])
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

		gets(user_input);

		if (strcmp(user_input, ""))
			parse_token(user_input);

		/* Check if there is at least one token to be processed */
		if (tokens[0][0] != 0)
			process_cmd();


	}
}
