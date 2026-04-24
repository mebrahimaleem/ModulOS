/* sysdeps.cpp - mlibc system dependant dependencies implementation */
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

#include <mlibc/all-sysdeps.hpp>
#include <syscall.h>
#include <syscall_vectors.h>
#include <string.h>

#define stderr	2

#define TRAP asm volatile ("int3" : : : "memory")

namespace mlibc {

// internal

void sys_libc_log(const char *message) {
	ssize_t _ign;
	sys_write(stderr, message, strlen(message), &_ign);
}

[[noreturn]] void sys_libc_panic() {
	sys_libc_log("mlibc panic");
	sys_exit(-1);
}

int sys_tcb_set(void *pointer) {
	asm volatile ("wrfsbaseq %0" : : "r"(pointer) : "memory");
	return 0;
}

int sys_futex_wait(int *pointer, int expected, const struct timespec *time) {
	(void)pointer;
	(void)expected;
	(void)time;
	return 0;
}

int sys_futex_wake(int *pointer) {
	(void)pointer;
	return 0;
}

int sys_anon_allocate(size_t size, void **pointer) {
	uint64_t addr = syscall_1(size, 0, 0, SYSCALL_ALLOC);
	if (!addr) {
		return 1;
	}

	memset((void*)addr, 0, size);

	*pointer = (void*)addr;
	return 0;
}

int sys_anon_free(void *pointer, size_t size) {
	(void)pointer;
	(void)size;
	return 0;
}

int sys_open(const char *pathname, int flags, mode_t mode, int *fd) {
	(void)flags;
	(void)mode;

	int f = syscall_1((uint64_t)pathname, 0, 0, SYSCALL_OPEN);
	if (!f) {
		return 1;
	}

	*fd = f;
	return 0;
}

int sys_read(int fd, void *buf, size_t count, ssize_t *bytes_read) {
	*bytes_read = (ssize_t)syscall_3((uint64_t)fd, (uint64_t)buf, (uint64_t)count, SYSCALL_READ);

	return 0;
}

int sys_seek(int fd, off_t offset, int whence, off_t *new_offset) {
	(void)fd;
	(void)offset;
	(void)whence;
	(void)new_offset;

	return 0;
}

int sys_close(int fd) {
	syscall_1(fd, 0, 0, SYSCALL_CLOSE);
	return 0;
}

// mlibc assumes that anonymous memory returned by sys_vm_map() is zeroed by the kernel / whatever is behind the sysdeps
int sys_vm_map(void *hint, size_t size, int prot, int flags, int fd, off_t offset, void **window) {
	(void)hint;
	(void)size;
	(void)prot;
	(void)flags;
	(void)fd;
	(void)offset;
	(void)window;

	TRAP;
	return 0;
}

int sys_vm_unmap(void *pointer, size_t size) {
	(void)pointer;
	(void)size;
	return 0;
}

// ansi

[[noreturn]] void sys_exit(int status) {
	syscall_1(status, 0, 0, SYSCALL_EXIT);
	__builtin_unreachable();
}

int sys_write(int fd, const void *buf, size_t count, ssize_t *bytes_written) {
	*bytes_written = syscall_3((uint64_t)fd, (uint64_t)buf	, (uint64_t)count, SYSCALL_WRITE);

	return 0;
}

int sys_clock_get(int clock, time_t *secs, long *nanos) {
	(void)clock;
	(void)secs;
	(void)nanos;
	TRAP;
	return 0;
}

} //namespace mlibc
