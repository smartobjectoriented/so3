/*
 * Copyright (C) 2014-2026 REDS Institute from HEIG-VD <daniel.rossier@heig-vd.ch>
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

/* ls: list directory contents (or the paths given as arguments). Supports -l
 * (long format: type, size, mtime) and colourised output by entry type. */

#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

static int long_format; /* -l */
static int use_color = 1; /* default: color on */

/* ANSI color codes, named per file role */
#define COLOR_RESET   "\033[0m"
#define COLOR_DIR     "\033[01;34m"   /* bold blue    — directories           */
#define COLOR_EXEC    "\033[01;32m"   /* bold green   — executables (.elf)    */
#define COLOR_SCRIPT  "\033[01;33m"   /* bold yellow  — shell / script files  */
#define COLOR_SRC     "\033[01;35m"   /* bold magenta — source code & headers */
#define COLOR_BIN     "\033[01;31m"   /* bold red     — binary / object / lib */
/*
 * Normal files, and plain-text files that are NOT source code (.txt, .md,
 * .conf, …), use the terminal default colour (white): file_color() returns
 * NULL for them and print_colored() prints them uncolored.
 */

/*
 * Return the color code for a filename based on its extension, or NULL for a
 * normal / non-source-text file (rendered uncolored, i.e. white).
 */
static const char *file_color(const char *name)
{
	const char *dot = strrchr(name, '.');

	if (dot != NULL) {
		const char *ext = dot + 1;

		/* Source / header files — magenta */
		if (strcmp(ext, "c") == 0 || strcmp(ext, "h") == 0 ||
		    strcmp(ext, "cpp") == 0 || strcmp(ext, "hpp") == 0 ||
		    strcmp(ext, "s") == 0 || strcmp(ext, "S") == 0 ||
		    strcmp(ext, "asm") == 0)
			return COLOR_SRC;

		/* Shell / script files — yellow */
		if (strcmp(ext, "sh") == 0 || strcmp(ext, "bash") == 0 ||
		    strcmp(ext, "zsh") == 0 || strcmp(ext, "py") == 0 ||
		    strcmp(ext, "rb") == 0)
			return COLOR_SCRIPT;

		/* Executables — green */
		if (strcmp(ext, "elf") == 0)
			return COLOR_EXEC;

		/* Binary / object / library — red */
		if (strcmp(ext, "o") == 0 || strcmp(ext, "a") == 0 ||
		    strcmp(ext, "so") == 0 || strcmp(ext, "bin") == 0)
			return COLOR_BIN;

		/*
		 * Plain-text / config files (.conf .cfg .ini .txt .md .rst, …) are
		 * not source code: fall through to NULL so they stay white.
		 */
	}

	/* Normal file (no recognised extension): no color (white). */
	return NULL;
}

/* Print a string with the given color, then reset. */
static void print_colored(const char *str, const char *color)
{
	if (use_color && color != NULL)
		printf("%s%s" COLOR_RESET, color, str);
	else
		printf("%s", str);
}

/* Color for an entry given its readdir type and name (NULL = white). */
static const char *entry_color(unsigned char d_type, const char *name)
{
	switch (d_type) {
	case DT_DIR:
		return COLOR_DIR;
	case DT_CHR:
		return COLOR_EXEC;
	case DT_REG:
		return file_color(name);
	default:
		return NULL;
	}
}

/* Join a directory path and an entry name into out[]. */
static void join_path(char *out, size_t outsz, const char *dir, const char *name)
{
	size_t len = strlen(dir);

	if (len > 0 && dir[len - 1] == '/')
		snprintf(out, outsz, "%s%s", dir, name);
	else
		snprintf(out, outsz, "%s/%s", dir, name);
}

/* Print one entry in the long format: type, size, mtime, name. */
static void print_long(const char *dir, struct dirent *e)
{
	char path[256], tbuf[24];
	struct stat st;
	char type = (e->d_type == DT_DIR) ? 'd' : '-';
	const char *suffix = (e->d_type == DT_DIR) ? "/" : "";
	const char *color = entry_color(e->d_type, e->d_name);

	join_path(path, sizeof(path), dir, e->d_name);

	if (stat(path, &st) == 0) {
		time_t t = st.st_mtime;
		struct tm tm;

		if (localtime_r(&t, &tm) != NULL)
			strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M", &tm);
		else
			strcpy(tbuf, "----------------");

		printf("%c %10ld  %s  ", type, (long) st.st_size, tbuf);
	} else {
		/* e.g. /dev entries have no FAT stat — show what readdir gave us. */
		printf("%c %10s  %s  ", type, "-", "----------------");
	}

	print_colored(e->d_name, color);
	printf("%s\n", suffix);
}

/* Print a long-format line from a stat result (used for a file argument). */
static void print_long_stat(const char *name, const struct stat *st)
{
	char tbuf[24];
	char type = S_ISDIR(st->st_mode) ? 'd' : '-';
	const char *suffix = S_ISDIR(st->st_mode) ? "/" : "";
	const char *color = S_ISDIR(st->st_mode) ? COLOR_DIR : file_color(name);
	time_t t = st->st_mtime;
	struct tm tm;

	if (localtime_r(&t, &tm) != NULL)
		strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M", &tm);
	else
		strcpy(tbuf, "----------------");

	printf("%c %10ld  %s  ", type, (long) st->st_size, tbuf);
	print_colored(name, color);
	printf("%s\n", suffix);
}

/* Print one entry in the default (name-only) format. */
static void print_short(struct dirent *e)
{
	const char *suffix = (e->d_type == DT_DIR) ? "/" : "";

	/* entry_color() returns NULL for normal / non-source-text files, which
	 * print_colored() renders uncolored (white). */
	print_colored(e->d_name, entry_color(e->d_type, e->d_name));
	printf("%s\n", suffix);
}

/*
 * ls [-l] [DIR]
 *
 * Lists the entries of DIR (default: the current directory). With -l, each
 * entry is shown with its type, size and modification time.
 */
int main(int argc, char **argv)
{
	const char *dir = NULL;
	DIR *stream;
	struct dirent *entry;
	int i;

	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-' && argv[i][1] != '\0') {
			const char *p;

			for (p = argv[i] + 1; *p; p++) {
				if (*p == 'l')
					long_format = 1;
				else {
					printf("ls: unknown option -%c\n", *p);
					return 1;
				}
			}
		} else
			dir = argv[i]; /* last DIR wins (single directory) */
	}

	if (dir == NULL)
		dir = ".";

	stream = opendir(dir);
	if (stream == NULL) {
		/* Not a directory: list it as a single entry if it exists. */
		struct stat st;

		if (stat(dir, &st) == 0) {
			if (long_format)
				print_long_stat(dir, &st);
			else
				printf("%s\n", dir);
			return 0;
		}

		printf("ls: cannot access '%s'\n", dir);
		return 1;
	}

	while ((entry = readdir(stream)) != NULL) {
		if (long_format)
			print_long(dir, entry);
		else
			print_short(entry);
	}

	closedir(stream);

	return 0;
}
