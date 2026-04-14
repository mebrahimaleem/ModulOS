/* fs.h - kernel file system layer interface */
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

struct fs_handle_t;

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
		FILE_TYPE_DIR,
		FILE_TYPE_CHAR
	} type;
	uint64_t size;
};

typedef struct file_handle_t* (*fs_open_t)(struct mount_cntx_t*, char*);
typedef void (*fs_close_t)(struct file_handle_t*);

typedef enum file_status_t (*fs_stat_t)(struct file_handle_t*, struct file_info_t*);
typedef size_t (*fs_read_t)(struct file_handle_t*, void*, size_t);
typedef uint64_t (*fs_get_seek_t)(struct file_handle_t*);
typedef enum file_status_t (*fs_seek_t)(struct file_handle_t*, uint64_t);

void fs_init(void);

enum file_status_t fs_mount(
		const char* mountpoint,
		struct mount_cntx_t* cntx,
		fs_open_t open,
		fs_close_t close,
		fs_stat_t stat,
		fs_read_t read,
		fs_get_seek_t get_seek,
		fs_seek_t seek
		);

extern struct fs_handle_t* fs_open(const char* path);
extern void fs_close(struct fs_handle_t* handle);

extern enum file_status_t fs_stat(struct fs_handle_t* handle, struct file_info_t* info);
extern size_t fs_read(struct fs_handle_t* handle, void* buffer, size_t count);
extern uint64_t fs_get_seek(struct fs_handle_t* handle);
extern enum file_status_t fs_seek(struct fs_handle_t* handle, uint64_t seek);

#endif /* KERNEL_CORE_FS_H */
