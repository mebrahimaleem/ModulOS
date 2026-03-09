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
	FS_FILE,
	FS_DIR,
};

enum fs_open_mode_t {
	FS_OPEN_NOCREATE = 0,
	FS_OPEN_CREATE = 1
};

enum fs_error_t {
	FS_ERROR_OK,
	FS_ERROR_FAIL
};

struct fs_file_info_t {
	enum fs_file_type_t type;
	size_t size;
};

struct fs_file_t;


typedef enum fs_error_t (*fs_stat_t)(void*, struct fs_file_info_t*);

extern void fs_init(void);

extern void fs_mount(void* root, fs_stat_t stat, const char* path);

#endif /* KERNEL_CORE_FS_H */
