/*
 * Copyright (C) 2025 André Costa <andre_miguel_costa@hotmail.com>
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
 * Simple Init Program for SO3
 * ----------------------------
 * This program serves as a minimal init system for SO3, capable of reading and 
 * executing commands from a configuration file.
 *
 * Supported Commands:
 * -------------------
 * - exit       : Terminates the init process.
 * - shell      : Launches the shell (`sh.elf`).
 * - echo       : Prints arguments to stdout.
 * - run <file> : Executes an ELF binary specified by <file>, along with any 
 *                additional arguments provided.
 *
 * Command Execution:
 * ------------------
 * Commands are read from a file and executed sequentially.
 * The "run" command forks a new process, executes the specified ELF binary,
 * and waits for its completion before proceeding to the next command.
 * By default, the program executes the shell program when it's done, you can
 * change this behaviour with an `exit` command at the end of the file.
 *
 * This init system simplifies scripting in SO3 by allowing easy modifications 
 * through a simple text-based configuration.
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
#include <stdbool.h>

#define INPUT_COMMAND_FILE "commands.ini"
#define ARGS_MAX 16
#define MAX_COMMAND_LEN 512

int fd = -1;

static void close_file(void);
static void start_shell(void);
static int process_cmd(const char *command);
static void sigint_handler(int sig);
static int process_file(int fd);

void close_file(void)
{
	if (fd >= 0) {
		close(fd);
	}
	fd = -1;
}
/*
 * Executes the shell
 * This function replaces the current process with the shell executable.
 * If execv fails, the process will terminate.
 *
 */
void start_shell(void)
{
	execv("sh.elf", NULL);
	printf("Error starting shell\n");
	exit(1);
}

/*
 * Processes a command which must be a NULL-terminated string
 *
 * Returns -1 on error
 */
int process_cmd(const char *command)
{
	char buffer[MAX_COMMAND_LEN];
	char *args[ARGS_MAX];
	char *token;
	size_t argc = 0;

	strcpy(buffer, command);
	if (command == NULL) {
		printf("Command is null\n");
		return -1;
	}

	if (strlen(command) >= MAX_COMMAND_LEN) {
		printf("Command is too big. Ignoring it\n");
		return -1;
	}

	token = strtok(buffer, " ");
	while (token != NULL) {
		if (argc >= ARGS_MAX - 1) {
			printf("Too many command arguments found. Ignoring the command\n");
			return -1;
		}
		args[argc++] = token;
		token = strtok(NULL, " ");
	}

	args[argc] = NULL;

	if (argc < 1) {
		printf("Invalid command format: %s\n", command);
		return -1;
	}

	if (strcmp(args[0], "exit") == 0) {
		close_file();
		exit(0);
	} else if (strcmp(args[0], "shell") == 0) {
		start_shell();
		printf("Failed to launch shell\n");
		return -1;
	} else if (strcmp(args[0], "echo") == 0) {
		for (size_t i = 1; i < argc; ++i) {
			printf("%s ", args[i]);
		}
		printf("\n");
		return 0;
	} else if (strcmp(args[0], "run") == 0) {
		if (argc < 2) {
			printf("Missing filename for 'run' command\n");
			return -1;
		}

		pid_t pid = fork();
		if (pid < 0) {
			printf("Fork failed\n");
			return -1;
		} else if (pid == 0) {
			execv(args[1], &args[1]);
			printf("Execv failed\n");
			exit(1);
		} else {
			int status;
			waitpid(pid, &status, 0);
			return WEXITSTATUS(status);
		}
	} else {
		printf("Unknown command: %s\n", args[0]);
		return -1;
	}

	return 0;
}

/*
 * Ignore the SIGINT signal unless
 */
void sigint_handler(int sig)
{
	(void) sig;
}
/*
 * Processes the file given by `fd`
 * It's the caller's responsability to close `fd`
 *
 * Returns -1 on error and 0 on success
 */
int process_file(int fd)
{
#define LINE_LEN MAX_COMMAND_LEN + 1

	char buffer[LINE_LEN];
	char line[LINE_LEN];
	int line_index = 0;
	ssize_t bytes_read;
	int i;

	while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
		for (i = 0; i < bytes_read; i++) {
			if (buffer[i] == '\n') {
				line[line_index] = '\0';
				if (line_index > 0) {
					process_cmd(line);
				}
				line_index = 0;
			} else if (line_index >= LINE_LEN) {
				printf("Invalid Command file, command is too big\n");
				return -1;
			} else {
				line[line_index++] = buffer[i];
			}
		}
	}

	// Handle the last command if the file doesn't end with a newline
	if (line_index > 0) {
		line[line_index] = '\0';
		process_cmd(line);
	}
	return 0;
}

/*
 * Main entry point of the init application.
 */
int main(int argc, char *argv[])
{
	struct sigaction sa;
	fd = open(INPUT_COMMAND_FILE, O_RDONLY);
	/* By default, we start the shell process */
	if (fd < 0) {
		start_shell();
		return EXIT_FAILURE;
	}

	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_handler = sigint_handler;
	sigaction(SIGINT, &sa, NULL);

	process_file(fd);
	close_file();

	start_shell();
	/* Unreachable */
	return EXIT_FAILURE;
}
