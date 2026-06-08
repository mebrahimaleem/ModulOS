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
#include <lib/kstrcmp.h>
#include <lib/kmemset.h>
#include <lib/kstrlen.h>
#include <lib/kstrcpy.h>

static struct fs_mount_t* dev_mount;

static uint64_t devid;
static uint64_t dev_inode_num;

struct devfs_tree_node_t {
	struct devfs_tree_node_t* sub;
	struct devfs_tree_node_t* co;
	char* name;

	uint64_t inode_num;
	union {
		struct tty_handle_t* tty;
	} dev_handle;
	enum {
		DEV_TYPE_DIR,
		DEV_TYPE_TTY,
	} type;
};

struct dev_handle_t {
	struct devfs_tree_node_t* node;
	struct devfs_tree_node_t* read_index;
};

struct devfs_tree_node_t* devfs_root;

struct devfs_tree_node_t devfs_root_node;

#ifdef SERIAL
struct devfs_tree_node_t devfs_serial1_node;
struct devfs_tree_node_t devfs_serial2_node;
#endif /* SERIAL */

static size_t path_entry_len(const char* path) {
	size_t len = 0;
	while (*path && *path != '/') {
		len++;
		path++;
	}
	return len;
}

static struct file_handle_t* devfs_open(struct mount_cntx_t* cntx, const char* path, uint32_t flags, uint32_t mode) {
	(void)cntx;
	(void)flags;
	(void)mode;
	struct dev_handle_t* dev_handle;
	size_t path_len;
	uint8_t cntrl;
	struct devfs_tree_node_t* info = devfs_root;

	while (info && *path) {
		info = info->sub;
		path_len = path_entry_len(path);

		cntrl = 1;
		for (; info; info = info->co) {
			if (path_len == kstrlen(info->name) && kmemcmp(path, info->name, path_len) == 0) {
				cntrl = 0;
				break;
			}
		}

		if (cntrl) {
			break; // file not found
		}

		path += path_len;

		// handle non EOS
		if (*path == '/') {
			path++;
		}
	}

	if (cntrl || !info) {
		// file not found;
		return 0;
	}

	dev_handle = kmalloc(sizeof(struct dev_handle_t));
	dev_handle->node = info;

	return (struct file_handle_t*)dev_handle;
}

static void devfs_close(struct file_handle_t* handle) {
	struct dev_handle_t* dev_handle = (struct dev_handle_t*)handle;
	if (!dev_handle) {
	 return;
	}

	switch (dev_handle->node->type) {
		case DEV_TYPE_DIR:
		case DEV_TYPE_TTY:
			break;
	}

	kfree(dev_handle);
}

static enum file_status_t devfs_stat(struct file_handle_t* handle, file_info_t* info) {
	struct dev_handle_t* dev_handle = (struct dev_handle_t*)handle;

	if (!dev_handle) {
		return FILE_ERROR;
	}

	kmemset(info, 0, sizeof(file_info_t));
	info->st_dev = devid;
	info->st_ino = dev_handle->node->inode_num;
	info->st_nlink = 1;

	switch (dev_handle->node->type) {
		case DEV_TYPE_DIR:
			info->st_mode = S_IFDIR;
			info->st_size = sizeof(struct dev_handle_t);
			info->st_blocks = sizeof(struct dev_handle_t);
			info->st_blksize = 1;
			break;
		case DEV_TYPE_TTY:
			info->st_mode = S_IFCHR;
			info->st_size = TTY_READ_BUFFER_SIZE;
			info->st_blocks = TTY_READ_BUFFER_SIZE;
			info->st_blksize = 1;
			break;
	}

	return FILE_OK;
}

static size_t devfs_read(struct file_handle_t* handle, void* buffer, size_t count) {
	struct dev_handle_t* dev_handle = (struct dev_handle_t*)handle;

	if (!dev_handle) {
		return FILE_ERROR;
	}

	switch (dev_handle->node->type) {
		case DEV_TYPE_DIR:
			return FILE_NO_SUPPORT;
		case DEV_TYPE_TTY:
			return tty_read(dev_handle->node->dev_handle.tty, buffer, count);
	}
}

