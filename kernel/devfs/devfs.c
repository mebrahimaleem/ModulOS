/* devfs.c - kernel device block file system subsystem routing */
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

#include <stdint.h>
#include <stddef.h>

#include <devfs/devfs.h>
#include <devfs/tty.h>

#include <core/alloc.h>
#include <core/fs.h>

#include <lib/kmemcmp.h>

struct dev_handle_t {
	enum {
		DEV_TYPE_TTY
	} type;
	union {
		struct tty_handle_t* tty;
	} dev_handle;
};

struct file_handle_t* devfs_open(struct mount_cntx_t* cntx, char* path) {
	(void)cntx;
	struct dev_handle_t* dev_handle;

	// tty devices
	if (!kmemcmp(path, "tty", 3)) {
		struct tty_handle_t* handle = tty_open(path + 3);
		if (!handle) {
			// invalid tty
			return 0;
		}

		dev_handle = kmalloc(sizeof(struct dev_handle_t));
		dev_handle->type = DEV_TYPE_TTY;
		dev_handle->dev_handle.tty = handle;

		return (struct file_handle_t*)dev_handle;
	}

	return 0;
}

void devfs_close(struct file_handle_t* handle) {
	struct dev_handle_t* dev_handle = (struct dev_handle_t*)handle;

	if (!dev_handle) {
		return;
	}

	switch (dev_handle->type) {
		case DEV_TYPE_TTY:
			break;
	}

	kfree(dev_handle);
}

enum file_status_t devfs_stat(struct file_handle_t* handle, struct file_info_t* info) {
	struct dev_handle_t* dev_handle = (struct dev_handle_t*)handle;

	if (!dev_handle) {
		return FILE_ERROR;
	}

	switch (dev_handle->type) {
		case DEV_TYPE_TTY:
			info->type = FILE_TYPE_CHAR;
			info->size = TTY_READ_BUFFER_SIZE;
			return FILE_OK;
	}
}

size_t devfs_read(struct file_handle_t* handle, void* buffer, size_t count) {
	struct dev_handle_t* dev_handle = (struct dev_handle_t*)handle;

	if (!dev_handle) {
		return FILE_ERROR;
	}

	switch (dev_handle->type) {
		case DEV_TYPE_TTY:
			tty_read(dev_handle->dev_handle.tty, buffer, count);
			return count;
	}
}

uint64_t devfs_get_seek(struct file_handle_t* handle) {
	struct dev_handle_t* dev_handle = (struct dev_handle_t*)handle;

	if (!dev_handle) {
		return FILE_ERROR;
	}

	switch (dev_handle->type) {
		case DEV_TYPE_TTY:
			return 0;
	}
}


enum file_status_t devfs_seek(struct file_handle_t* handle, uint64_t seek) {
	(void)seek;

	struct dev_handle_t* dev_handle = (struct dev_handle_t*)handle;

	if (!dev_handle) {
		return FILE_ERROR;
	}

	switch (dev_handle->type) {
		case DEV_TYPE_TTY:
			return FILE_NO_SUPPORT;
	}
}

size_t devfs_write(struct file_handle_t* handle, void* buffer, size_t count) {
	struct dev_handle_t* dev_handle = (struct dev_handle_t*)handle;

	if (!dev_handle) {
		return FILE_ERROR;
	}

	switch (dev_handle->type) {
		case DEV_TYPE_TTY:
			tty_write(dev_handle->dev_handle.tty, buffer, count);
			return count;
	}
}
