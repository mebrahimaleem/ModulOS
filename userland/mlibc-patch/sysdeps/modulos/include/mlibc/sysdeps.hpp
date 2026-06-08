#pragma once

#include <mlibc/sysdep-signatures.hpp>

namespace mlibc {

struct ModulOSSysdepTags :
	LibcLog,
	LibcPanic,
	TcbSet,
	Exit,
	Fork,
	Execve,
	GetPid,
	GetPpid,
	GetGid,
	GetEgid,
	GetUid,
	GetEuid,
	Waitpid,
	FutexWait,
	FutexWake,
	AnonAllocate,
	AnonFree,
	VmMap,
	VmUnmap,
	Openat,
	Open,
	Close,
	Seek,
	Ftruncate,
	Fallocate,
	Read,
	Write,
	OpenDir,
	ReadEntries,
	Isatty,
	Faccessat,
	Access,
	Stat,
	Linkat,
	Link,
	Unlinkat,
	Rmdir,
	Mkdirat,
	Mkdir,
	Renameat,
	Rename,
	Fcntl,
	Dup,
	Dup2,
	GetCwd,
	Fchdir,
	Chdir,
	ClockGet,
	Recvfrom
{};

template<typename Tag>
using Sysdeps = SysdepOf<ModulOSSysdepTags, Tag>;

} // namespace mlibc

