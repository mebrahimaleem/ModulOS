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
#include <lib/array_list.h>

#define ARGC_6 \
	(void)rbp; \
	(void)rcx; \
	(void)r11;

#define ARGC_5 \
	ARGC_6 \
	(void)arg6;

#define ARGC_4 \
	ARGC_5 \
	(void)arg5;

#define ARGC_3 \
	ARGC_4 \
	(void)arg4;

#define ARGC_2 \
	ARGC_3 \
	(void)arg3;

#define ARGC_1 \
	ARGC_2 \
	(void)arg2;

#define ARGC_0 \
	ARGC_1 \
	(void)arg1;

#define USERLAND_AT_FDCWD -100

DECLARE_SYSCALL(exit) {
	ARGC_1;

	struct pcb_t* pcb = proc_data_get()->current_process;
	pcb->exit_code = arg1;

	process_kill_current();
}

DECLARE_SYSCALL(openat) {
	ARGC_4;

	struct pcb_t* pcb = proc_data_get()->current_process;
	struct fs_handle_t* at;

	if ((int32_t)arg3 == USERLAND_AT_FDCWD) {
		at = pcb->wd;
	}
	else {
		at = array_list_get(pcb->fd_table, arg1);
	}

	if (!at) {
		return SYSCALL_STS_FAIL;
	}

	struct fs_handle_t* handle = fs_openat((const char*)arg1, (uint32_t)arg2, at, (uint32_t)arg4);
	if (!handle) {
		return SYSCALL_STS_FAIL;
	}

	return array_list_push(pcb->fd_table, handle);
}

DECLARE_SYSCALL(close) {
	ARGC_1;

	struct pcb_t* pcb = proc_data_get()->current_process;
	struct fs_handle_t* handle = array_list_get(pcb->fd_table, arg1);

	if (!handle) {
		return SYSCALL_STS_FAIL;
	}

	fs_close(handle);
	array_list_remove(pcb->fd_table, arg1);

	return SYSCALL_STS_OK;
}

DECLARE_SYSCALL(read) {
	ARGC_3;

	struct pcb_t* pcb = proc_data_get()->current_process;
	struct fs_handle_t* handle = array_list_get(pcb->fd_table, arg1);

	if (!handle) {
		return SYSCALL_STS_FAIL;
	}

	return fs_read(handle, (void*)arg2, (size_t)arg3);
}

