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
/*
struct fs_dir_handle_t;

enum fs_status_t {
	FS_STATUS_OK,
	FS_STATUS_ERROR
};

struct fs_stat_buf_t {
};

typedef enum fs_status_t (*fs_stat_t)(const char* path, struct fs_stat_buf_t* stat_buf);
typedef struct fs_dir_handle_t* (*fs_opendir_t)(const char* path);
typedef void (*fs_closedir_t)(struct fs_dir_handle_t* handle);
typedef enum fs_status_t (*fs_readdir_t)(struct fs_dir_handle_t* handle, const char** name);

extern void fs_init(void);

extern void fs_mount(
		const char* path,
		fs_stat_t stat,
		fs_opendir_t opendir,
		fs_closedir_t closedir,
		fs_readdir_t readdir);
*/

/* old interface */

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


typedef struct fs_file_open_handle_t* (*fs_open_t)(struct fs_file_handle_t* handle);
typedef void (*fs_close_t)(struct fs_file_open_handle_t* handle);

typedef enum fs_status_t (*fs_stat_t)(struct fs_file_open_handle_t* handle, struct fs_info_t* info);
typedef enum fs_status_t (*fs_read_dir_t)(struct fs_file_open_handle_t* handle, struct fs_file_handle_t** file, char* name);

typedef struct fs_file_handle_t* (*fs_create_t)(
		struct fs_file_handle_t* handle,
		enum fs_file_type_t type,
		const char* name,
		uint8_t name_len);

extern void fs_init(void);

extern enum fs_status_t fs_mount(
		struct fs_file_handle_t* root,
		const char* path,
		fs_open_t open,
		fs_close_t close,
		fs_stat_t stat,
		fs_read_dir_t read_dir);

extern struct fs_file_t* fs_open(const char* path);
extern void fs_close(struct fs_file_t* file);

extern enum fs_status_t fs_stat(struct fs_file_t* file, struct fs_info_t* info);
extern enum fs_status_t fs_read_dir(struct fs_file_t* file, char* buffer);

#endif /* KERNEL_CORE_FS_H */
