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

#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

static int long_format; /* -l */

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

	join_path(path, sizeof(path), dir, e->d_name);

	if (stat(path, &st) == 0) {
		time_t t = st.st_mtime;
		struct tm tm;

		if (localtime_r(&t, &tm) != NULL)
			strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M", &tm);
		else
			strcpy(tbuf, "----------------");

		printf("%c %10ld  %s  %s%s\n", type, (long) st.st_size, tbuf, e->d_name, suffix);
	} else {
		/* e.g. /dev entries have no FAT stat — show what readdir gave us. */
		printf("%c %10s  %s  %s%s\n", type, "-", "----------------", e->d_name, suffix);
	}
}

/* Print one entry in the default (name-only) format. */
static void print_short(struct dirent *e)
{
	switch (e->d_type) {
	case DT_DIR:
		printf("%s/\n", e->d_name);
		break;
	case DT_REG:
	case DT_CHR:
		printf("%s\n", e->d_name);
		break;
	default:
		break;
	}
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
		printf("ls: cannot open '%s'\n", dir);
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
