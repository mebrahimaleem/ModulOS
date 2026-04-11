/* fs.c - kernel file system layer */
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

#include <core/fs.h>
#include <core/lock.h>
#include <core/alloc.h>
#include <core/logging.h>
#include <core/panic.h>

#include <lib/kmemcmp.h>
#include <lib/kmemcpy.h>
#include <lib/kstrcmp.h>
#include <lib/kstrcpy.h>
#include <lib/kstrlen.h>

static uint8_t fs_lock;

struct vfs_mount_t {
	struct mount_cntx_t* cntx;

	fs_open_t open;
	fs_close_t close;
	fs_stat_t stat;
};

struct fs_handle_t {
	struct vfs_mount_t* mount;
	struct file_handle_t* handle;
};

struct vfs_tree_node_t {
	struct vfs_tree_node_t* co;
	struct vfs_tree_node_t* sub;

	const char* name;

	struct vfs_mount_t* mount;
};

static struct vfs_tree_node_t vfs_root;

static inline char* path_next(char* path, size_t* len) {
	*len = 0;

	while (*path && *path != '/') {
		path++;
		(*len)++;
	}

	if (*path) {
		return path + 1;
	}

	return path;
}

void fs_init(void) {
	lock_init(&fs_lock);

	vfs_root.co = 0;
	vfs_root.sub = 0;
	vfs_root.name = "";
	vfs_root.mount = 0;
}

enum file_status_t fs_mount(
		const char* mountpoint,
		struct mount_cntx_t* cntx,
		fs_open_t open,
		fs_close_t close,
		fs_stat_t stat
		) {

	if (kstrcmp(mountpoint, "") && !vfs_root.mount) {
		vfs_root.mount = kmalloc(sizeof(struct vfs_mount_t));

		if (!vfs_root.mount) {
			logging_log_error("Failed to mount root");
			return FILE_ERROR;
		}

		vfs_root.mount->cntx = cntx;
		vfs_root.mount->open = open;
		vfs_root.mount->close = close;
		vfs_root.mount->stat = stat;

		return FILE_OK;
	}

	//TODO: support non root mount
	logging_log_error("Can only mount root");
	return FILE_ERROR;
}

struct fs_handle_t* fs_open(const char* path) {
	struct vfs_tree_node_t* node = &vfs_root, * walk = 0;
	size_t len = kstrlen(path);
	char* clean_path = kmalloc(len + 1);
	char* mount_path = (char*)"";
	struct vfs_mount_t* mount = 0;


	uint64_t num_chars = 0;
	uint64_t skip = 0;
	uint8_t dot_only = 1;

	const char* path_read = path + len;
	char* path_write = clean_path + len;

	for (; path_read >= path; --path_read) {
		switch (*path_read) {
			case '/': // absolute paths must start with /
				if (dot_only && num_chars == 1) {
					path_write += num_chars;
				}
				else if (dot_only && num_chars == 2) {
					skip++;
					path_write += num_chars;
				}
				else if (num_chars > 0) {
					if (skip) {
						path_write += num_chars;
						skip--;
					}
					else {
						*path_write = '/';
						path_write--;
					}
				}

				num_chars = 0;
				dot_only = 1;
				break;
			default:
				dot_only = 0;
				__attribute__((fallthrough));
			case '.':
				num_chars++;
				__attribute__((fallthrough));
			case 0:
				*path_write = *path_read;
				path_write--;
				break;
		}
	}

	path_write++; // one to rewind write pointer

	if (*path_write == '/') {
		path_write++; // one to skip /
	}


	lock_acquire(&fs_lock);

	do {
		if (node->mount) {
			mount_path = path_write;
			mount = node->mount;
		}

		path_read = path_write;
		path_write = path_next(path_write, &len);

		for (walk = node->sub; walk; walk = walk->co) {
			if (kstrlen(node->name) == len && kmemcmp(node->name, path_read, len) == 0) {
				node = walk;
				break;
			}
		}

	} while (walk && node != walk);

	lock_release(&fs_lock);

	if (!mount) {
		kfree(clean_path);
		return 0;
	}

	struct file_handle_t* handle = mount->open(mount->cntx, mount_path);

	kfree(clean_path);

	struct fs_handle_t* fs_handle = kmalloc(sizeof(struct fs_handle_t));
	fs_handle->handle = handle;
	fs_handle->mount = mount;
	return fs_handle;
}

void fs_close(struct fs_handle_t* handle) {
	lock_acquire(&fs_lock);

	handle->mount->close(handle->handle);
	lock_release(&fs_lock);

	kfree(handle);
}

enum file_status_t fs_stat(struct fs_handle_t* handle, struct file_info_t* info) {
	lock_acquire(&fs_lock);

	enum file_status_t ret = handle->mount->stat(handle->handle, info);
	lock_release(&fs_lock);
	return ret;
}
