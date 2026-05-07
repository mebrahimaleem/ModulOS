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

#define FILE_MODE_READ		0x1
#define FILE_MODE_WRITE		0x2

#define FILE_INFO_UNK			0
#define FILE_INFO_REG			1
#define FILE_INFO_DIR			2

struct mount_cntx_t;

struct file_handle_t;

struct fs_handle_t;

enum file_status_t {
	FILE_OK,
	FILE_ERROR,
	FILE_DNE,
	FILE_BUSY,
	FILE_NO_SUPPORT,
	FILE_BAD_MODE,
	FILE_NOT_DIR,
};

struct file_info_t {
	enum {
		FILE_TYPE_REG,
		FILE_TYPE_DIR,
		FILE_TYPE_CHAR
	} type;
	uint64_t size;
};

struct dir_info_t {
	uint64_t inode_num;
	uint64_t seek_pos;
	uint16_t rec_len;
	uint8_t type;
	char name[256];
};

typedef struct file_handle_t* (*fs_open_t)(struct mount_cntx_t*, const char*);
typedef void (*fs_close_t)(struct file_handle_t*);

typedef enum file_status_t (*fs_stat_t)(struct file_handle_t*, struct file_info_t*);
typedef size_t (*fs_read_t)(struct file_handle_t*, void*, size_t);
typedef uint64_t (*fs_get_seek_t)(struct file_handle_t*);
typedef enum file_status_t (*fs_seek_t)(struct file_handle_t*, uint64_t);
typedef size_t (*fs_write_t)(struct file_handle_t*, void*, size_t);
typedef enum file_status_t (*fs_create_t)(struct file_handle_t*, const char*);
typedef enum file_status_t (*fs_delete_t)(struct file_handle_t*);
typedef void (*fs_delete_final_t)(struct file_handle_t*);

typedef enum file_status_t (*fs_open_dir_t)(struct file_handle_t*);
typedef void (*fs_close_dir_t)(struct file_handle_t*);

typedef enum file_status_t (*fs_read_dir_t)(struct file_handle_t*, struct dir_info_t*);

typedef enum file_status_t (*fs_create_dir_t)(struct file_handle_t*, const char*);
typedef enum file_status_t (*fs_delete_dir_t)(struct file_handle_t*);

typedef uint8_t (*fs_is_interactive_t)(struct file_handle_t*);

void fs_init(void);

enum file_status_t fs_mount(
		const char* mountpoint,
		struct mount_cntx_t* cntx,
		fs_open_t open,
		fs_close_t close,
		fs_stat_t stat,
		fs_read_t read,
		fs_get_seek_t get_seek,
		fs_seek_t seek,
		fs_write_t write,
		fs_create_t create,
		fs_delete_t delete,
		fs_delete_final_t delete_final,
		fs_open_dir_t open_dir,
		fs_close_dir_t close_dir,
		fs_read_dir_t read_dir,
		fs_create_dir_t create_dir,
		fs_delete_dir_t delete_dir
		);

extern struct fs_handle_t* fs_open(const char* path, uint8_t mode);
extern void fs_close(struct fs_handle_t* handle);

extern enum file_status_t fs_stat(struct fs_handle_t* handle, struct file_info_t* info);
extern size_t fs_read(struct fs_handle_t* handle, void* buffer, size_t count);
extern uint64_t fs_get_seek(struct fs_handle_t* handle);
extern enum file_status_t fs_seek(struct fs_handle_t* handle, uint64_t seek);
extern size_t fs_write(struct fs_handle_t* handle, void* buffer, size_t count);
extern enum file_status_t fs_create(struct fs_handle_t* handle, const char* name);
extern enum file_status_t fs_delete(struct fs_handle_t* handle);

extern struct fs_handle_t* fs_open_dir(struct fs_handle_t* handle);
extern void fs_close_dir(struct fs_handle_t* handle);

extern enum file_status_t fs_read_dir(struct fs_handle_t* handle, struct dir_info_t* info);

extern enum file_status_t fs_create_dir(struct fs_handle_t* handle, const char* name);
extern enum file_status_t fs_delete_dir(struct fs_handle_t* handle);

extern uint8_t fs_is_interactive(struct fs_handle_t* handle);

#endif /* KERNEL_CORE_FS_H */
