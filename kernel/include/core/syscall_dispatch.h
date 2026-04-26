/* syscall_dispatch.h - kernel system call dispatching interface */
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

#ifndef KERNEL_CORE_SYSCALL_DISPATCH_H
#define KERNEL_CORE_SYSCALL_DISPATCH_H

#include <stdint.h>

#include <kernel/core/syscall_vectors.h>

#define DECLARE_SYSCALL(name) \
	uint64_t syscall_dispatch_##name ( \
			uint64_t arg1, \
			uint64_t arg2, \
			uint64_t arg3, \
			uint64_t arg6, \
			uint64_t arg4, \
			uint64_t arg5)

typedef uint64_t (*syscall_dispatch_t)(
		uint64_t arg1,
		uint64_t arg2,
		uint64_t arg3,
		uint64_t arg6,
		uint64_t arg4,
		uint64_t arg5);


extern syscall_dispatch_t syscall_handlers[SYSCALL_MAX];

extern DECLARE_SYSCALL(exit);
extern DECLARE_SYSCALL(open);
extern DECLARE_SYSCALL(close);
extern DECLARE_SYSCALL(read);
extern DECLARE_SYSCALL(write);
extern DECLARE_SYSCALL(alloc);
extern DECLARE_SYSCALL(create);

#endif /* KERNEL_CORE_SYSCALL_DISPATCH_H */
