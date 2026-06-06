/* userland_conv.h - userland conventions abi */
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

#ifndef KERNEL_ABI_USERLAND_CONV_H
#define KERNEL_ABI_USERLAND_CONV_H

#include <stdint.h>

#ifdef __MODULOS_VERIFY_CONV
#include <abi-bits/stat.h>
#include <abi-bits/wait.h>
#include <abi-bits/fcntl.h>

#include <dirent.h>

#define __MODULOS_VERIFY_SIZE(type) _Static_assert(sizeof(__mlibc_##type) == sizeof(type##_t), \
		"abi mismatch for " #type);
#define __MODULOS_VERIFY_TYPE(type) _Static_assert(sizeof(u_##type) == sizeof(type), \
	"abi mismatch for " #type);
#define __MODULOS_VERIFY_STRUCT(type) _Static_assert(sizeof(struct u_##type) == sizeof(struct type), \
	"abi mismatch for " #type);
#define __MODULOS_VERIFY_MEM(type, mem) \
	_Static_assert(offsetof(struct u_##type, mem) == offsetof(struct type, mem), \
	"abi mismatch for struct " #type "." #mem);

__MODULOS_VERIFY_SIZE(int8)
__MODULOS_VERIFY_SIZE(int16)
__MODULOS_VERIFY_SIZE(int32)
__MODULOS_VERIFY_SIZE(int64)

__MODULOS_VERIFY_SIZE(uint8)
__MODULOS_VERIFY_SIZE(uint16)
__MODULOS_VERIFY_SIZE(uint32)
__MODULOS_VERIFY_SIZE(uint64)
#endif /* __MODULOS_VERIFY_CONV */

#define DT_UNKNOWN 0
#define DT_FIFO 1
#define DT_CHR 2
#define DT_DIR 4
#define DT_BLK 6
#define DT_REG 8
#define DT_LNK 10
#define DT_SOCK 12
#define DT_WHT 14

#define AT_FDCWD -100
#define AT_SYMLINK_NOFOLLOW 0x100
#define AT_REMOVEDIR 0x200
#define AT_SYMLINK_FOLLOW 0x400
#define AT_EACCESS 0x200

#define WNOHANG 1
#define WUNTRACED 2
#define WSTOPPED 2
#define WEXITED 4
#define WCONTINUED 8
#define WNOWAIT 0x01000000

#define __MLIBC_NAME_MAX 255

typedef int64_t u_off_t;
typedef unsigned short u_reclen_t;
typedef long u_time_t;
typedef uint64_t u_ino_t;
typedef uint64_t u_dev_t;
typedef unsigned long u_nlink_t;
typedef unsigned int u_mode_t;
typedef unsigned int u_uid_t;
typedef unsigned int u_gid_t;
typedef long u_blksize_t;
typedef int64_t u_blkcnt_t;

#ifdef __MODULOS_VERIFY_CONV
__MODULOS_VERIFY_TYPE(ino_t)
__MODULOS_VERIFY_TYPE(dev_t)
__MODULOS_VERIFY_TYPE(nlink_t)
__MODULOS_VERIFY_TYPE(mode_t)
__MODULOS_VERIFY_TYPE(gid_t)
__MODULOS_VERIFY_TYPE(blksize_t)
__MODULOS_VERIFY_TYPE(blkcnt_t)
#endif /* __MODULOS_VERIFY_CONV */

struct u_timespec {
	u_time_t tv_sec;
	long tv_nsec;
};

#ifdef __MODULOS_VERIFY_CONV
__MODULOS_VERIFY_STRUCT(timespec)
__MODULOS_VERIFY_MEM(timespec, tv_sec)
__MODULOS_VERIFY_MEM(timespec, tv_nsec)
#endif /* __MODULOS_VERIFY_CONV */

struct u_dirent {
	u_ino_t d_ino;
	u_off_t d_off;
	u_reclen_t d_reclen;
	unsigned char d_type;
	char d_name[__MLIBC_NAME_MAX+1];
};

#ifdef __MODULOS_VERIFY_CONV
__MODULOS_VERIFY_STRUCT(dirent)
__MODULOS_VERIFY_MEM(dirent, d_ino)
__MODULOS_VERIFY_MEM(dirent, d_off)
__MODULOS_VERIFY_MEM(dirent, d_reclen)
__MODULOS_VERIFY_MEM(dirent, d_type)
__MODULOS_VERIFY_MEM(dirent, d_name)
#endif /* __MODULOS_VERIFY_CONV */

struct u_stat {
	u_dev_t st_dev;
	u_ino_t st_ino;
	u_nlink_t st_nlink;
	u_mode_t st_mode;
	u_uid_t st_uid;
	u_gid_t st_gid;
	unsigned int __pad0;
	u_dev_t st_rdev;
	u_off_t st_size;
	u_blksize_t st_blksize;
	u_blkcnt_t st_blocks;
	struct u_timespec st_atim;
	struct u_timespec st_mtim;
	struct u_timespec st_ctim;
	long __unused[3];
};

#ifdef __MODULOS_VERIFY_CONV
__MODULOS_VERIFY_STRUCT(stat)
__MODULOS_VERIFY_MEM(stat, st_dev)
__MODULOS_VERIFY_MEM(stat, st_ino)
__MODULOS_VERIFY_MEM(stat, st_nlink)
__MODULOS_VERIFY_MEM(stat, st_mode)
__MODULOS_VERIFY_MEM(stat, st_uid)
__MODULOS_VERIFY_MEM(stat, st_gid)
__MODULOS_VERIFY_MEM(stat, __pad0)
__MODULOS_VERIFY_MEM(stat, st_rdev)
__MODULOS_VERIFY_MEM(stat, st_size)
__MODULOS_VERIFY_MEM(stat, st_blksize)
__MODULOS_VERIFY_MEM(stat, st_blocks)
__MODULOS_VERIFY_MEM(stat, st_atime)
__MODULOS_VERIFY_MEM(stat, st_mtime)
__MODULOS_VERIFY_MEM(stat, st_ctime)
__MODULOS_VERIFY_MEM(stat, __unused)
#endif /* __MODULOS_VERIFY_CONV */

#endif /* KERNEL_ABI_USERLAND_CONV_H */
