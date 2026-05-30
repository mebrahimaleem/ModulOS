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
 * rsi: flags (uint8_t)
 * rdx: at (int)
 * r8 : mode (int)
 * ret: handle (int)
 */
#define SYSCALL_OPENAT			1

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
 * ret: child pid (pid_t)
 */
#define SYSCALL_FORK				6

/*
 * rdi: path (const char*)
 * rsi: argv (const char*)
 * rdx: envp (const char*)
 * ret: success (int)
 */
#define SYSCALL_EXECVE			7

/*
 * rdi: handle (int)
 * ret handle (int)
 */
#define SYSCALL_OPEN_DIR		8

/*
 * rdi: handle (int)
 * rsi: info buffer (void*)
 * rdx: max_size (size_t)
 * ret: size (size_t)
 */
#define SYSCALL_READ_DIR		9

/*
 * rdi: handle (int)
 * rsi: size (size_t)
 * ret: success (int)
 */
#define SYSCALL_TRUNCATE		10

/*
 * rdi: handle (int)
 * rsi: seek (long int)
 * ret: new seek (long int)
 */
#define SYSCALL_SEEK				11

/*
 * rdi: handle
 * ret: seek (long int)
 */
#define SYSCALL_TELL				12

/*
 * rdi: handle (int)
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
#define SYSCALL_EPOCH_TIME	15

/*
 * rdi: handle (int)
 * ret: 1 on tty. 0 otherwise (int)
 */
#define SYSCALL_IS_A_TTY		16

/*
 * rdi: buffer (char*)
 * rsi: max len (size_t)
 * ret: success (int)
 */
#define SYSCALL_GCWD				17

/*
 * rdi: handle (int)
 * ret: success (int)
 */
#define SYSCALL_CCWD				18

/*
 * rdi: handle
 * rsi: replace handle
 * ret: success (int)
 */
#define SYSCALL_LINK				19

/*
 * rdi: handle (int)
 * ret: success (int)
 */
#define SYSCALL_UNLINK			20

/*
 * rdi: handle (int)
 * rsi: statbuf (struct file_info_t*)
 * ret: success (int)
 */
#define SYSCALL_STAT				21

/*
 * ret: current pid (pid_t)
 */
#define SYSCALL_GETPID			22

#define SYSCALL_MAX					23
