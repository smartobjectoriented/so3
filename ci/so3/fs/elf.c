/*
 * Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
 * Copyright (C) 2016-2017 Xavier Ruppen <xavier.ruppen@heig-vd.ch>
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


#if 0
#define DEBUG
#endif

#include <common.h>
#include <heap.h>
#include <vfs.h>
#include <process.h>
#include <elf.h>
#include <sizes.h>
#include <types.h>
#include <string.h>

/* Load the file in a buffer allocated in the kernel heap */
uint8_t *elf_load_buffer(const char *filename)
{
	int fd;
	uint8_t *buffer;
	struct stat st;

	/* open and read file */
	fd = do_open(filename, O_RDONLY);

	if (fd < 0)
		return NULL;

	if (do_stat(filename, &st))
		return NULL;

	if (!st.st_size)
		return NULL;

	buffer = malloc(st.st_size);
	if (!buffer) {
		printk("%s: failed to allocate memory\n", __func__);
		return NULL;
	}

	do_read(fd, buffer, st.st_size);

	do_close(fd);

	return buffer;
}

/*
 * Clean all allocated data related to the ELF image.
 */
void elf_clean_image(elf_img_info_t *elf_img_info) {
	int i;

	free(elf_img_info->file_buffer);

	/* Release the heap allocated to store the binary image components */
	for (i = 0; i < elf_img_info->header->e_shnum; i++)
		free(elf_img_info->section_names[i]);

	free(elf_img_info->section_names);
	free(elf_img_info->sections);
	free(elf_img_info->header);
	free(elf_img_info->segments);
}

void elf_load_sections(elf_img_info_t *elf_img_info)
{
	size_t i;
	size_t section_name_len;
	size_t section_name_offset;

	/* header */
#ifdef CONFIG_ARCH_ARM32
	elf_img_info->header = (struct elf32_hdr *) malloc(sizeof(struct elf32_hdr));
#else
	elf_img_info->header = (struct elf64_hdr *) malloc(sizeof(struct elf64_hdr));
#endif
	if (!elf_img_info->header) {
		printk("%s: failed to allocate memory\n", __func__);
		kernel_panic();
	}

#ifdef CONFIG_ARCH_ARM32
	memcpy(elf_img_info->header, elf_img_info->file_buffer, sizeof(struct elf32_hdr));
#else
	memcpy(elf_img_info->header, elf_img_info->file_buffer, sizeof(struct elf64_hdr));
#endif

	DBG("Magic: 0x%02x%c%c%c\n", elf_img_info->header->e_ident[EI_MAG0],
	                             elf_img_info->header->e_ident[EI_MAG1],
	                             elf_img_info->header->e_ident[EI_MAG2],
	                             elf_img_info->header->e_ident[EI_MAG3]);
	DBG("%d sections\n", elf_img_info->header->e_shnum);
	DBG("section table is at offset 0x%08x (%d bytes/section)\n", elf_img_info->header->e_shoff, elf_img_info->header->e_shentsize);

#ifdef CONFIG_ARCH_ARM32
	DBG("sizeof(struct elf32_shdr): %d bytes\n", sizeof(struct elf32_shdr));
#else
	DBG("sizeof(struct elf64_shdr): %d bytes\n", sizeof(struct elf64_shdr));
#endif

	/* sections */
#ifdef CONFIG_ARCH_ARM32
	elf_img_info->sections = (struct elf32_shdr *) malloc(elf_img_info->header->e_shnum * sizeof(struct elf32_shdr));
#else
	elf_img_info->sections = (struct elf64_shdr *) malloc(elf_img_info->header->e_shnum * sizeof(struct elf64_shdr));
#endif

	if (!elf_img_info->sections) {
		printk("%s: failed to allocate memory\n", __func__);
		kernel_panic();
	}

	elf_img_info->section_names = (char **) malloc(elf_img_info->header->e_shnum * sizeof(char *));
	if (!elf_img_info->section_names) {
		printk("%s: failed to allocate memory\n", __func__);
		kernel_panic();
	}

	for (i = 0; i < elf_img_info->header->e_shnum; i++)
		memcpy(elf_img_info->sections + i, elf_img_info->file_buffer + elf_img_info->header->e_shoff + i*elf_img_info->header->e_shentsize, sizeof(struct elf32_shdr));

	/* Section names */
	for (i = 0; i < elf_img_info->header->e_shnum; i++) {
		section_name_offset = elf_img_info->sections[elf_img_info->header->e_shstrndx].sh_offset + elf_img_info->sections[i].sh_name;
		section_name_len = strlen((const char *)(elf_img_info->file_buffer + section_name_offset));

		elf_img_info->section_names[i] = (char *) malloc(section_name_len + 1);
		if (!elf_img_info->section_names[i]) {
			printk("%s: failed to allocate memory\n", __func__);
			kernel_panic();
		}

		strcpy(elf_img_info->section_names[i], (char *)(elf_img_info->file_buffer + section_name_offset));

		DBG("[0x%08x] section name: %s\t(%d bytes)\n", section_name_offset,
		                                               elf_img_info->section_names[i],
		                                               section_name_len);
	}

	for (i = 0; i < elf_img_info->header->e_shnum; i++) {
		DBG("\t[0x%08x] %s loads at 0x%08x (%d bytes) - flags: 0x%08x\n",
	            elf_img_info->sections[i].sh_offset,
		    elf_img_info->section_names[i],
		    elf_img_info->sections[i].sh_addr,
		    elf_img_info->sections[i].sh_size,
		    elf_img_info->sections[i].sh_flags);
	}
}

