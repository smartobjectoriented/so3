/*
 * Copyright (C) 2026 Daniel Rossier <daniel.rossier@heig-vd.ch>
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

#define RM_PATH_MAX 256
#define RM_NAME_MAX 128
#define RM_ENTRIES 128 /* entries handled per directory level */

static int opt_recursive;
static int opt_force;

static int remove_path(const char *path);

/*
 * Recursively empty a directory. The entry names are collected first and the
 * directory handle closed before anything is removed, so we never mutate a
 * directory while its read cursor is open.
 */
static int remove_dir_contents(const char *path)
{
	DIR *d;
	struct dirent *e;
	char(*names)[RM_NAME_MAX];
	char child[RM_PATH_MAX];
	int n = 0, i, rc = 0;

	d = opendir(path);
	if (d == NULL)
		return -1;

	names = malloc(RM_ENTRIES * RM_NAME_MAX);
	if (names == NULL) {
		closedir(d);
		return -1;
	}

	while ((e = readdir(d)) != NULL && n < RM_ENTRIES) {
		if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, ".."))
			continue;
		strncpy(names[n], e->d_name, RM_NAME_MAX - 1);
		names[n][RM_NAME_MAX - 1] = '\0';
		n++;
	}
	closedir(d);

	for (i = 0; i < n; i++) {
		snprintf(child, sizeof(child), "%s/%s", path, names[i]);
		if (remove_path(child) < 0)
			rc = -1;
	}

	free(names);
	return rc;
}

/* Remove a single path (file or, with -r, directory tree). */
static int remove_path(const char *path)
{
	DIR *d = opendir(path);

	if (d != NULL) { /* it is a directory */
		closedir(d);

		if (!opt_recursive) {
			if (!opt_force)
				printf("rm: '%s' is a directory (use -r)\n", path);
			return -1;
		}

		if (remove_dir_contents(path) < 0 && !opt_force)
			return -1;

		if (rmdir(path) < 0) {
			if (!opt_force)
				printf("rm: cannot remove '%s'\n", path);
			return -1;
		}
		return 0;
	}

	/* a regular file */
	if (unlink(path) < 0) {
		if (!opt_force)
			printf("rm: cannot remove '%s'\n", path);
		return -1;
	}

	return 0;
}

int main(int argc, char **argv)
{
	int i, rc = 0, nfiles = 0;

	for (i = 1; i < argc; i++) {
		const char *a = argv[i];

		if (a[0] == '-' && a[1] != '\0') {
			const char *p;

			for (p = a + 1; *p; p++) {
				if (*p == 'r' || *p == 'R')
					opt_recursive = 1;
				else if (*p == 'f')
					opt_force = 1;
				else {
					printf("rm: unknown option -%c\n", *p);
					return 1;
				}
			}
		} else {
			nfiles++;
			if (remove_path(a) < 0)
				rc = 1;
		}
	}

	if (nfiles == 0 && !opt_force) {
		printf("usage: rm [-r] [-f] FILE...\n");
		return 1;
	}

	return rc;
}
