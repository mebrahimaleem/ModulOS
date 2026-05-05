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
#include <core/time.h>

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
	ARGC_1;

	int fd;
	struct pcb_t* pcb = proc_data_get()->current_process;

	for (fd = 0; fd < MAX_FD; fd++) {
		if (!pcb->fd_table[fd]) {
			pcb->fd_table[fd] = fs_open((const char*)arg1);
			if (!pcb->fd_table[fd]) {
				return SYSCALL_STS_FAIL;
			}

			return (uint64_t)fd;
		}
	}

	return SYSCALL_STS_FAIL;
}

DECLARE_SYSCALL(close) {
	ARGC_1;

	struct pcb_t* pcb = proc_data_get()->current_process;
	struct fs_handle_t* handle = pcb->fd_table[arg1];

	if (!handle) {
		return SYSCALL_STS_FAIL;
	}

	fs_close(handle);
	pcb->fd_table[arg1] = 0;

	return SYSCALL_STS_OK;
}

DECLARE_SYSCALL(read) {
	ARGC_3;

	struct pcb_t* pcb = proc_data_get()->current_process;
	struct fs_handle_t* handle = pcb->fd_table[arg1];

	if (!handle) {
		return SYSCALL_STS_FAIL;
	}

	return fs_read(handle, (void*)arg2, (size_t)arg3);
}

DECLARE_SYSCALL(write) {
	ARGC_3;
	
	struct pcb_t* pcb = proc_data_get()->current_process;
	struct fs_handle_t* handle = pcb->fd_table[arg1];

	if (!handle) {
		return SYSCALL_STS_FAIL;
	}

	return fs_write(handle, (void*)arg2, (size_t)arg3);
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

DECLARE_SYSCALL(create) {
	ARGC_2;
	
	struct pcb_t* pcb = proc_data_get()->current_process;
	struct fs_handle_t* handle = pcb->fd_table[arg1];

	if (!handle) {
		return SYSCALL_STS_FAIL;
	}

	return fs_create(handle, (const char*)arg2) == FILE_OK;
}

DECLARE_SYSCALL(delete) {
	ARGC_1;

	struct pcb_t* pcb = proc_data_get()->current_process;
	struct fs_handle_t* handle = pcb->fd_table[arg1];

	if (!handle) {
		return SYSCALL_STS_FAIL;
	}

	return fs_delete(handle) == FILE_OK;
}

DECLARE_SYSCALL(open_dir) {
	ARGC_1;

	struct pcb_t* pcb = proc_data_get()->current_process;
	struct fs_handle_t* handle = pcb->fd_table[arg1];

	if (!handle) {
		return SYSCALL_STS_FAIL;
	}

	return (uint64_t)fs_open_dir(handle);
}

DECLARE_SYSCALL(read_dir) {
	ARGC_2;

	struct pcb_t* pcb = proc_data_get()->current_process;
	struct fs_handle_t* handle = pcb->fd_table[arg1];

	if (!handle) {
		return SYSCALL_STS_FAIL;
	}

	return fs_read_dir(handle, (struct dir_info_t*)arg2) == FILE_OK;
}

DECLARE_SYSCALL(close_dir) {
	ARGC_1;

	struct pcb_t* pcb = proc_data_get()->current_process;
	struct fs_handle_t* handle = pcb->fd_table[arg1];

	if (!handle) {
		return SYSCALL_STS_FAIL;
	}

	fs_close_dir(handle);

	return SYSCALL_STS_OK;
}

DECLARE_SYSCALL(seek) {
	ARGC_2;

	struct pcb_t* pcb = proc_data_get()->current_process;
	struct fs_handle_t* handle = pcb->fd_table[arg1];

	if (!handle) {
		return SYSCALL_STS_FAIL;
	}

	fs_seek(handle, (uint64_t)arg2);
	return fs_get_seek(handle);
}

DECLARE_SYSCALL(tell) {
	ARGC_1;

	struct pcb_t* pcb = proc_data_get()->current_process;
	struct fs_handle_t* handle = pcb->fd_table[arg1];

	if (!handle) {
		return SYSCALL_STS_FAIL;
	}

	return fs_get_seek(handle);
}

DECLARE_SYSCALL(create_dir) {
	ARGC_2;

	struct pcb_t* pcb = proc_data_get()->current_process;
	struct fs_handle_t* handle = pcb->fd_table[arg1];

	if (!handle) {
		return SYSCALL_STS_FAIL;
	}

	return fs_create_dir(handle, (const char*)arg2) == FILE_OK;
}

DECLARE_SYSCALL(delete_dir) {
	ARGC_1;

	struct pcb_t* pcb = proc_data_get()->current_process;
	struct fs_handle_t* handle = pcb->fd_table[arg1];

	if (!handle) {
		return SYSCALL_STS_FAIL;
	}

	return fs_delete_dir(handle) == FILE_OK;
}

DECLARE_SYSCALL(epoch_time) {
	ARGC_0;

	return time_since_init_ns();
}
