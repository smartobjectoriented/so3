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

#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <dirent.h>

/*
 * Main function of ls application.
 * The ls application is very very short and does not support any options like -l -a etc.
 * It is only possible to give a subdir name to list the directory entries of this subdir.
 */
int main(int argc, char **argv) {
	DIR *stream;
	struct dirent  *p_entry;
	char *dir;

	if (argc == 1)
		dir = ".";
	else if (argc == 2)
		dir = argv[1];
	else {
		printf("Usage: ls [DIR]\n");
		exit(1);
	}

	stream = opendir(dir);

	if (stream == NULL)
		exit(1);

	while ((p_entry = readdir(stream)) != NULL) {
		switch (p_entry->d_type) {

		/* Directory entry */
		case DT_DIR:
			printf("%s/\n", p_entry->d_name);
			break;

		/* Regular entry */
		case DT_REG:
			printf("%s\n", p_entry->d_name);
			break;

		default:
			break;
		}
	}

	exit(0);
}

