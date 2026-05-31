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

#include <syscall.h>
#include <syscall_vectors.h>

#include <mlibc/all-sysdeps.hpp>
#include <mlibc/fsfd_target.hpp>

#include <abi-bits/fcntl.h>
#include <abi-bits/seek-whence.h>
#include <abi-bits/errno.h>

#include <stdlib.h>

#include <string.h>

#define stdout	0
#define stdin		1
#define stderr	2

#define SECONDS_PER_NANO	0x10000000000		

extern "C" {
struct file_info_t {
	enum {
		FILE_TYPE_REG,
		FILE_TYPE_DIR,
		FILE_TYPE_CHAR
	} type;
	uint64_t size;
};
}

namespace mlibc {

// misc

void sys_libc_log(const char *message) {
	ssize_t _ign;
	sys_write(stderr, message, strlen(message), &_ign);
	sys_write(stderr, "\n", 1, &_ign);
}

[[noreturn]] void sys_libc_panic() {
	sys_libc_log("mlibc panic");
	sys_exit(-1);
}

// proccess

int sys_tcb_set(void *pointer) {
	asm volatile ("wrfsbaseq %0" : : "r"(pointer) : "memory");
	return 0;
}

[[noreturn]] void sys_exit(int status) {
	syscall_1(status, 0, 0, SYSCALL_EXIT);
	__builtin_unreachable();
}

pid_t sys_getpid() {
	return syscall_0(0, 0, 0, SYSCALL_GETPID);
}

int sys_fork(pid_t *child) {
	pid_t pid = syscall_0(0, 0, 0, SYSCALL_FORK);

	if (pid == SYSCALL_STS_FAIL) {
		return ENOMEM;
	}

	*child = pid;
	return 0;
}

int sys_execve(const char *path, char *const argv[], char *const envp[]) {
	syscall_3((uint64_t)path, (uint64_t)argv, (uint64_t)envp, SYSCALL_EXECVE);

	return EACCES;
}

// locking

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

// memory

int sys_anon_allocate(size_t size, void **pointer) {
	uint64_t addr = syscall_1(size, 0, 0, SYSCALL_ALLOC);
	if (!addr) {
		return ENOMEM;
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

// mlibc assumes that anonymous memory returned by sys_vm_map() is zeroed by the kernel / whatever is behind the sysdeps
int sys_vm_map(void *hint, size_t size, int prot, int flags, int fd, off_t offset, void **window) {
	(void)hint;
	(void)size;
	(void)prot;
	(void)flags;
	(void)fd;
	(void)offset;
	(void)window;

	return ENOSYS;
}

int sys_vm_unmap(void *pointer, size_t size) {
	(void)pointer;
	(void)size;
	return 0;
}

// files

int sys_openat(int dirfd, const char *path, int flags, mode_t mode, int *fd) {
	uint64_t f = syscall_4((uint64_t)path, flags, dirfd, SYSCALL_OPENAT, mode);

	if (f == SYSCALL_STS_FAIL) {
		return EACCES;
	}
	
	*fd = f;
	return 0;
}

int sys_open(const char *pathname, int flags, mode_t mode, int *fd) {
	return sys_openat(AT_FDCWD, pathname, flags, mode, fd);
}

int sys_close(int fd) {
	syscall_1(fd, 0, 0, SYSCALL_CLOSE);
	return 0;
}

int sys_seek(int fd, off_t offset, int whence, off_t *new_offset) {
	off_t new_off;
	switch (whence) {
		case SEEK_END:
			struct stat statbuf;
			if (sys_stat(fsfd_target::fd, fd, nullptr, 0, &statbuf)) {
				return EACCES;
			}
			offset += statbuf.st_size;
			goto set;
		case SEEK_CUR:
			offset += syscall_1(fd, 0, 0, SYSCALL_TELL);
			goto set;
		case SEEK_SET:
set:
			new_off = syscall_2(fd, (uint64_t)offset, 0, SYSCALL_SEEK);
			if ((uint64_t)*new_offset == SYSCALL_STS_FAIL) {
				return EACCES;
			}

			*new_offset = new_off;
			return 0;

		default:
			return ENOSYS;
	}
}

int sys_ftruncate(int fd, size_t size) {
	(void)fd;
	(void)size;

	if (syscall_2(fd, size, 0, SYSCALL_TRUNCATE) == SYSCALL_STS_FAIL) {
		return EACCES;
	}
	return 0;
}

int sys_fallocate(int fd, off_t offset, size_t size) {
	return sys_ftruncate(fd, offset + size);
}

int sys_read(int fd, void *buf, size_t count, ssize_t *bytes_read) {
	*bytes_read = (ssize_t)syscall_3((uint64_t)fd, (uint64_t)buf, (uint64_t)count, SYSCALL_READ);

	return 0;
}

int sys_write(int fd, const void *buf, size_t count, ssize_t *bytes_written) {
	*bytes_written = syscall_3((uint64_t)fd, (uint64_t)buf, (uint64_t)count, SYSCALL_WRITE);

	return 0;
}

int sys_open_dir(const char *path, int *handle) {
	int fd;
	int sts;
	if ((sts = sys_open(path, O_RDWR, 0, &fd))) {
		return sts;
	}

	uint64_t dfd = syscall_1(fd, 0, 0, SYSCALL_OPEN_DIR);

	if (dfd == SYSCALL_STS_FAIL) {
		sys_close(fd);
		return ENOTDIR;
	}

	*handle = (int)dfd;

	return 0;
}

int sys_read_entries(int handle, void *buffer, size_t max_size,
		size_t *bytes_read) {
	(void)handle;
	(void)buffer;
	(void)max_size;
	(void)bytes_read;

	//TODO
	return ENOSYS;
}

int sys_isatty(int fd) {
	if (syscall_1(fd, 0, 0, SYSCALL_IS_A_TTY)) {
		return 0; //mlibc expects 0 for tty
	}

	return ENOTTY;
}

int sys_faccessat(int dirfd, const char *pathname, int mode, int flags) {
	(void)dirfd;
	(void)pathname;
	(void)mode;
	(void)flags;

	return 0;
}

int sys_access(const char *path, int mode) {
	return sys_faccessat(AT_FDCWD, path, mode, 0);
}

int sys_stat(fsfd_target fsfdt, int fd, const char *path, int flags,
		struct stat *statbuf) {

	int f;
	int sts;

	switch (fsfdt) {
		case fsfd_target::fd:
			f = fd;
			break;
		case fsfd_target::path:
			if ((sts = sys_open(path, flags, 0, &f))) {
				return sts;
			}
		case fsfd_target::fd_path:
			if ((sts = sys_openat(fd, path, flags, 0, &f))) {
				return sts;
			}
		default:
			return ENOSYS;

	}

	struct file_info_t buf;
	uint64_t res = syscall_2(f, (uint64_t)&buf, 0, SYSCALL_STAT);

	if (res == SYSCALL_STS_FAIL) {
		return EACCES;
	}

	statbuf->st_size = buf.size;

	return 0;
}

int sys_linkat(int olddirfd, const char *old_path, int newdirfd, const char *new_path, int flags) {
	int sts;
	int old_f;
	int new_f;

	if ((sts = sys_openat(olddirfd, old_path, flags, 0, &old_f))) {
		return sts;
	}

	if ((sts = sys_openat(newdirfd, new_path, flags, 0, &new_f))) {
		return sts;
	}

	uint64_t res = syscall_2(old_f, new_f, 0, SYSCALL_LINK);

	if (res == SYSCALL_STS_FAIL) {
		return EACCES;
	}

	return 0;
}

int sys_link(const char *old_path, const char *new_path) {
	return sys_linkat(AT_FDCWD, old_path, AT_FDCWD, new_path, 0);
}

int sys_unlinkat(int fd, const char *path, int flags) {
	(void)flags;

	int sts;
	int f;

	if ((sts = sys_openat(fd, path, O_RDWR, 0, &f))) {
		return sts;
	}

	uint64_t res = syscall_1(f, 0, 0, SYSCALL_UNLINK);

	if (res == SYSCALL_STS_FAIL) {
		return EACCES;
	}

	return 0;
}

int sys_rmdir(const char *path) {
	return sys_unlinkat(AT_FDCWD, path, AT_REMOVEDIR);
}

int sys_mkdirat(int dirfd, const char *path, mode_t mode) {
	(void)dirfd;
	(void)path;
	(void)mode;

	int sts;
	int fd;

	sts = sys_openat(dirfd, path, O_RDWR | O_CREAT | O_EXCL, 0, &fd);

	if (sts) {
		return sts;
	}

	sts = syscall_1(fd, 0, 0, SYSCALL_CREATE_DIR);
	sys_close(fd);

	if (sts) {
		sys_unlinkat(dirfd, path, 0);
		return EACCES;
	}

	return 0;
}

int sys_mkdir(const char *path, mode_t mode) {
	return sys_mkdirat(AT_FDCWD, path, mode);
}

int sys_renameat(int olddirfd, const char *old_path, int newdirfd, const char *new_path) {
	int sts;
	if ((sts = sys_linkat(olddirfd, old_path, newdirfd, new_path, 0))) {
		return sts;
	}

	return sys_unlinkat(olddirfd, old_path, 0);
}

int sys_rename(const char *path, const char *new_path) {
	return sys_renameat(AT_FDCWD, path, AT_FDCWD, new_path);
}

// working directory 

int sys_getcwd(char *buffer, size_t size) {
	if (syscall_2((uint64_t)buffer, size, 0, SYSCALL_GCWD) == SYSCALL_STS_FAIL) {
		return EACCES;
	}

	return 0;
}

int sys_fchdir(int fd) {
	if (syscall_1(fd, 0, 0, SYSCALL_CCWD) SYSCALL_STS_FAIL) {
		return ENOTDIR;
	}
	
	return 0;
}

int sys_chdir(const char *path) {
	int fd;
	int sts;
	if ((sts = sys_open(path, O_RDWR, 0, &fd))) {
		return sts;
	}

	sts = sys_fchdir(fd);
	sys_close(fd);

	return sts;
}

// time

int sys_clock_get(int clock, time_t *secs, long *nanos) {
	uint64_t epoch_nanos = syscall_0(0, 0, 0, SYSCALL_EPOCH_TIME);

	switch (clock) {
		case CLOCK_REALTIME:
		case CLOCK_MONOTONIC:
		case CLOCK_MONOTONIC_RAW:
		case CLOCK_REALTIME_COARSE:
		case CLOCK_MONOTONIC_COARSE:
		case CLOCK_BOOTTIME:
		case CLOCK_REALTIME_ALARM:
		case CLOCK_BOOTTIME_ALARM:
		case CLOCK_TAI:
			*secs = epoch_nanos / SECONDS_PER_NANO;
			*nanos = epoch_nanos % SECONDS_PER_NANO;
			return 0;
		default:
			return ENOSYS;
	}
}


} //namespace mlibc
