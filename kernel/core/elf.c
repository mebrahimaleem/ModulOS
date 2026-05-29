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
#include <core/proc_data.h>
#include <core/time.h>

#include <lib/kmemcmp.h>
#include <lib/kmemset.h>
#include <lib/kmemcpy.h>
#include <lib/kstrlen.h>
#include <lib/kstrcpy.h>
#include <lib/array_list.h>

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

// auxv
#define AT_NULL					0
#define AT_IGNORE				1
#define AT_EXECFD				2
#define AT_PHDR					3
#define AT_PHENT				4
#define AT_PHNUM				5
#define AT_PAGESZ				6
#define AT_BASE					7
#define AT_FLAGS				8
#define AT_ENTRY				9
#define AT_NOTELF				10
#define AT_UID					11
#define AT_EUID					12
#define AT_GID					13
#define AT_EGID					14

// linux vector entries
#define AT_PLATFORM 		15
#define AT_HWCAP 				16
#define AT_CLKTCK 			17
#define AT_FPUCW 				18
#define AT_SECURE 			23
#define AT_RANDOM 			25
#define AT_HWCAP2 			26
#define AT_HWCAP3 			29
#define AT_HWCAP4 			30
#define AT_EXECFN 			31
#define AT_SYSINFO_EHDR 33

#define FD_INIT_SIZE		4
#define FD_GROWTH				8

enum at_index_t {
	AT_INDEX_PHDR,
	AT_INDEX_PHENT,
	AT_INDEX_PHNUM,
	AT_INDEX_PAGESZ,
	AT_INDEX_UID,
	AT_INDEX_EUID,
	AT_INDEX_GID,
	AT_INDEX_EGID,
	AT_INDEX_SECURE,
	AT_INDEX_RANDOM,
	AT_INDEX_CLKTCK,

	AT_INDEX_NULL
};

typedef struct {
	int a_type;
	union {
		long a_val;
		void *a_ptr;
		void (*a_fnc)();
	} a_un;
} auxv_t;

static auxv_t default_auxv[] = {
	[AT_INDEX_PHDR] = {.a_type = AT_PHDR},
	[AT_INDEX_PHENT] = {.a_type = AT_PHENT},
	[AT_INDEX_PHNUM] = {.a_type = AT_PHNUM},
	[AT_INDEX_PAGESZ] = {.a_type = AT_PAGESZ, .a_un.a_val = PAGE_SIZE_4K},
	[AT_INDEX_UID] = {.a_type = AT_UID, .a_un.a_val = 0},	
	[AT_INDEX_EUID] = {.a_type = AT_EUID, .a_un.a_val = 0},	
	[AT_INDEX_GID] = {.a_type = AT_GID, .a_un.a_val = 0},	
	[AT_INDEX_EGID] = {.a_type = AT_EGID, .a_un.a_val = 0},	
	[AT_INDEX_SECURE] = {.a_type = AT_SECURE, .a_un.a_val = 0},
	[AT_INDEX_RANDOM] = {.a_type = AT_RANDOM},
	[AT_INDEX_CLKTCK] = {.a_type = AT_CLKTCK, .a_un.a_val = 100},

	[AT_INDEX_NULL] = {.a_type = AT_NULL} // extra qword is ignored
};

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
	Elf64_Addr p_paddr;
	Elf64_Xword p_filesz;
	Elf64_Xword p_memsz;
	Elf64_Xword p_align;
} Elf64_Phdr;

_Static_assert(sizeof(Elf64_Phdr) == 56, "Bad ELF program header size");

struct mem_reg_t {
	uint64_t base;
	uint64_t top;
	uint64_t perms;
	struct mem_reg_t* next;
};

static const uint8_t elf_magic[4] = {0x7f, 'E', 'L', 'F'};

static uint8_t check_valid(struct fs_handle_t* file, Elf64_Ehdr* header) {
	if (
			fs_seek(file, 0) != FILE_OK || /* file ok seek */
			fs_read(file, header, sizeof(*header)) != sizeof(*header) || /* file ok read */
			kmemcmp(elf_magic, &header->e_ident[EI_MAG0], sizeof(elf_magic)) || /* magic */
			header->e_ident[EI_CLASS] != ELFCLASS64 || /* 64 bit */
			header->e_ident[EI_DATA] != ELFDATA2LSB || /* little endian */
			//header->e_ident[EI_OSABI] != ELFOSABI_SYSV || /* sys V */ // unreliable
			header->e_type != ET_EXEC || /* executable */
			header->e_machine != EM_X86_64 /* amd64 */
		) {
		return 1;
	}

	return 0;
}

uint8_t elf_is_elf(struct fs_handle_t* file) {
	Elf64_Ehdr header;

	return check_valid(file, &header);
}

struct pcb_t* elf_load(struct fs_handle_t* file, uint64_t pid, const char* invoker, const char* env) {
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


