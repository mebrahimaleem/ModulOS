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
	union {
		struct tty_handle_t* tty;
	} dev_handle;
	enum {
		DEV_TYPE_TTY,
	} type;
};

static struct fs_mount_t* dev_mount;

static struct file_handle_t* devfs_open(struct mount_cntx_t* cntx, const char* path, uint32_t flags, uint32_t mode) {
	(void)cntx;
	(void)flags;
	(void)mode;
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

static void devfs_close(struct file_handle_t* handle) {
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

static enum file_status_t devfs_stat(struct file_handle_t* handle, struct file_info_t* info) {
	struct dev_handle_t* dev_handle = (struct dev_handle_t*)handle;

	if (!dev_handle) {
		return FILE_ERROR;
	}

	switch (dev_handle->type) {
		case DEV_TYPE_TTY:
			info->mode = S_IFCHR;
			info->size = TTY_READ_BUFFER_SIZE;
			return FILE_OK;
	}
}

static size_t devfs_read(struct file_handle_t* handle, void* buffer, size_t count) {
	struct dev_handle_t* dev_handle = (struct dev_handle_t*)handle;

	if (!dev_handle) {
		return FILE_ERROR;
	}

	switch (dev_handle->type) {
		case DEV_TYPE_TTY:
			return tty_read(dev_handle->dev_handle.tty, buffer, count);
	}
}

static size_t devfs_write(struct file_handle_t* handle, const void* buffer, size_t count) {
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

static uint8_t devfs_is_interactive(struct file_handle_t* handle) {
	struct dev_handle_t* dev_handle = (struct dev_handle_t*)handle;

	if (!dev_handle) {
		return FILE_ERROR;
	}

	switch (dev_handle->type) {
		case DEV_TYPE_TTY:
			return 1;
	}
}

static struct file_handle_t* devfs_dup(struct file_handle_t* handle) {
	struct dev_handle_t* dev_handle = (struct dev_handle_t*)handle;

	if (!dev_handle) {
		return 0;
	}

	struct dev_handle_t* dev_handle2 = kmalloc(sizeof(struct dev_handle_t));

	dev_handle2->type = dev_handle->type;

	switch (dev_handle->type) {
		case DEV_TYPE_TTY:
			dev_handle2->dev_handle.tty = dev_handle->dev_handle.tty;
			break;
	}

	return (struct file_handle_t*)dev_handle2;
}

void devfs_init(void) {
	dev_mount = fs_mount("/dev", 0);

	fs_mount_assign_open(dev_mount, devfs_open);	
	fs_mount_assign_close(dev_mount, devfs_close);	
	fs_mount_assign_stat(dev_mount, devfs_stat);	
	fs_mount_assign_read(dev_mount, devfs_read);	
	fs_mount_assign_write(dev_mount, devfs_write);	
	fs_mount_assign_is_interactive(dev_mount, devfs_is_interactive);	
	fs_mount_assign_dup(dev_mount, devfs_dup);	
}
