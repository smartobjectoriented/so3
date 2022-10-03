/*
 * Copyright (C) 2015-2017 Daniel Rossier <daniel.rossier@heig-vd.ch>
 * Copyright (C) 2017 Xavier Ruppen <xavier.ruppen@heig-vd.ch>
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

#ifndef ELF_H
#define ELF_H

#include <elf-struct.h>

#define ELF_MAXSIZE (128 * SZ_1K)

/* ELF Binary image related information */
#ifdef CONFIG_ARCH_ARM32

struct elf_img_info {
	Elf32_Ehdr *header;
	Elf32_Shdr *sections;
	Elf32_Phdr *segments; /* program header */
	char **section_names;
	void *file_buffer;
	uint32_t segment_page_count;
};

#else

struct elf_img_info {
	Elf64_Ehdr *header;
	Elf64_Shdr *sections;
	Elf64_Phdr *segments; /* program header */
	char **section_names;
	void *file_buffer;
	uint64_t segment_page_count;
};

#endif



typedef struct elf_img_info elf_img_info_t;

uint8_t *elf_load_buffer(const char *filename);

void elf_load_sections(elf_img_info_t *elf_img_info);
void elf_load_segments(elf_img_info_t *elf_img_info);

void elf_clean_image(elf_img_info_t *elf_img_info);

#endif /* ELF_H */