	pcb->init_k_rsp_paddr = stack_paddr;
	pcb->init_k_rsp_vaddr = stack_vaddr;
	pcb->rsp = rsp;

	pcb->k_rsp_lo = rsp & 0xFFFFFFFF;
	pcb->k_rsp_hi = rsp >> 32;

	pcb->fsbase = 0;

	pcb->rflags = INIT_USERLAND_RFL;

	pcb->rdi = header.e_entry; // rip
	pcb->rsi = INIT_USERLAND_RFL; // rflags

	pcb->rip = (uint64_t)syscall_return;
	pcb->cs = GDT_KERNEL_CS;
	pcb->ss = GDT_KERNEL_SS;

	pcb->sched_cntr = SCHED_READY;
	pcb->pid = pid;

	cpu_restore_fx(pcb->fxdata);

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
	uint64_t page_flags;

	struct mem_reg_t* mem_regs = 0;
	struct mem_reg_t* j;
	struct mem_reg_t* temp;

	uint64_t memtop = 0;

	// first pass to determine memory layout
	for (Elf64_Half i = 0; i < header.e_phnum; i++) {
		fs_seek(file, ph_off);
		fs_read(file, &pheader, sizeof(pheader));
		ph_off += header.e_phentsize;

		if (pheader.p_type != PT_LOAD) {
			// TODO: support other pt types
			continue;
		}

		if (pheader.p_vaddr + pheader.p_memsz >= INIT_USERLAND_SB) {
			// stack collision
			kfree(pcb);
			pcb = 0;
			return 0;
		}

		page_flags = PAGE_PRESENT | PAGE_US;

		if (pheader.p_flags & PF_W) {
			page_flags |= PAGE_RW;
		}

		if (!(pheader.p_flags & PF_X)) {
			page_flags |= PAGE_XD;
		}

		uint64_t base = pheader.p_vaddr;
		uint64_t top = pheader.p_vaddr + pheader.p_memsz;

		base -= base % PAGE_SIZE_4K;

		if (top % PAGE_SIZE_4K) {
			top += PAGE_SIZE_4K - (top % PAGE_SIZE_4K);
		}

		struct mem_reg_t** insert = &mem_regs;
		for (j = mem_regs; j; j = j->next) {
			if (base < j->base) {
				break;
			}

			insert = &j->next;
		}

		// insert in order
		
		j = (*insert);
		*insert = kmalloc(sizeof(struct mem_reg_t));
		(*insert)->base = base;
		(*insert)->top = top;
		(*insert)->perms = page_flags;
		(*insert)->next = j;
	}

	// detect overlaping memory
	for (j = mem_regs; j && j->next; j = j->next) {
		if (j->top > j->next->base) {
			if (j->perms != j->next->perms) {
				// overlap
				kfree(pcb);
				return 0;
			}

			// merge
			j->top = j->top > j->next->top ? j->top : j->next->top;
			temp = j->next;
			j->next = j->next->next;
			kfree(temp);
		}
	}

	uint64_t old_cr3 = proc_data_get()->current_process->cr3;
	proc_data_get()->current_process->cr3 = pcb->cr3;

	cpu_set_cr3(pcb->cr3);

	// map in memory for copying
	for (j = mem_regs; j; j = j->next) {
		if (j->top > memtop) {
			memtop = j->top;
		}

		for (uint64_t off = 0; off < j->top - j->base; off += PAGE_SIZE_4K) {
			paddr = mm_alloc_p(PAGE_SIZE_4K);

			if (!paddr) {
				paging_free_userspace((uint64_t*)pcb->cr3);
				kfree(pcb);
				pcb = 0;
				goto restore_cr3;
			}

			paging_map_proc(j->base + off, paddr, PAGE_PRESENT | PAGE_RW, PAGE_4K, (uint64_t*)pcb->cr3);
		}
	}


	memtop += PAGE_SIZE_4K;
	uint64_t pheaders_base = memtop;

	// map in pheaders
	size_t pheader_total = header.e_phentsize & header.e_phnum;
	for (size_t off = 0; off < pheader_total; off += PAGE_SIZE_4K) {
		paddr = mm_alloc_p(PAGE_SIZE_4K);

		if (!paddr) {
			paging_free_userspace((uint64_t*)pcb->cr3);
			kfree(pcb);
			pcb = 0;
			goto restore_cr3;
		}

		paging_map_proc(pheaders_base + off, paddr, PAGE_PRESENT | PAGE_RW | PAGE_US | PAGE_XD, PAGE_4K, (uint64_t*)pcb->cr3);
		memtop += PAGE_SIZE_4K;
	}

	memtop += PAGE_SIZE_4K;
	pcb->mem_top = memtop;

	ph_off = 0;
	// copy pheaders
	for (Elf64_Half i = 0; i < header.e_phnum; i++) {
		fs_seek(file, ph_off + header.e_phoff);
		fs_read(file, (void*)(pheaders_base + ph_off), sizeof(pheader));
		ph_off += header.e_phentsize;
	}

