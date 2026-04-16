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

	return (uint64_t)fs_open((const char*)arg1);
}

DECLARE_SYSCALL(close) {
	ARGC_1;

	fs_close((struct fs_handle_t*)arg1);

	return SYSCALL_STS_OK;
}

DECLARE_SYSCALL(read) {
	ARGC_3;

	return (uint64_t)fs_read((struct fs_handle_t*)arg1, (void*)arg2, (size_t)arg3);
}