static size_t devfs_write(struct file_handle_t* handle, const void* buffer, size_t count) {
	struct dev_handle_t* dev_handle = (struct dev_handle_t*)handle;

	if (!dev_handle) {
		return FILE_ERROR;
	}

	switch (dev_handle->node->type) {
		case DEV_TYPE_DIR:
			return FILE_NO_SUPPORT;
		case DEV_TYPE_TTY:
			tty_write(dev_handle->node->dev_handle.tty, buffer, count);
			return count;
	}
}

static uint8_t devfs_is_interactive(struct file_handle_t* handle) {
	struct dev_handle_t* dev_handle = (struct dev_handle_t*)handle;

	if (!dev_handle) {
		return FILE_ERROR;
	}

	switch (dev_handle->node->type) {
		case DEV_TYPE_DIR:
			return 0;
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

	*dev_handle2 = *dev_handle;

	return (struct file_handle_t*)dev_handle2;
}

static enum file_status_t devfs_open_dir(struct file_handle_t* handle) {
	struct dev_handle_t* dev_handle = (struct dev_handle_t*)handle;

	if (!dev_handle) {
		return 0;
	}

	switch (dev_handle->node->type) {
		case DEV_TYPE_DIR:
			dev_handle->read_index = dev_handle->node->sub;
			return FILE_OK;
		case DEV_TYPE_TTY:
			return FILE_NO_SUPPORT;
	}
}

static enum file_status_t devfs_read_dir(struct file_handle_t* handle, struct dir_info_t* info) {
	struct dev_handle_t* dev_handle = (struct dev_handle_t*)handle;

	if (!dev_handle->read_index) {
		return FILE_DNE;
	}

	info->inode_num = dev_handle->read_index->inode_num;
	info->rec_len = 0;
	info->seek_pos = 0;
	switch (dev_handle->read_index->type) {
		case DEV_TYPE_DIR:
			info->type = DT_DIR;
			break;
		case DEV_TYPE_TTY:
			info->type = DT_CHR;
			break;
	}
	kstrcpy(info->name, dev_handle->read_index->name);

	return FILE_OK;
}

static enum file_status_t devfs_next_dir(struct file_handle_t* handle) {
	struct dev_handle_t* dev_handle = (struct dev_handle_t*)handle;

	if (!dev_handle->read_index) {
		return FILE_DNE;
	}

	dev_handle->read_index = dev_handle->read_index->co;

	return FILE_OK;
}

void devfs_init(void) {
	dev_inode_num = 1;

	static char root_name[] = "";

	devfs_root_node.inode_num = dev_inode_num++;
	devfs_root_node.co = 0;
	devfs_root_node.sub = 0;
	devfs_root_node.name = root_name;
	devfs_root_node.type = DEV_TYPE_DIR;

#ifdef SERIAL
	static char serial1_name[] = "ttyS0";
	static char serial2_name[] = "ttyS1";

	devfs_serial1_node.inode_num = dev_inode_num++;
	devfs_serial1_node.co = devfs_root_node.sub;
	devfs_serial1_node.sub = 0;
	devfs_serial1_node.name = serial1_name;
	devfs_serial1_node.type = DEV_TYPE_TTY;
	devfs_serial1_node.dev_handle.tty = tty_com1();
	devfs_root_node.sub = &devfs_serial1_node;

	devfs_serial2_node.inode_num = dev_inode_num++;
	devfs_serial2_node.co = devfs_root_node.sub;
	devfs_serial2_node.sub = 0;
	devfs_serial2_node.name = serial2_name;
	devfs_serial2_node.type = DEV_TYPE_TTY;
	devfs_serial2_node.dev_handle.tty = tty_com2();
	devfs_root_node.sub = &devfs_serial2_node;
#endif /* SERIAL */

	devfs_root = &devfs_root_node;

	dev_mount = fs_mount("/dev", 0);

	fs_mount_assign_open(dev_mount, devfs_open);	
	fs_mount_assign_close(dev_mount, devfs_close);	
	fs_mount_assign_stat(dev_mount, devfs_stat);	
	fs_mount_assign_read(dev_mount, devfs_read);	
	fs_mount_assign_write(dev_mount, devfs_write);	
	fs_mount_assign_is_interactive(dev_mount, devfs_is_interactive);	
	fs_mount_assign_dup(dev_mount, devfs_dup);	
	fs_mount_assign_open_dir(dev_mount, devfs_open_dir);
	fs_mount_assign_read_dir(dev_mount, devfs_read_dir);
	fs_mount_assign_next_dir(dev_mount, devfs_next_dir);

	devid = fs_assign_id();
}
