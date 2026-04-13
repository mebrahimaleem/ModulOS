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

extern void syscall_dispatch(
		uint64_t vector,
		uint64_t argc,
		uint64_t* argv,
		uint64_t saved_rip,
		uint64_t saved_rsp,
		uint64_t saved_rflags) __attribute__((noreturn));

#endif /* KERNEL_CORE_SYSCALL_DISPATCH_H */
