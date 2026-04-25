/* syscall_vectors.h - ModulOS system call vectors */
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

/*
 * System call convention
 * rax: vector
 * rdi: arg1
 * rsi: arg2
 * rdx: arg3
 * r8 : arg4
 * r9 : arg5
 * r10: arg6
 *
 * arguments not guaranteed to be preserved
 *
 * rcx and r11 are clobbered
 * all other registers are preserved
 * return status stored in rax
 */

#define SYSCALL_STS_OK			0
#define SYSCALL_STS_FAIL		-1uLL

/*
 * rdi: exit status (int)
 * ret: (void)
 */
#define SYSCALL_EXIT				0

/*
 * rdi: path (const char*)
 * ret: handle (int)
 */
#define SYSCALL_OPEN				1

/*
 * rdi: handle (int)
 * ret: (void)
 */
#define SYSCALL_CLOSE				2

/*
 * rdi: handle (int)
 * rsi: buffer (char*)
 * rdx: count (size_t)
 * ret: bytes read (size_t)
 */
#define SYSCALL_READ				3

/*
 * rdi: handle (int)
 * rsi: buffer (char*)
 * rdx: count (size_t)
 * ret: bytes written (size_t)
 */
#define SYSCALL_WRITE				4

/*
 * rdi: size (long)
 * ret: mem region (void*)
 */
#define SYSCALL_ALLOC				5

#define SYSCALL_MAX					6