	// map in stack
	for (img_off = INIT_USERLAND_RSP - INIT_STACK_SIZE; img_off < INIT_USERLAND_RSP; img_off += PAGE_SIZE_4K) {
		paddr = mm_alloc_p(PAGE_SIZE_4K);

		if (!paddr) {
			paging_free_userspace((uint64_t*)pcb->cr3);
			kfree(pcb);
			pcb = 0;
			goto restore_cr3;
		}

		paging_map_proc(img_off, paddr, PAGE_PRESENT | PAGE_RW | PAGE_US | PAGE_XD, PAGE_4K, (uint64_t*)pcb->cr3);
		kmemset((void*)img_off, 0, PAGE_SIZE_4K);
	}

	// create auxv
	uint64_t invoke_addr = INIT_USERLAND_RSP - kstrlen(invoker) - 1;
	uint64_t env_addr = invoke_addr - kstrlen(env) - 1;
	uint64_t random_addr = env_addr - 16;
	auxv_t* auxv_addr = (auxv_t*)(random_addr - sizeof(default_auxv));

	kstrcpy((char*)invoke_addr, invoker);
	kstrcpy((char*)env_addr, env);
	kmemcpy(auxv_addr, default_auxv, sizeof(default_auxv));

	// leave upper bytes untouched for "randomness"
	*(volatile uint64_t*)random_addr = time_since_init_fs(); //TODO: better randomization

	auxv_addr[AT_INDEX_PHDR].a_un.a_val = (int64_t)pheaders_base;
	auxv_addr[AT_INDEX_PHENT].a_un.a_val = header.e_phentsize;
	auxv_addr[AT_INDEX_PHNUM].a_un.a_val = header.e_phnum;
	auxv_addr[AT_INDEX_RANDOM].a_un.a_val = (int64_t)random_addr;

	uint64_t* stack_builder = (uint64_t*)auxv_addr;
	stack_builder--;
	*stack_builder = 0; // env end
	
	char* inf_chars;
	for (inf_chars = (char*)(env_addr + kstrlen(env)); inf_chars >= (char*)env_addr; inf_chars--) {
		if (*inf_chars == ' ') {
			stack_builder--;
			*stack_builder = (uint64_t)inf_chars + 1;
			*inf_chars = 0;
		}
	}

	stack_builder--;
	*stack_builder = (uint64_t)inf_chars + 1;

	stack_builder--;
	*stack_builder = 0; // env start
	
	uint64_t argc = 1;

	for (inf_chars = (char*)(invoke_addr + kstrlen(invoker)); inf_chars >= (char*)invoke_addr; inf_chars--) {
		if (*inf_chars == ' ') {
			stack_builder--;
			*stack_builder = (uint64_t)inf_chars + 1;
			*inf_chars = 0;
			argc++;
		}
	}

	stack_builder--;
	*stack_builder = (uint64_t)inf_chars + 1;

	stack_builder--;
	*stack_builder = argc;

	pcb->rdx = (uint64_t)stack_builder; // rsp

	// copy in executable
	ph_off = header.e_phoff;
	for (Elf64_Half i = 0; i < header.e_phnum; i++) {
		fs_seek(file, ph_off);
		fs_read(file, &pheader, sizeof(pheader));
		ph_off += header.e_phentsize;

		if (pheader.p_type != PT_LOAD) {
			// TODO: support other pt types
			continue;
		}

		fs_seek(file, pheader.p_offset);
		fs_read(file, (void*)pheader.p_vaddr, pheader.p_filesz);
		kmemset((void*)(pheader.p_vaddr + pheader.p_filesz), 0, pheader.p_memsz - pheader.p_filesz);
	}

	// update page permissions
	for (j = mem_regs; j; j = j->next) {
		for (uint64_t off = 0; off < j->top - j->base; off += PAGE_SIZE_4K) {
			paging_update_perms(j->base + off, j->perms, PAGE_4K, (uint64_t*)pcb->cr3);
		}
	}

	pcb->fd_table = array_list_alloc(FD_INIT_SIZE, FD_GROWTH, 0);
	pcb->wd = fs_open("/", FILE_FLAGS_READ | FILE_FLAGS_WRITE);
#ifdef SERIAL
	array_list_push(pcb->fd_table, fs_open("/dev/ttyS0", FILE_FLAGS_READ));
	array_list_push(pcb->fd_table, fs_open("/dev/ttyS0", FILE_FLAGS_WRITE));
	array_list_push(pcb->fd_table, fs_open("/dev/ttyS0", FILE_FLAGS_WRITE));
#endif /* SERIAL */

restore_cr3:

	while (mem_regs) {
		temp = mem_regs->next;
		kfree(mem_regs);
		mem_regs = temp;
	}

	proc_data_get()->current_process->cr3 = old_cr3;
	cpu_set_cr3(old_cr3);

	return pcb;
}
