/* fs.c - kernel file system layer interface */
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

#ifndef KERNEL_CORE_FS_H
#define KERNEL_CORE_FS_H

#include <stdint.h>
#include <stddef.h>

struct mount_cntx_t;

struct file_handle_t;
struct dir_handle_t;

enum file_status_t {
	FILE_OK,
	FILE_ERROR,
	FILE_DNE,
	FILE_BUSY,
	FILE_NO_SUPPORT
};

struct file_info_t {
	enum {
		FILE_TYPE_REG,
		FILE_TYPE_DIR
	} type;
	uint64_t size;
};

typedef enum file_status_t (*fs_stat_t)(struct mount_cntx_t* cntx, const char*, struct file_info_t*);

enum file_status_t fs_stat(const char* name, struct file_info_t* info);

void fs_init(void);

enum file_status_t fs_mount(
		const char* mountpoint,
		struct mount_cntx_t* cntx,
		fs_stat_t stat
		);

#endif /* KERNEL_CORE_FS_H */
