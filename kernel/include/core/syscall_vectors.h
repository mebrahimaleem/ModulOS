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

/*
 * rdi: handle (int)
 * rsi: name (const char*)
 * ret: success (int)
 */
#define SYSCALL_CREATE			6

/*
 * rdi: handle (int)
 * ret: success (int)
 */
#define SYSCALL_DELETE			7

/*
 * rdi: handle (int)
 * ret handle (void*)
 */
#define SYSCALL_OPEN_DIR		8

/*
 * rdi: handle (void*)
 * rsi: info buffer (void*)
 * ret: success (int)
 */
#define SYSCALL_READ_DIR		9

/*
 * rdi: handle (void*)
 * ret: (void)
 */
#define SYSCALL_CLOSE_DIR		10

/*
 * rdi: handle (int)
 * rsi: seek (long int)
 * ret: new seek (long int)
 */
#define SYSCALL_SEEK				11

/*
 * rdi: handle (int)
 * ret: seek (long int)
 */
#define SYSCALL_TELL				12

/*
 * rdi: handle (int)
 * rsi: name (const char*)
 * ret: success (int)
 */
#define SYSCALL_CREATE_DIR	13

/*
 * rdi: handle (int)
 * ret: success (int)
 */
#define SYSCALL_DELETE_DIR	14

/*
 * ret: time since epoch in nanos (long int)
 */
#define SYSCALL_EPOCH_TIME		15

#define SYSCALL_MAX					16


