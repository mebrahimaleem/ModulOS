/* devfs.h - kernel device block file system interface */
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

#ifndef KERNEL_DEVFS_DEVFS_H
#define KERNEL_DEVFS_DEVFS_H

#include <stdint.h>
#include <stddef.h>

#include <kernel/core/fs.h>

extern struct file_handle_t* devfs_open(struct mount_cntx_t* cntx, char* path);
extern void devfs_close(struct file_handle_t* handle);

extern enum file_status_t devfs_stat(struct file_handle_t* handle, struct file_info_t* info);
extern size_t devfs_read(struct file_handle_t* handle, void* buffer, size_t count);
extern uint64_t devfs_get_seek(struct file_handle_t* handle);
extern enum file_status_t devfs_seek(struct file_handle_t* handle, uint64_t seek);
extern size_t devfs_write(struct file_handle_t* handle, void* buffer, size_t count);
extern enum file_status_t devfs_create(struct file_handle_t* handle, const char* name);
extern enum file_status_t devfs_delete(struct file_handle_t* handle);
extern void devfs_delete_final(struct file_handle_t* handle);

extern enum file_status_t devfs_open_dir(struct file_handle_t* handle);
extern void devfs_close_dir(struct file_handle_t* handle);

extern enum file_status_t devfs_read_dir(struct file_handle_t* handle, struct dir_info_t* info);

extern enum file_status_t devfs_create_dir(struct file_handle_t* handle, const char* name);
extern enum file_status_t devfs_delete_dir(struct file_handle_t* handle);

#endif /* KERNEL_DEVFS_DEVFS_H */