void elf_load_segments(elf_img_info_t *elf_img_info)
{
	size_t i;

	/* Segments */
#ifdef CONFIG_ARCH_ARM32
	elf_img_info->segments = (struct elf32_phdr *) malloc(sizeof(struct elf32_phdr) * elf_img_info->header->e_phnum);
#else
	elf_img_info->segments = (struct elf64_phdr *) malloc(sizeof(struct elf64_phdr) * elf_img_info->header->e_phnum);
#endif

	if (!elf_img_info->segments) {
		printk("%s: failed to allocate memory\n", __func__);
		kernel_panic();
	}

	DBG("%d segments\n", elf_img_info->header->e_phnum);
	DBG("segment table is at offset 0x%08x (%d bytes/section)\n", elf_img_info->header->e_phoff, elf_img_info->header->e_phentsize);
#ifdef CONFIG_ARCH_ARM32
	DBG("sizeof(struct elf32_phdr): %d bytes\n", sizeof(struct elf32_phdr));
#else
	DBG("sizeof(struct elf64_phdr): %d bytes\n", sizeof(struct elf64_phdr));
#endif

	elf_img_info->segment_page_count = 0;
	for (i = 0; i < elf_img_info->header->e_phnum; i++) {
#ifdef CONFIG_ARCH_ARM32
		memcpy(elf_img_info->segments + i, elf_img_info->file_buffer + elf_img_info->header->e_phoff + i*elf_img_info->header->e_phentsize, sizeof(struct elf32_phdr));
#else
		memcpy(elf_img_info->segments + i, elf_img_info->file_buffer + elf_img_info->header->e_phoff + i*elf_img_info->header->e_phentsize, sizeof(struct elf64_phdr));
#endif

		if (elf_img_info->segments[i].p_type == PT_LOAD)
			elf_img_info->segment_page_count += (elf_img_info->segments[i].p_memsz >> PAGE_SHIFT) + 1;
	}
	DBG("segments use %d virtual pages\n", elf_img_info->segment_page_count);

	for (i = 0; i < elf_img_info->header->e_phnum; i++) {
		DBG("[0x%08x] vaddr: 0x%08x; paddr: 0x%08x; filesize: 0x%08x; memsize: 0x%08x flags: 0x%08x\n",
		    elf_img_info->segments[i].p_offset,
		    elf_img_info->segments[i].p_vaddr,
		    elf_img_info->segments[i].p_paddr,
		    elf_img_info->segments[i].p_filesz,
		    elf_img_info->segments[i].p_memsz,
		    elf_img_info->segments[i].p_flags);
	}
}
