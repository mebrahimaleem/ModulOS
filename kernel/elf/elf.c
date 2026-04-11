/* elf.h - Executable and Linking Format loading implementation */
/* Copyright (C) 2026  Ebrahim Aleem
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <https://www.gnu.org/licenses/>
*/

#include <stdint.h>

#include <elf/elf.h>

#include <core/fs.h>
#include <core/process.h>
#include <core/logging.h>

#include <lib/kmemcmp.h>

#define EI_MAG0				0
#define EI_CLASS			4
#define EI_DATA				5
#define EI_VERSION		6
#define EI_OSABI			7
#define EI_ABIVERSION	8

#define EV_CURRENT		1

#define ELFCLASS64		2

#define ELFDATA2LSB		1

#define ELFOSABI_SYSV	0

#define ET_EXEC				2


#define SHT_NULL			0
#define SHT_PROGBITS	1
#define SHT_STRTAB		3
#define SHT_NOBITS		8

#define SHF_WRITE			1
#define SHF_ALLOC			2
#define SHF_EXECINSTR	4


#define PT_NULL				0
#define PT_LOAD				1
#define PT_PHDR				6

#define PF_X					1
#define PF_W					2
#define PF_R					4

typedef uint64_t	Elf64_Addr;
typedef uint64_t	Elf64_Off;
typedef uint16_t	Elf64_Half;
typedef uint32_t	Elf64_Word;
typedef int32_t		Elf64_Sword;
typedef uint64_t	Elf64_Xword;
typedef int64_t		Elf64_Sxword;

typedef struct {
	uint8_t e_ident[16];
	Elf64_Half e_type;
	Elf64_Half e_machine;
	Elf64_Word e_version;
	Elf64_Addr e_entry;
	Elf64_Off e_phoff;
	Elf64_Off e_shoff;
	Elf64_Word e_flags;
	Elf64_Half e_ehsize;
	Elf64_Half e_phentsize;
	Elf64_Half e_phnum;
	Elf64_Half e_shentsize;
	Elf64_Half e_shnum;
	Elf64_Half e_shstrndx;
} Elf64_Ehdr;

_Static_assert(sizeof(Elf64_Ehdr) == 64, "Bad ELF elf header size");

typedef struct {
	Elf64_Word sh_name;
	Elf64_Word sh_type;
	Elf64_Xword sh_flags;
	Elf64_Addr sh_addr;
	Elf64_Off sh_offset;
	Elf64_Xword sh_size;
	Elf64_Word sh_link;
	Elf64_Word sh_info;
	Elf64_Xword sh_addralign;
	Elf64_Xword sh_entsize;
} Elf64_Shdr;

_Static_assert(sizeof(Elf64_Shdr) == 64, "Bad ELF section header size");

typedef struct {
	Elf64_Word p_type;
	Elf64_Word p_flags;
	Elf64_Off p_offset;
	Elf64_Addr p_vaddr;
	Elf64_Addr v_paddr;
	Elf64_Xword p_filesz;
	Elf64_Xword p_memsz;
	Elf64_Xword p_align;
} Elf64_Phdr;

_Static_assert(sizeof(Elf64_Phdr) == 56, "Bad ELF program header size");

static const uint8_t elf_magic[4] = {0x7f, 'E', 'L', 'F'};

uint8_t elf_is_elf(struct fs_handle_t* file) {
	Elf64_Ehdr header;

	return
			fs_seek(file, 0) == FILE_OK && /* file ok seek */
			fs_read(file, &header, sizeof(header)) == sizeof(header) && /* file ok read */
			!kmemcmp(elf_magic, &header.e_ident[EI_MAG0], sizeof(elf_magic)) && /* magic */
			header.e_ident[EI_CLASS] == ELFCLASS64 && /* 64 bit */
			header.e_ident[EI_DATA] == ELFDATA2LSB && /* little endian */
			header.e_ident[EI_OSABI] == ELFOSABI_SYSV && /* sys V */
			header.e_type == ET_EXEC; /* executable */
}
