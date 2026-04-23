/* syscall_dispatch.c - kernel system call dispatcher */
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

#include <core/syscall_dispatch.h>
#include <core/syscall.h>
#include <core/syscall_vectors.h>
#include <core/process.h>
#include <core/logging.h>
#include <core/fs.h>
#include <core/mm.h>
#include <core/paging.h>
#include <core/proc_data.h>
#include <core/process.h>

#include <lib/kmemset.h>

#define ARGC_0 \
	(void)arg1; \
	(void)arg2; \
	(void)arg3; \
	(void)arg4; \
	(void)arg5; \
	(void)arg6

#define ARGC_1 \
	(void)arg2; \
	(void)arg3; \
	(void)arg4; \
	(void)arg5; \
	(void)arg6

#define ARGC_2 \
	(void)arg3; \
	(void)arg4; \
	(void)arg5; \
	(void)arg6

#define ARGC_3 \
	(void)arg4; \
	(void)arg5; \
	(void)arg6

#define ARGC_4 \
	(void)arg5; \
	(void)arg6

#define ARGC_5 \
	(void)arg6

#define ARGC_6

DECLARE_SYSCALL(exit) {
	ARGC_1;

	//TODO: handle exit code
	(void)arg1;

	process_kill_current();
}

DECLARE_SYSCALL(open) {
	ARGC_0;

	//TODO
	return 1;
}

DECLARE_SYSCALL(close) {
	ARGC_0;

	//TODO
	return 1;
}

DECLARE_SYSCALL(read) {
	ARGC_3;

	struct pcb_t* pcb = proc_data_get()->current_process;
	return (uint64_t)fs_read(pcb->fd_table[arg1], (void*)arg2, (size_t)arg3);
}

DECLARE_SYSCALL(write) {
	ARGC_3;
	
	struct pcb_t* pcb = proc_data_get()->current_process;
	return (uint64_t)fs_write(pcb->fd_table[arg1], (void*)arg2, (size_t)arg3);
}

DECLARE_SYSCALL(alloc) {
	ARGC_1;

	if (arg1 % PAGE_SIZE_4K) {
		arg1 += PAGE_SIZE_4K - (arg1 % PAGE_SIZE_4K);
	}

	struct pcb_t* pcb = proc_data_get()->current_process;
	uint64_t paddr = mm_alloc_p(arg1);
	
	if (!paddr) {
		return 0;
	}

	uint64_t vaddr = pcb->mem_top;

	for (uint64_t i = 0; i < arg1; i += PAGE_SIZE_4K) {
		paging_map_proc(vaddr + i,
										paddr + i,
										PAGE_PRESENT | PAGE_RW | PAGE_US | PAGE_XD, PAGE_4K,
										(uint64_t*)proc_data_get()->current_process->cr3);
	}

	pcb->mem_top += arg1;

	return vaddr;
}
