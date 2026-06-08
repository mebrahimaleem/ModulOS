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

#include <sys/syscall.h>
#include <sys/syscall_vectors.h>

#include <mlibc/all-sysdeps.hpp>
#include <mlibc/fsfd_target.hpp>

#include <abi-bits/fcntl.h>
#include <abi-bits/seek-whence.h>
#include <abi-bits/errno.h>
#include <abi-bits/uid_t.h>
#include <abi-bits/gid_t.h>
#include <abi-bits/pid_t.h>
#include <abi-bits/stat.h>

#include <stdlib.h>

#include <string.h>

#define stdout	0
#define stdin		1
#define stderr	2

#define SECONDS_PER_NANO	0x10000000000		

namespace mlibc {

// misc

void Sysdeps<LibcLog>::operator()(const char *message) {
	syscall_1((uint64_t)message, 0, 0, SYSCALL_LOG);
}

void Sysdeps<LibcPanic>::operator()() {
	Sysdeps<Exit>{}(-1);
}

// process

int Sysdeps<TcbSet>::operator()(void *pointer) {
	asm volatile ("wrfsbaseq %0" : : "r"(pointer) : "memory");
	return 0;
}

void Sysdeps<Exit>::operator()(int status) {
	syscall_1(status, 0, 0, SYSCALL_EXIT);
	__builtin_unreachable();
}

int Sysdeps<Fork>::operator()(pid_t *child) {
	uint64_t pid = syscall_6_nr(0, 0, 0, SYSCALL_FORK, 0, 0, 0);

	if (pid == SYSCALL_STS_FAIL) {
		return ENOMEM;
	}

	*child = (pid_t)pid;
	return 0;
}

int Sysdeps<Execve>::operator()(const char *path, char *const argv[], char *const envp[]) {
	syscall_3((uint64_t)path, (uint64_t)argv, (uint64_t)envp, SYSCALL_EXECVE);

	return ENOENT;
}

pid_t Sysdeps<GetPid>::operator()() {
	return syscall_0(0, 0, 0, SYSCALL_GETPID);
}

pid_t Sysdeps<GetPpid>::operator()() {
	return syscall_0(0, 0, 0, SYSCALL_GETPPID);
}

gid_t Sysdeps<GetGid>::operator()() {
	return 0;
}

gid_t Sysdeps<GetEgid>::operator()() {
	return 0;
}

uid_t Sysdeps<GetUid>::operator()() {
	return 0;
}

uid_t Sysdeps<GetEuid>::operator()() {
	return 0;
}

int Sysdeps<Waitpid>::operator()(pid_t pid, int *status, int flags, struct rusage *ru, pid_t *ret_pid) {
	(void)ru;

	uint64_t p;
	uint64_t sts = syscall_4(pid, (uint64_t)status, flags, SYSCALL_WAITPID, (uint64_t)&p);

	if (sts == SYSCALL_STS_FAIL) {
		return ECHILD;
	}

	*ret_pid = p;

	return 0;
}

// locking

int Sysdeps<FutexWait>::operator()(int *pointer, int expected, const struct timespec *time) {
	(void)pointer;
	(void)expected;
	(void)time;
	return 0;
}

int Sysdeps<FutexWake>::operator()(int *pointer, bool all) {
	(void)pointer;
	(void)all;
	return 0;
}

// memory

int Sysdeps<AnonAllocate>::operator()(size_t size, void **pointer) {
	uint64_t addr = syscall_1(size, 0, 0, SYSCALL_ALLOC);
	if (!addr) {
		return ENOMEM;
	}

	memset((void*)addr, 0, size);

	*pointer = (void*)addr;
	return 0;
}

int Sysdeps<AnonFree>::operator()(void *pointer, size_t size) {
	(void)pointer;
	(void)size;
	return 0;
}

// mlibc assumes that anonymous memory returned by sys_vm_map() is zeroed by the kernel / whatever is behind the sysdeps
int Sysdeps<VmMap>::operator()(void *hint, size_t size, int prot, int flags, int fd, off_t offset, void **window) {
	(void)hint;
	(void)size;
	(void)prot;
	(void)flags;
	(void)fd;
	(void)offset;
	(void)window;

	return ENOSYS;
}

int Sysdeps<VmUnmap>::operator()(void *pointer, size_t size) {
	(void)pointer;
	(void)size;
	return 0;
}

// files

int Sysdeps<Openat>::operator()(int dirfd, const char *path, int flags, mode_t mode, int *fd) {
	uint64_t f = syscall_4((uint64_t)path, flags, dirfd, SYSCALL_OPENAT, mode);

	if (f == SYSCALL_STS_FAIL) {
		return ENOENT;
	}
	
	*fd = f;
	return 0;
}

int Sysdeps<Open>::operator()(const char *pathname, int flags, mode_t mode, int *fd) {
	return Sysdeps<Openat>{}(AT_FDCWD, pathname, flags, mode, fd);
}

int Sysdeps<Close>::operator()(int fd) {
	syscall_1(fd, 0, 0, SYSCALL_CLOSE);
	return 0;
}

int Sysdeps<Seek>::operator()(int fd, off_t offset, int whence, off_t *new_offset) {
	uint64_t new_off;
	switch (whence) {
		case SEEK_END:
			struct stat statbuf;
			if (Sysdeps<Stat>{}(fsfd_target::fd, fd, nullptr, 0, &statbuf)) {
				return EBADF;
			}
			offset += statbuf.st_size;
			goto set;
		case SEEK_CUR:
			new_off = syscall_1(fd, 0, 0, SYSCALL_TELL);
			if (new_off == SYSCALL_STS_FAIL) {
				return EBADF;
			}
			offset += new_off;
			goto set;
		case SEEK_SET:
set:
			new_off = syscall_2(fd, (uint64_t)offset, 0, SYSCALL_SEEK);
			if (new_off == SYSCALL_STS_FAIL) {
				return EBADF;
			}

			*new_offset = new_off;
			return 0;

		default:
			return ENOSYS;
	}
}

int Sysdeps<Ftruncate>::operator()(int fd, size_t size) {
	(void)fd;
	(void)size;

	if (syscall_2(fd, size, 0, SYSCALL_TRUNCATE) == SYSCALL_STS_FAIL) {
		return ENOENT;
	}
	return 0;
}

int Sysdeps<Fallocate>::operator()(int fd, off_t offset, size_t size) {
	return Sysdeps<Ftruncate>{}(fd, offset + size);
}

int Sysdeps<Read>::operator()(int fd, void *buf, size_t count, ssize_t *bytes_read) {
	*bytes_read = (ssize_t)syscall_3((uint64_t)fd, (uint64_t)buf, (uint64_t)count, SYSCALL_READ);

	return 0;
}

int Sysdeps<Write>::operator()(int fd, const void *buf, size_t count, ssize_t *bytes_written) {
	*bytes_written = syscall_3((uint64_t)fd, (uint64_t)buf, (uint64_t)count, SYSCALL_WRITE);

	return 0;
}

int Sysdeps<OpenDir>::operator()(const char *path, int *handle) {
	int fd;
	int sts;
	if ((sts = Sysdeps<Open>{}(path, O_RDWR, 0, &fd))) {
		return sts;
	}

	if (syscall_1(fd, 0, 0, SYSCALL_OPEN_DIR) == SYSCALL_STS_FAIL) {
		Sysdeps<Close>{}(fd);
		return ENOTDIR;
	}

	*handle = (int)fd;

	return 0;
}

int Sysdeps<ReadEntries>::operator()(int handle, void *buffer, size_t max_size,
		size_t *bytes_read) {

	uint64_t bytes = syscall_3(handle, (uint64_t)buffer, max_size, SYSCALL_READ_DIR);

	if (bytes == SYSCALL_STS_FAIL) {
		return ENOTDIR;
	}

	*bytes_read = bytes;
	return 0;
}

int Sysdeps<Isatty>::operator()(int fd) {
	if (syscall_1(fd, 0, 0, SYSCALL_IS_A_TTY)) {
		return 0; //mlibc expects 0 for tty
	}

	return ENOTTY;
}

int Sysdeps<Faccessat>::operator()(int dirfd, const char *pathname, int mode, int flags) {
	(void)dirfd;
	(void)pathname;
	(void)mode;
	(void)flags;

	return 0;
}

int Sysdeps<Access>::operator()(const char *path, int mode) {
	return Sysdeps<Faccessat>{}(AT_FDCWD, path, mode, 0);
}

int Sysdeps<Stat>::operator()(fsfd_target fsfdt, int fd, const char *path, int flags,
		struct stat *statbuf) {

