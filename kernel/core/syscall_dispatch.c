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

extern void syscall_dispatch(
		uint64_t vector,
		uint64_t argc,
		uint64_t* argv,
		uint64_t saved_rip,
		uint64_t saved_rsp,
		uint64_t saved_rflags) {

	switch (vector) {
		case SYSCALL_EXIT:
			if (argc != 1) {
				goto syscall_fail;
			}

			logging_log_debug("Process %lu terminated with %d", process_get_pid(), argv[0]);
			process_kill_current();
		default:
syscall_fail:
			syscall_return(saved_rip, saved_rflags, saved_rsp, ~0uLL);
	}
}
