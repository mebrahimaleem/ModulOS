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

enum fs_file_type_t {
	FS_TYPE_FILE,
	FS_TYPE_DIR
};

enum fs_status_t {
	FS_STATUS_OK,
	FS_STATUS_ERROR,
	FS_STATUS_EOF,
	FS_STATUS_FILE_NOT_FOUND,
	FS_STATUS_BUSY
};

struct fs_info_t {
	enum fs_file_type_t type;
	size_t size;
};

struct fs_ls_info_t {
	struct fs_file_handle_t* handle;
	char name[256];
	uint8_t name_len;
	uint8_t valid;
};

struct fs_file_t;
struct fs_file_handle_t;
struct fs_dirls_handle_t;

extern void fs_init(void);

typedef enum fs_status_t (*fs_stat_t)(struct fs_file_handle_t* handle, struct fs_info_t* info);

typedef struct fs_dirls_handle_t* (*fs_dirst_t)(struct fs_file_handle_t* handle);
typedef enum fs_status_t (*fs_dirls_t)(struct fs_dirls_handle_t** handle, struct fs_ls_info_t* file);
typedef void (*fs_diren_t)(struct fs_dirls_handle_t* handle);

typedef struct fs_file_handle_t* (*fs_create_t)(
		struct fs_file_handle_t* handle,
		enum fs_file_type_t type,
		const char* name,
		uint8_t name_len);

extern void fs_init(void);

extern enum fs_status_t fs_mount(
		struct fs_file_handle_t* root,
		const char* path,
		fs_stat_t stat,
		fs_dirst_t dirst,
		fs_dirls_t dirls,
		fs_diren_t diren,
		fs_create_t create);

extern struct fs_file_t* fs_open(const char* path);
extern void fs_close(struct fs_file_t* file);
extern enum fs_status_t fs_stat(struct fs_file_t* file, struct fs_info_t* info);
extern enum fs_status_t fs_list(struct fs_file_t* file, char* buffer);
extern enum fs_status_t fs_create(struct fs_file_t* file, enum fs_file_type_t type, const char* name);

#ifdef DEBUG
extern void fs_log_vfs_tree(void);
#endif /* DEBUG */

#endif /* KERNEL_CORE_FS_H */
