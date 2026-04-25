/* syscall.h - userland system call interface */
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

extern "C" uint64_t syscall_0(uint64_t __attribute((unused)),
													uint64_t __attribute__((unused)),
													uint64_t __attribute__((unused)),
													uint64_t v);

extern "C" uint64_t syscall_1(uint64_t arg1,
													uint64_t __attribute__((unused)),
													uint64_t __attribute__((unused)),
													uint64_t v);

extern "C" uint64_t syscall_2(uint64_t arg1,
													uint64_t arg2,
													uint64_t __attribute__((unused)),
													uint64_t v);

extern "C" uint64_t syscall_3(uint64_t arg1,
													uint64_t arg2,
													uint64_t arg3,
													uint64_t v);

extern "C" uint64_t syscall_4(uint64_t arg1,
													uint64_t arg2,
													uint64_t arg3,
													uint64_t v,
													uint64_t arg4);

extern "C" uint64_t syscall_5(uint64_t arg1,
													uint64_t arg2,
													uint64_t arg3,
													uint64_t v,
													uint64_t arg4,
													uint64_t arg5);

extern "C" uint64_t syscall_6(uint64_t arg1,
													uint64_t arg2,
													uint64_t arg3,
													uint64_t v,
													uint64_t arg4,
													uint64_t arg5,
													uint64_t arg6);
