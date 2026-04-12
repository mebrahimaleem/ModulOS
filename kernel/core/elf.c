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

#include <core/elf.h>
#include <core/fs.h>
#include <core/process.h>
#include <core/logging.h>
#include <core/alloc.h>
#include <core/paging.h>
#include <core/mm.h>
#include <core/gdt.h>
#include <core/syscall.h>
#include <core/cpu_instr.h>

#include <lib/kmemcmp.h>
#include <lib/kmemset.h>

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

#define EM_X86_64			62


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

#define INIT_USERLAND_RFL		0x200
#define INIT_USERLAND_RSP		0x800000000000
#define INIT_USERLAND_SB		0x7FFFFF800000
#define INIT_STACK_SIZE			PAGE_SIZE_4K * 8

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

static uint8_t check_valid(struct fs_handle_t* file, Elf64_Ehdr* header) {
	return 
			fs_seek(file, 0) != FILE_OK || /* file ok seek */
			fs_read(file, header, sizeof(*header)) != sizeof(*header) || /* file ok read */
			kmemcmp(elf_magic, &header->e_ident[EI_MAG0], sizeof(elf_magic)) || /* magic */
			header->e_ident[EI_CLASS] != ELFCLASS64 || /* 64 bit */
			header->e_ident[EI_DATA] != ELFDATA2LSB || /* little endian */
			header->e_ident[EI_OSABI] != ELFOSABI_SYSV || /* sys V */
			header->e_type != ET_EXEC || /* executable */
			header->e_machine != EM_X86_64; /* amd64 */
}

uint8_t elf_is_elf(struct fs_handle_t* file) {
	Elf64_Ehdr header;

	return check_valid(file, &header);
}

struct pcb_t* elf_load(struct fs_handle_t* file, uint64_t pid) {
	Elf64_Ehdr header;
	uint64_t stack_paddr, stack_vaddr, rsp;

	if (check_valid(file, &header)) {
		return 0;
	}

	if (process_create_guarded_stack(&stack_vaddr, &stack_paddr, &rsp)) {
		logging_log_error("Failed to allocate stack");
		return 0;
	}

	struct pcb_t* pcb = kmalloc(sizeof(struct pcb_t));

	pcb->rax =
		pcb->rbx =
		pcb->rcx =
		//pcb->rdx = (parameter passing)
		pcb->rbp =
		//pcb->rsi = (parameter passing)
		//pcb->rdi = (parameter passing)
		pcb->r8 =
		pcb->r9 =
		pcb->r10 =
		pcb->r11 =
		pcb->r12 =
		pcb->r13 =
		pcb->r14 =
		pcb->r15 = 0;


	pcb->saved_usr_rsp = INIT_USERLAND_RSP;

	pcb->init_k_rsp_paddr = stack_paddr;
	pcb->init_k_rsp_vaddr = stack_vaddr;
	pcb->rsp = rsp;

	pcb->k_rsp_lo = rsp & 0xFFFFFFFF;
	pcb->k_rsp_hi = rsp >> 32;

	pcb->rflags = INIT_USERLAND_RFL;

	pcb->rdi = header.e_entry; // rip
	pcb->rsi = INIT_USERLAND_RFL; // rflags
	pcb->rdx = pcb->saved_usr_rsp; // rsp

	pcb->rip = (uint64_t)syscall_return;
	pcb->cs = GDT_KERNEL_CS;
	pcb->ss = GDT_KERNEL_SS;

	pcb->sched_cntr = SCHED_READY;
	pcb->pid = pid;

	pcb->cr3 = paging_create_pml4();
	if (!pcb->cr3) {
		logging_log_error("Failed to create pml4 for process");
		kfree(pcb);
		return 0;
	}
	
	Elf64_Phdr pheader;

	Elf64_Off ph_off = header.e_phoff;
	Elf64_Off img_off;

	uint64_t paddr;
	uint16_t page_flags;

	uint64_t old_cr3 = cpu_get_cr3();

	cpu_set_cr3(pcb->cr3);

	for (img_off = INIT_USERLAND_RSP - INIT_STACK_SIZE; img_off < INIT_USERLAND_RSP; img_off += PAGE_SIZE_4K) {
		paddr = mm_alloc_p(PAGE_SIZE_4K);

		if (!paddr) {
			cpu_set_cr3(old_cr3);
			paging_free_userspace((uint64_t*)pcb->cr3);
			kfree(pcb);
			return 0;
		}

		paging_map_proc(img_off, paddr, PAGE_PRESENT | PAGE_RW | PAGE_XD, PAGE_4K, (uint64_t*)pcb->cr3);
		kmemset((void*)img_off, 0, PAGE_SIZE_4K);
	}

	for (Elf64_Half i = 0; i < header.e_phnum; i++) {
		fs_seek(file, ph_off);
		fs_read(file, &pheader, sizeof(pheader));

		if (pheader.p_type != PT_LOAD) {
			// TODO: support other pt types
			continue;
		}

		if (pheader.p_vaddr + pheader.p_memsz >= INIT_USERLAND_SB) {
			// image stack collision
			kfree(pcb);
			cpu_set_cr3(old_cr3);
			return 0;
		}

		if (pheader.p_vaddr % PAGE_SIZE_4K) {
			kfree(pcb);
			cpu_set_cr3(old_cr3);
			return 0;
		}

		page_flags = PAGE_PRESENT | PAGE_US;

		if (pheader.p_flags & PF_W) {
			page_flags |= PAGE_RW;
		}

		if (!(pheader.p_flags & PF_X)) {
			page_flags |= PAGE_XD;
		}

		for (img_off = 0; img_off < pheader.p_memsz; img_off += PAGE_SIZE_4K) {
			paddr = mm_alloc_p(PAGE_SIZE_4K);

			if (!paddr) {
				cpu_set_cr3(old_cr3);
				paging_free_userspace((uint64_t*)pcb->cr3);
				kfree(pcb);
				return 0;
			}

			paging_map_proc(pheader.p_vaddr + img_off, paddr, page_flags, PAGE_4K, (uint64_t*)pcb->cr3);
		}

		fs_seek(file, pheader.p_offset);
		fs_read(file, (void*)pheader.p_vaddr, pheader.p_filesz);
		kmemset((void*)(pheader.p_vaddr + pheader.p_memsz - pheader.p_filesz), 0, pheader.p_memsz - pheader.p_filesz);

		ph_off += header.e_phentsize;
	}

	cpu_set_cr3(old_cr3);

	return pcb;
}