	int f;
	int sts;

	switch (fsfdt) {
		case fsfd_target::fd:
			f = fd;
			break;
		case fsfd_target::path:
			if ((sts = Sysdeps<Open>{}(path, flags, 0, &f))) {
				return sts;
			}
			break;
		case fsfd_target::fd_path:
			if ((sts = Sysdeps<Openat>{}(fd, path, flags, 0, &f))) {
				return sts;
			}
			break;
		default:
			return ENOSYS;

	}

	struct stat temp;
	uint64_t res = syscall_2(f, (uint64_t)&temp, 0, SYSCALL_STAT);

	if (res == SYSCALL_STS_FAIL) {
		return EACCES;
	}

	*statbuf = temp;
	return 0;
}

int Sysdeps<Linkat>::operator()(int olddirfd, const char *old_path, int newdirfd, const char *new_path, int flags) {
	int sts;
	int old_f;
	int new_f;

	if ((sts = Sysdeps<Openat>{}(olddirfd, old_path, flags, 0, &old_f))) {
		return sts;
	}

	if ((sts = Sysdeps<Openat>{}(newdirfd, new_path, flags, 0, &new_f))) {
		return sts;
	}

	uint64_t res = syscall_2(old_f, new_f, 0, SYSCALL_LINK);

	if (res == SYSCALL_STS_FAIL) {
		return ENOENT;
	}

	return 0;
}

int Sysdeps<Link>::operator()(const char *old_path, const char *new_path) {
	return Sysdeps<Linkat>{}(AT_FDCWD, old_path, AT_FDCWD, new_path, 0);
}

int Sysdeps<Unlinkat>::operator()(int fd, const char *path, int flags) {
	(void)flags;

	int sts;
	int f;

	if ((sts = Sysdeps<Openat>{}(fd, path, O_RDWR, 0, &f))) {
		return sts;
	}

	uint64_t res = syscall_1(f, 0, 0, SYSCALL_UNLINK);

	if (res == SYSCALL_STS_FAIL) {
		return ENOENT;
	}

	return 0;
}

int Sysdeps<Rmdir>::operator()(const char *path) {
	return Sysdeps<Unlinkat>{}(AT_FDCWD, path, AT_REMOVEDIR);
}

int Sysdeps<Mkdirat>::operator()(int dirfd, const char *path, mode_t mode) {
	(void)dirfd;
	(void)path;
	(void)mode;

	int sts;
	int fd;

	sts = Sysdeps<Openat>{}(dirfd, path, O_RDWR | O_CREAT | O_EXCL, 0, &fd);

	if (sts) {
		return sts;
	}

	sts = syscall_1(fd, 0, 0, SYSCALL_CREATE_DIR);
	Sysdeps<Close>{}(fd);

	if (sts) {
		Sysdeps<Unlinkat>{}(dirfd, path, 0);
		return ENOENT;
	}

	return 0;
}

int Sysdeps<Mkdir>::operator()(const char *path, mode_t mode) {
	return Sysdeps<Mkdirat>{}(AT_FDCWD, path, mode);
}

int Sysdeps<Renameat>::operator()(int olddirfd, const char *old_path, int newdirfd, const char *new_path) {
	int sts;
	if ((sts = Sysdeps<Linkat>{}(olddirfd, old_path, newdirfd, new_path, 0))) {
		return sts;
	}

	return Sysdeps<Unlinkat>{}(olddirfd, old_path, 0);
}

int Sysdeps<Rename>::operator()(const char *path, const char *new_path) {
	return Sysdeps<Renameat>{}(AT_FDCWD, path, AT_FDCWD, new_path);
}

int Sysdeps<Fcntl>::operator()(int fd, int request, va_list args, int *result) {
	(void)fd;
	(void)request;
	(void)args;
	*result = 0;

	//TODO
	return 0;
}

int Sysdeps<Dup>::operator()(int fd, int flags, int *newfd) {
	(void)flags;

	uint64_t sts = syscall_1(fd, 0, 0, SYSCALL_DUP);

	if (sts == SYSCALL_STS_FAIL) {
		return EBADF;
	}

	*newfd = sts;
	return 0;
}

int Sysdeps<Dup2>::operator()(int fd, int flags, int newfd) {
	(void)flags;

	uint64_t sts = syscall_1(fd, newfd, 0, SYSCALL_DUP2);

	if (sts == SYSCALL_STS_FAIL) {
		return EBADF;
	}

	return 0;
}

// working directory 

int Sysdeps<GetCwd>::operator()(char *buffer, size_t size) {
	if (syscall_2((uint64_t)buffer, size, 0, SYSCALL_GCWD) == SYSCALL_STS_FAIL) {
		return EINVAL;
	}

	return 0;
}

int Sysdeps<Fchdir>::operator()(int fd) {
	if (syscall_1(fd, 0, 0, SYSCALL_CCWD) == SYSCALL_STS_FAIL) {
		return EBADF;
	}
	
	return 0;
}

int Sysdeps<Chdir>::operator()(const char *path) {
	int fd;
	int sts;
	if ((sts = Sysdeps<Open>{}(path, O_RDWR, 0, &fd))) {
		return sts;
	}

	sts = Sysdeps<Fchdir>{}(fd);
	Sysdeps<Close>{}(fd);

	return sts;
}

// time

int Sysdeps<ClockGet>::operator()(int clock, time_t *secs, long *nanos) {
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

// networking

int Sysdeps<Recvfrom>::operator()(int fd, void* buffer, size_t size, int flags, struct sockaddr* sock_addr, socklen_t* addr_length, ssize_t* length) {
	(void)fd;
	(void)buffer;
	(void)size;
	(void)flags;
	(void)sock_addr;
	(void)addr_length;
	(void)length;

	return ENOSYS;
}

} //namespace mlibc

extern "C" {
#define __MODULOS_VERIFY_CONV
#include <sys/userland_conv.h>
}