DECLARE_SYSCALL(write) {
	ARGC_3;
	
	struct pcb_t* pcb = proc_data_get()->current_process;
	struct fs_handle_t* handle = array_list_get(pcb->fd_table, arg1);

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
		return SYSCALL_STS_FAIL;
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

DECLARE_SYSCALL(fork) {
	ARGC_0;

	uint64_t pid = process_fork(r11, rcx, rbp);

	if (pid == -1uLL) {
		return SYSCALL_STS_FAIL;
	}

	return pid;
}

DECLARE_SYSCALL(execve) {
	ARGC_3;

	(void)arg1;
	(void)arg2;
	(void)arg3;

	return SYSCALL_STS_FAIL;
}

DECLARE_SYSCALL(open_dir) {
	ARGC_1;

	struct pcb_t* pcb = proc_data_get()->current_process;
	struct fs_handle_t* handle = array_list_get(pcb->fd_table, arg1);

	if (!handle) {
		return SYSCALL_STS_FAIL;
	}

	return (uint64_t)fs_open_dir(handle);
}

DECLARE_SYSCALL(read_dir) {
	ARGC_3;

	(void)arg1;
	(void)arg2;
	(void)arg3;

	//TODO
	return SYSCALL_STS_FAIL;
}

DECLARE_SYSCALL(truncate) {
	ARGC_2;

	struct pcb_t* pcb = proc_data_get()->current_process;
	struct fs_handle_t* handle = array_list_get(pcb->fd_table, arg1);

	if (!handle) {
		return SYSCALL_STS_FAIL;
	}

	return (fs_truncate(handle, (size_t)arg2) == FILE_OK) ? SYSCALL_STS_OK : SYSCALL_STS_FAIL;
}

DECLARE_SYSCALL(seek) {
	ARGC_2;

	struct pcb_t* pcb = proc_data_get()->current_process;
	struct fs_handle_t* handle = array_list_get(pcb->fd_table, arg1);

	if (!handle) {
		return SYSCALL_STS_FAIL;
	}

	fs_seek(handle, (uint64_t)arg2);
	return fs_get_seek(handle);
}

DECLARE_SYSCALL(tell) {
	ARGC_1;

	struct pcb_t* pcb = proc_data_get()->current_process;
	struct fs_handle_t* handle = array_list_get(pcb->fd_table, arg1);

	if (!handle) {
		return SYSCALL_STS_FAIL;
	}

	return fs_get_seek(handle);
}

DECLARE_SYSCALL(create_dir) {
	ARGC_1;

	struct pcb_t* pcb = proc_data_get()->current_process;
	struct fs_handle_t* handle = array_list_get(pcb->fd_table, arg1);

	if (!handle) {
		return SYSCALL_STS_FAIL;
	}

	return (fs_create_dir(handle) == FILE_OK) ? SYSCALL_STS_OK : SYSCALL_STS_FAIL;
}

DECLARE_SYSCALL(delete_dir) {
	ARGC_1;

	struct pcb_t* pcb = proc_data_get()->current_process;
	struct fs_handle_t* handle = array_list_get(pcb->fd_table, arg1);

	if (!handle) {
		return SYSCALL_STS_FAIL;
	}

	return (fs_delete_dir(handle) == FILE_OK) ? SYSCALL_STS_OK : SYSCALL_STS_FAIL;
}

DECLARE_SYSCALL(epoch_time) {
	ARGC_0;

	return time_since_init_ns();
}

DECLARE_SYSCALL(is_a_tty) {
	ARGC_1;

	struct pcb_t* pcb = proc_data_get()->current_process;
	struct fs_handle_t* handle = array_list_get(pcb->fd_table, arg1);

	if (!handle) {
		return SYSCALL_STS_FAIL;
	}

	return fs_is_interactive(handle);
}

DECLARE_SYSCALL(gcwd) {
	ARGC_2;

	struct pcb_t* pcb = proc_data_get()->current_process;

	fs_path(pcb->wd, (size_t)arg2, (char*)arg1);

	return SYSCALL_STS_FAIL;
}

DECLARE_SYSCALL(ccwd) {
	ARGC_1;

	struct pcb_t* pcb = proc_data_get()->current_process;
	struct fs_handle_t* handle = array_list_get(pcb->fd_table, arg1);

	if (!handle) {
		return SYSCALL_STS_FAIL;
	}

	pcb->wd = handle;

	return SYSCALL_STS_OK;
}

DECLARE_SYSCALL(link) {
	ARGC_2;

	struct pcb_t* pcb = proc_data_get()->current_process;
	struct fs_handle_t* handle1 = array_list_get(pcb->fd_table, arg1);

	if (!handle1) {
		return SYSCALL_STS_FAIL;
	}

	struct fs_handle_t* handle2 = array_list_get(pcb->fd_table, arg2);

	if (!handle2) {
		return SYSCALL_STS_FAIL;
	}

	return (fs_link(handle1, handle2) == FILE_OK) ? SYSCALL_STS_OK : SYSCALL_STS_FAIL;
}

DECLARE_SYSCALL(unlink) {
	ARGC_1;

	struct pcb_t* pcb = proc_data_get()->current_process;
	struct fs_handle_t* handle = array_list_get(pcb->fd_table, arg1);

	if (!handle) {
		return SYSCALL_STS_FAIL;
	}

	return (fs_unlink(handle) == FILE_OK) ? SYSCALL_STS_OK : SYSCALL_STS_FAIL;
}

DECLARE_SYSCALL(stat) {
	ARGC_2;

	struct pcb_t* pcb = proc_data_get()->current_process;
	struct fs_handle_t* handle = array_list_get(pcb->fd_table, arg1);

	if (!handle) {
		return SYSCALL_STS_FAIL;
	}

	struct userland_stat_t {
		size_t st_size;
	};

	struct file_info_t info;
	if (fs_stat(handle, &info) != FILE_OK) {
		return SYSCALL_STS_FAIL;
	}

	struct userland_stat_t* u_stat = (struct userland_stat_t*)arg2;
	u_stat->st_size = info.size;

	return SYSCALL_STS_OK;
}

DECLARE_SYSCALL(getpid) {
	ARGC_0;

	struct pcb_t* pcb = proc_data_get()->current_process;

	return pcb->pid;
}
